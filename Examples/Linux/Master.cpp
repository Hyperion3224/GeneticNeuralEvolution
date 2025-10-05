// Master.cpp (or your main)
#include "./MasterServer.hpp"

int main() {
    dist::MasterConfig cfg;
    cfg.bindAddress = "0.0.0.0";
    cfg.port        = 5050;
    cfg.maxNodes    = 8;

    dist::MasterServer master(cfg);
    if (!master.start()) return 1;

    // ... run your orchestrator main loop here, e.g., CLI or control plane ...
    // For demo, wait on stdin
    std::puts("Master running. Press ENTER to stop.");
    (void)std::getchar();

    master.stop();
    return 0;
}
