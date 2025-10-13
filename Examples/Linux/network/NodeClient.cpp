#include "NodeClient.hpp"

#include <cstring>
#include <thread>
#include <chrono>

#if defined(_WIN32)
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "Ws2_32.lib")
#else
#include <netdb.h>
#include <netinet/tcp.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#endif

using namespace std::chrono_literals;

// ---------- tiny helpers ----------
namespace
{

    // create a TCP socket and connect to host:port (IPv4)
    // returns INVALID_SOCKET on failure
    static socket_t dialTcpIPv4(const std::string &host, uint16_t port, std::string &outPeerIp)
    {
#if defined(_WIN32)
        WSADATA wsa{};
        if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        {
            LOG_ERROR("WSAStartup failed");
            return INVALID_SOCKET;
        }
#endif

        socket_t s = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET)
        {
            LOG_ERROR("socket() failed");
            return INVALID_SOCKET;
        }

        // TCP_NODELAY (optional, helps interactive control messages)
        int one = 1;
        ::setsockopt(s, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char *>(&one), sizeof(one));

        sockaddr_in sin{};
        sin.sin_family = AF_INET;
        sin.sin_port = htons(port);

        // try to parse as dotted quad; if that fails, resolve
        if (::inet_pton(AF_INET, host.c_str(), &sin.sin_addr) != 1)
        {
#if defined(_WIN32)
            addrinfoW hints{};
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            wchar_t whost[256]{};
            mbstowcs(whost, host.c_str(), sizeof(whost) / sizeof(wchar_t) - 1);
            addrinfoW *res = nullptr;
            if (GetAddrInfoW(whost, nullptr, &hints, &res) != 0 || !res)
            {
                LOG_ERROR("DNS resolve failed for host '%s'", host.c_str());
                closesocket(s);
                return INVALID_SOCKET;
            }
            // take first A record
            sockaddr_in *ain = reinterpret_cast<sockaddr_in *>(res->ai_addr);
            sin.sin_addr = ain->sin_addr;
            FreeAddrInfoW(res);
#else
            addrinfo hints{};
            hints.ai_family = AF_INET;
            hints.ai_socktype = SOCK_STREAM;
            hints.ai_protocol = IPPROTO_TCP;
            addrinfo *res = nullptr;
            if (::getaddrinfo(host.c_str(), nullptr, &hints, &res) != 0 || !res)
            {
                LOG_ERROR("DNS resolve failed for host '%s'", host.c_str());
                ::close(s);
                return INVALID_SOCKET;
            }
            // take first A record
            sockaddr_in *ain = reinterpret_cast<sockaddr_in *>(res->ai_addr);
            sin.sin_addr = ain->sin_addr;
            ::freeaddrinfo(res);
#endif
        }

        if (::connect(s, reinterpret_cast<sockaddr *>(&sin), sizeof(sin)) != 0)
        {
#if defined(_WIN32)
            LOG_ERROR("connect() failed with %d", int(WSAGetLastError()));
            closesocket(s);
#else
            LOG_ERROR("connect() failed: %s", std::strerror(errno));
            ::close(s);
#endif
            return INVALID_SOCKET;
        }

        char ipbuf[INET_ADDRSTRLEN]{};
        ::inet_ntop(AF_INET, &sin.sin_addr, ipbuf, sizeof(ipbuf));
        outPeerIp = ipbuf;
        return s;
    }

#if defined(_WIN32)
    static uint64_t totalRamBytes()
    {
        MEMORYSTATUSEX msx{};
        msx.dwLength = sizeof(msx);
        if (GlobalMemoryStatusEx(&msx))
        {
            return static_cast<uint64_t>(msx.ullTotalPhys);
        }
        return 0;
    }
#else
#include <sys/sysinfo.h>
    static uint64_t totalRamBytes()
    {
#ifdef __linux__
        struct sysinfo si{};
        if (sysinfo(&si) == 0)
        {
            return static_cast<uint64_t>(si.totalram) * static_cast<uint64_t>(si.mem_unit);
        }
        return 0;
#else
        // Fallback: try sysconf on POSIX-ish systems
        long pages = ::sysconf(_SC_PHYS_PAGES);
        long psize = ::sysconf(_SC_PAGESIZE);
        if (pages > 0 && psize > 0)
        {
            return static_cast<uint64_t>(pages) * static_cast<uint64_t>(psize);
        }
        return 0;
#endif
    }
#endif

} // namespace

// ---------- NodeClient impl ----------

NodeClient::NodeClient(std::string masterHost, uint16_t masterPort)
    : masterHost_(std::move(masterHost)),
      masterPort_(masterPort),
      // Initialize with an invalid handle; we reconstruct it after dialing.
      conn_(INVALID_SOCKET, /*peerIp*/ std::string{})
{
}

