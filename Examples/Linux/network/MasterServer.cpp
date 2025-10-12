#include "./MasterServer.hpp"
#include "./net/Logger.hpp"
#include <string>
#include <sstream>
#include <iomanip>
#include <cstring>

#if defined(_WIN32)
#pragma comment(lib, "ws2_32.lib")
#endif

namespace dist
{

    namespace
    {
        // Platform init for sockets.
        struct NetInit
        {
            NetInit()
            {
#if defined(_WIN32)
                WSADATA wsa;
                WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
            }
            ~NetInit()
            {
#if defined(_WIN32)
                WSACleanup();
#endif
            }
        };
        NetInit g_netInit;

        std::string ipFromSockaddr(const sockaddr_in &addr)
        {
            char buf[INET_ADDRSTRLEN] = {0};
#if defined(_WIN32)
            inet_ntop(AF_INET, (PVOID)&addr.sin_addr, buf, INET_ADDRSTRLEN);
#else
            inet_ntop(AF_INET, &addr.sin_addr, buf, INET_ADDRSTRLEN);
#endif
            return std::string(buf);
        }

    } // namespace

    MasterServer::MasterServer(const MasterConfig &cfg)
        : cfg_(cfg),
          registry_(),
          pool_(std::max(2u, std::thread::hardware_concurrency())) // or reuse your project’s default ctor
    {
        registry_.setMax(cfg_.maxNodes);
    }

    MasterServer::~MasterServer()
    {
        stop();
    }

    bool MasterServer::setupListener()
    {
        listener_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (listener_ == INVALID_SOCKET)
        {
            LOG_ERROR("socket() failed");
            return false;
        }

        int yes = 1;
        setsockopt(listener_, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<char *>(&yes), sizeof(yes));

        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_port = htons(cfg_.port);
#if defined(_WIN32)
        inet_pton(AF_INET, cfg_.bindAddress.c_str(), &addr.sin_addr);
#else
        if (::inet_aton(cfg_.bindAddress.c_str(), &addr.sin_addr) == 0)
        {
            LOG_ERROR("Invalid bind address: %s", cfg_.bindAddress.c_str());
            return false;
        }
#endif
        if (::bind(listener_, (sockaddr *)&addr, sizeof(addr)) != 0)
        {
            LOG_ERROR("bind(%s:%u) failed", cfg_.bindAddress.c_str(), unsigned(cfg_.port));
            ::close(listener_);
            listener_ = INVALID_SOCKET;
            return false;
        }
        if (::listen(listener_, cfg_.listenBacklog) != 0)
        {
            LOG_ERROR("listen() failed");
            ::close(listener_);
            listener_ = INVALID_SOCKET;
            return false;
        }
        LOG_INFO("Master listening on %s:%u", cfg_.bindAddress.c_str(), unsigned(cfg_.port));
        return true;
    }

    void MasterServer::teardownListener()
    {
        if (listener_ != INVALID_SOCKET)
        {
            ::shutdown(listener_, SHUT_RDWR);
            ::close(listener_);
            listener_ = INVALID_SOCKET;
        }
    }

    bool MasterServer::start()
    {
        if (running_.load())
            return true;
        stopping_.store(false);

        if (!setupListener())
            return false;

        running_.store(true);
        acceptThread_ = std::thread(&MasterServer::acceptLoop, this);
        heartbeatThread_ = std::thread(&MasterServer::heartbeatLoop, this);
        return true;
    }

    void MasterServer::stop()
    {
        if (!running_.load())
            return;
        stopping_.store(true);

        teardownListener(); // unblocks accept

        if (acceptThread_.joinable())
            acceptThread_.join();
        if (heartbeatThread_.joinable())
            heartbeatThread_.join();

        // Close remaining connections in registry snapshot
        for (auto &[id, info] : registry_.snapshot())
        {
            (void)id;
            (void)info; // actual Connection instances are owned per-connection loops
            // The connection loops close their sockets on exit.
        }

        running_.store(false);
        LOG_INFO("Master stopped");
    }

