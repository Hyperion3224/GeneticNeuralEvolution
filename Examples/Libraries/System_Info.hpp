#pragma once
#include <cstdint>
#include <thread>
#include <string>

namespace sys_info
{

    // Returns number of hardware threads (logical cores). Always >= 1.
    inline unsigned hardware_threads()
    {
        unsigned n = std::thread::hardware_concurrency();
        return n ? n : 1;
    }

    // Returns available/free RAM in megabytes (best effort, cross-platform).
    std::uint64_t free_ram_mb();

    // Returns total RAM in megabytes if available; otherwise 0.
    std::uint64_t total_ram_mb();

    // Best-effort local IP string (may return "0.0.0.0" if unknown).
    std::string local_ip();

} // namespace sysinfo