bool NodeClient::connect()
{
    std::string peerIp;
    socket_t s = dialTcpIPv4(masterHost_, masterPort_, peerIp);
    if (s == INVALID_SOCKET)
    {
        LOG_ERROR("Failed to connect to %s:%u", masterHost_.c_str(), unsigned(masterPort_));
        return false;
    }

    // Destroy the placeholder Connection and reconstruct in-place with the live socket.
    conn_.~Connection();
    new (&conn_) dist::Connection(s, peerIp);

    LOG_INFO("Connected to master %s:%u (peer=%s)", masterHost_.c_str(),
             unsigned(masterPort_), peerIp.c_str());
    return true;
}

bool NodeClient::sendSpecsAndAwaitConfig(NodeConfig &outConfig)
{
    // Gather host specs
    NodeSpecs specs = gatherSpecs();

    // Translate to current wire payload (ResourceReportPayload = total RAM bytes, hardware threads)
    dist::ResourceReportPayload pld{};
    pld.ramBytes = (specs.ram_mb_free > 0) ? (specs.ram_mb_free * 1024ull * 1024ull) : totalRamBytes();
    pld.threads = specs.hardware_threads > 0 ? specs.hardware_threads : 1;

    std::vector<uint8_t> payload = dist::encodeResourceReport(pld);

    if (!conn_.sendMessage(dist::MsgType::RESOURCE_REPORT, payload))
    {
        LOG_ERROR("Failed to send RESOURCE_REPORT to master");
        return false;
    }
    LOG_INFO("Sent RESOURCE_REPORT: ram=%llu bytes, threads=%u",
             static_cast<unsigned long long>(pld.ramBytes), pld.threads);

    // With the protocol you provided, there is no explicit "config" message.
    // We’ll service keepalives and return true once the report is sent.
    // If/when you add a CONFIG message, handle it in the switch below.
    auto start = std::chrono::steady_clock::now();
    while (true)
    {
        // Soft, brief window to answer an immediate PING or SHUTDOWN after connect
        if (std::chrono::steady_clock::now() - start > 500ms)
            break;

        dist::MsgType type{};
        std::vector<uint8_t> in;
        if (!conn_.recvMessage(type, in))
        {
            // Non-fatal: if master sends nothing in the short window, we’re done.
            break;
        }
        switch (type)
        {
        case dist::MsgType::PING:
        {
            LOG_DEBUG("Received PING; replying PONG");
            conn_.sendMessage(dist::MsgType::PONG, {});
            break;
        }
        case dist::MsgType::SHUTDOWN:
        {
            LOG_WARN("Received SHUTDOWN from master");
            return false;
        }
        // Placeholder for future: CONFIG
        // case dist::MsgType::CONFIG:
        //   parse config (e.g., JSON or binary), fill outConfig, return true;
        //   break;
        default:
            LOG_WARN("Unexpected message type %u from master", unsigned(type));
            break;
        }
    }

    // No config defined in current protocol; leave defaults and report success of the handshake.
    outConfig = NodeConfig{};
    return true;
}

// ---------- static helpers from NodeClient.hpp ----------

NodeSpecs NodeClient::gatherSpecs()
{
    NodeSpecs s{};
    s.ip = ""; // master infers IP from the socket; no need to set here
    s.hardware_threads = std::max(1u, std::thread::hardware_concurrency());

    // We’ll report *total* RAM via totalRamBytes(); if you truly want "free" MB for your app,
    // you can replace this with a free-memory query. For now, keep it simple and portable.
    uint64_t total = totalRamBytes();
    s.ram_mb_free = (total > 0) ? (total / (1024ull * 1024ull)) : 0;

    return s;
}

// These JSON helpers are present because NodeClient.hpp declares them.
// They’re not used by the current wire protocol, but we provide minimal definitions.
json NodeClient::specsToJson(const NodeSpecs &s)
{
    json j;
    j["ip"] = s.ip;
    j["ram_mb_free"] = s.ram_mb_free;
    j["hardware_threads"] = s.hardware_threads;
    return j;
}

NodeConfig NodeClient::configFromJson(const json &j)
{
    NodeConfig c{};
    if (j.contains("node_index"))
        c.node_index = j["node_index"].get<int>();
    if (j.contains("is_first"))
        c.is_first = j["is_first"].get<bool>();
    if (j.contains("is_last"))
        c.is_last = j["is_last"].get<bool>();
    if (j.contains("layers"))
        c.layers = j["layers"].get<std::vector<int>>();
    if (j.contains("array_size"))
        c.array_size = j["array_size"].get<std::uint64_t>();
    if (j.contains("next_node_addr"))
        c.next_node_addr = j["next_node_addr"].get<std::string>();
    return c;
}