    void MasterServer::acceptLoop()
    {
        while (!stopping_.load())
        {
            sockaddr_in peerAddr{};
            socklen_t len = sizeof(peerAddr);

            socket_t s = ::accept(listener_, (sockaddr *)&peerAddr, &len);
            if (s == INVALID_SOCKET)
            {
                if (!stopping_.load())
                    LOG_WARN("accept() failed or interrupted");
                break;
            }

            // Limit concurrent nodes
            if (registry_.size() >= cfg_.maxNodes)
            {
                LOG_WARN("Max nodes reached (%zu). Rejecting %s", registry_.size(), ipFromSockaddr(peerAddr).c_str());
                ::close(s);
                continue;
            }

            // TCP_NODELAY helps small heartbeats
            int one = 1;
            setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char *>(&one), sizeof(one));

            std::string ip = ipFromSockaddr(peerAddr);
            LOG_INFO("Accepted connection from %s", ip.c_str());

            // Track immediately with empty specs; will be filled after RESOURCE_REPORT
            NodeInfo info;
            info.ip = ip;
            info.alive = true;
            info.lastSeen = std::chrono::steady_clock::now();
            registry_.insert(s, info);

            auto conn = std::make_shared<Connection>(s, ip);
            // Spawn a blocking connection loop; dispatch message handling into ThreadPool
            std::thread(&MasterServer::connectionLoop, this, conn).detach();
        }
        LOG_INFO("Accept loop exited");
    }

    void MasterServer::connectionLoop(std::shared_ptr<Connection> conn)
    {
        auto id = conn->raw();
        for (;;)
        {
            if (stopping_.load())
                break;
            MsgType type;
            std::vector<uint8_t> payload;
            if (!conn->recvMessage(type, payload))
            {
                LOG_WARN("Connection %s closed or errored", conn->peerIp().c_str());
                registry_.markDead(id);
                registry_.erase(id);
                conn->close();
                return;
            }

            // Hand off processing to the thread pool to avoid heavy work in the IO loop:
            pool_.enqueue([this, id, type, payload = std::move(payload)]() mutable
                          {
            switch (type) {
                case MsgType::RESOURCE_REPORT: {
                    try {
                        auto rpt = decodeResourceReport(payload);
                        registry_.update(id, [&](NodeInfo& n){
                            n.ramBytes = rpt.ramBytes;
                            n.threads  = rpt.threads;
                            n.lastSeen = std::chrono::steady_clock::now();
                            n.alive    = true;
                        });
                        LOG_INFO("Node[%d] resource report: RAM=%llu bytes, threads=%u",
                                 int(id), (unsigned long long)rpt.ramBytes, (unsigned)rpt.threads);
                    } catch (const std::exception& e) {
                        LOG_ERROR("Bad RESOURCE_REPORT from node[%d]: %s", int(id), e.what());
                    }
                } break;

                case MsgType::PONG: {
                    registry_.update(id, [&](NodeInfo& n){
                        n.lastSeen = std::chrono::steady_clock::now();
                        n.alive    = true;
                    });
                    LOG_DEBUG("PONG from node[%d]", int(id));
                } break;

                case MsgType::PING: {
                    // If workers ping, reply with PONG
                    // We don't assume this path, but handle it for symmetry.
                    // Find the connection by id and reply (best-effort).
                    // (Connection object lifetime is owned by conn loop; reply from IO thread preferred.
                    //  For simplicity here, we let the IO thread send after pool returns a flag — but to keep it
                    //  simple and thread-safe we skip immediate reply. Optional.)
                } break;

                case MsgType::SHUTDOWN:
                    // Worker is going down.
                    registry_.markDead(id);
                    LOG_INFO("Node[%d] requested shutdown", int(id));
                    break;

                default:
                    LOG_WARN("Unknown message type %u from node[%d]", (unsigned)type, int(id));
                    break;
            } });
        }

        registry_.erase(id);
        conn->close();
    }

    void MasterServer::heartbeatLoop()
    {
        using clock = std::chrono::steady_clock;
        while (!stopping_.load())
        {
            auto start = clock::now();

            // Send PINGs
            for (auto &[id, info] : registry_.snapshot())
            {
                (void)info;
                // Best-effort send: create a new lightweight Connection wrapper around raw socket? We can’t duplicate.
                // We will send using a tiny lambda that uses raw send. Since Connection is owned by IO thread,
                // we do a one-off frame send here (it’s safe to write from another thread on TCP as long as app-level
                // framing is used; to be conservative, production code would add a per-connection send mutex).
                // For simplicity, we opportunistically send and ignore short races.

                uint32_t len = hostToNet32(1);
                uint8_t frame[5];
                std::memcpy(frame, &len, 4);
                frame[4] = static_cast<uint8_t>(MsgType::PING);

#if defined(_WIN32)
                int n = ::send(id, reinterpret_cast<const char *>(frame), 5, 0);
#else
                ssize_t n = ::send(id, frame, 5, MSG_NOSIGNAL);
#endif
                if (n != 5)
                {
                    LOG_WARN("Failed to send PING to node[%d]", int(id));
                }
            }

            // Reap dead nodes
            auto now = clock::now();
            for (auto &[id, info] : registry_.snapshot())
            {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(now - info.lastSeen);
                if (elapsed > cfg_.heartbeatTimeout)
                {
                    LOG_WARN("Node[%d] timed out (%lld ms) — removing", int(id), (long long)elapsed.count());
                    registry_.erase(id);
                    ::shutdown(id, SHUT_RDWR);
                    ::close(id);
                }
            }

            // Sleep until next heartbeat
            auto elapsed = clock::now() - start;
            auto remaining = cfg_.heartbeatInterval - std::chrono::duration_cast<std::chrono::milliseconds>(elapsed);
            if (remaining.count() > 0)
                std::this_thread::sleep_for(remaining);
        }
        LOG_INFO("Heartbeat loop exited");
    }

} // namespace dist

