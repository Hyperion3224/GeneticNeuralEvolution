#pragma once
#include <atomic>
#include <thread>
#include <vector>
#include <functional>
#include <condition_variable>
#include "Registry.hpp"
#include "../../../Libraries/ThreadPool.hpp"
#include "Protocol.hpp"

namespace dist {

struct MasterConfig {
    std::string bindAddress = "0.0.0.0";
    uint16_t    port        = 5050;
    size_t      maxNodes    = 8;
    std::chrono::milliseconds heartbeatInterval{2000};
    std::chrono::milliseconds heartbeatTimeout {6000};
    int         listenBacklog = 8;
};

class MasterServer {
public:
    explicit MasterServer(const MasterConfig& cfg);
    ~MasterServer();

    bool start();           // create listener, start accept + heartbeat threads
    void stop();            // graceful shutdown
    bool isRunning() const { return running_.load(); }

    // expose registry snapshot (thread-safe copy)
    std::vector<std::pair<socket_t, NodeInfo>> nodes() const { return registry_.snapshot(); }

private:
    void acceptLoop();
    void connectionLoop(std::shared_ptr<Connection> conn);
    void heartbeatLoop();

    bool setupListener();
    void teardownListener();

private:
    MasterConfig cfg_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopping_{false};

    socket_t listener_{INVALID_SOCKET};
    std::thread acceptThread_;
    std::thread heartbeatThread_;

    ConnectionRegistry registry_;
    ThreadPool pool_; // reuse your ThreadPool for message processing
};

} // namespace dist
