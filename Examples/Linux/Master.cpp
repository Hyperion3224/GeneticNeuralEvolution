#include "./network/MasterServer.hpp"
#include "./network/net/Logger.hpp"

#include <csignal>
#include <atomic>
#include <chrono>
#include <thread>
#include <iostream>

static std::atomic<bool> g_stop{false};

#if defined(_WIN32)
static BOOL WINAPI onCtrlHandler(DWORD ctrlType)
{
    (void)ctrlType;
    g_stop.store(true);
    return TRUE;
}
#else
void onSig(int) { g_stop.store(true); }
#endif

int main(int argc, char **argv)
{
    uint16_t port = 5050;
    if (argc > 1)
    {
        try
        {
            port = static_cast<uint16_t>(std::stoi(argv[1]));
        }
        catch (...)
        {
            std::cerr << "Invalid port '" << argv[1] << "', using default 5050\n";
        }
    }

    dist::MasterConfig cfg;
    cfg.bindAddress = "0.0.0.0"; // listen on all interfaces
    cfg.port = port;
    cfg.maxNodes = 8; // <= accepts up to 8 concurrent node connections
    cfg.listenBacklog = 8;
    cfg.heartbeatInterval = std::chrono::milliseconds(2000);
    cfg.heartbeatTimeout = std::chrono::milliseconds(6000);

    dist::MasterServer master(cfg);
    if (!master.start())
    {
        LOG_ERROR("Failed to start master on %s:%u", cfg.bindAddress.c_str(), unsigned(cfg.port));
        return 1;
    }

#if defined(_WIN32)
    SetConsoleCtrlHandler(onCtrlHandler, TRUE);
#else
    std::signal(SIGINT, onSig);
    std::signal(SIGTERM, onSig);
#endif

    LOG_INFO("Master listening on %s:%u; waiting for up to %zu node connections. Press Ctrl+C to stop.",
             cfg.bindAddress.c_str(), unsigned(cfg.port), cfg.maxNodes);

    size_t lastCount = 0;
    while (!g_stop.load())
    {
        auto snap = master.nodes();
        if (snap.size() != lastCount)
        {
            lastCount = snap.size();
            LOG_INFO("Connected nodes: %zu/%zu", lastCount, cfg.maxNodes);
            master.print_resource_table();
            if (lastCount == cfg.maxNodes)
            {
                LOG_INFO("Reached max nodes (%zu). New connections will be rejected until a slot frees.", lastCount);
            }
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    LOG_INFO("Shutting down master...");
    master.stop();
    LOG_INFO("Master exited.");
    return 0;
}