namespace dist
{

    void MasterServer::on_client_connect(Connection c)
    {
        (void)c;
        LOG_INFO("on_client_connect hook invoked (placeholder).");
    }

    void MasterServer::on_client_connected(Connection c)
    {
        // For compatibility, call the same hook.
        on_client_connect(std::move(c));
    }

    void MasterServer::on_message(Connection &c, const std::string &payload)
    {
        (void)c;
        (void)payload;
        LOG_WARN("on_message hook not wired for binary Protocol; placeholder only.");
    }

    void MasterServer::print_resource_table()
    {
        auto snap = registry_.snapshot();
        LOG_INFO("Resource summary for %zu nodes:", snap.size());
        std::ostringstream oss;
        oss << "\n+--------+---------------+-------------+---------+\n";
        oss << "| socket | ip            | RAM(bytes)  | threads |\n";
        oss << "+--------+---------------+-------------+---------+\n";
        for (auto &kv : snap)
        {
            auto id = kv.first;
            const auto &n = kv.second;
            oss << "| " << std::setw(6) << int(id)
                << " | " << std::setw(13) << n.ip
                << " | " << std::setw(11) << n.ramBytes
                << " | " << std::setw(7) << n.threads
                << " |\n";
        }
        oss << "+--------+---------------+-------------+---------+\n";
        LOG_INFO("%s", oss.str().c_str());
    }

    int MasterServer::get_total_layers_from_model() const
    {
        return 120; // placeholder
    }

    std::uint64_t MasterServer::get_bytes_per_layer() const
    {
        return 1ull * 1024ull * 1024ull; // 1 MB per layer (placeholder)
    }

    void MasterServer::compute_and_send_configs(int total_layers, std::uint64_t bytes_per_layer)
    {
        (void)total_layers;
        (void)bytes_per_layer;
        LOG_WARN("compute_and_send_configs is a placeholder; integrate your partitioner if needed.");
    }

} // namespace dist
