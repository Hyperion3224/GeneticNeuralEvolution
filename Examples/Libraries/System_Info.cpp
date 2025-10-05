#include "System_Info.hpp"

#if defined(_WIN32)
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")
#elif defined(__APPLE__) || defined(__linux__)
#include <sys/types.h>
#include <ifaddrs.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#if defined(__APPLE__)
#include <sys/sysctl.h>
#include <mach/mach.h>
#endif
#endif

#include <cstring>
#include <string>

namespace sys_info
{

    std::uint64_t free_ram_mb()
    {
#if defined(_WIN32)
        MEMORYSTATUSEX st;
        st.dwLength = sizeof(st);
        if (GlobalMemoryStatusEx(&st))
        {
            return static_cast<std::uint64_t>(st.ullAvailPhys / (1024ull * 1024ull));
        }
        return 0;
#elif defined(__APPLE__)
        mach_msg_type_number_t count = HOST_VM_INFO_COUNT;
        vm_statistics64_data_t vmstat;
        mach_port_t host_port = mach_host_self();
        if (host_statistics64(host_port, HOST_VM_INFO, reinterpret_cast<host_info64_t>(&vmstat), &count) == KERN_SUCCESS)
        {
            std::uint64_t free_bytes = (static_cast<std::uint64_t>(vmstat.free_count) +
                                        static_cast<std::uint64_t>(vmstat.inactive_count)) *
                                       static_cast<std::uint64_t>(sysconf(_SC_PAGESIZE));
            return free_bytes / (1024ull * 1024ull);
        }
        return 0;
#else // Linux
        struct ::sysinfo info;
        if (sysinfo(&info) == 0)
        {
            // mem_unit may be 1 on some kernels; guard overflow
            std::uint64_t unit = info.mem_unit ? info.mem_unit : 1;
            std::uint64_t free_bytes = static_cast<std::uint64_t>(info.freeram) * unit + static_cast<std::uint64_t>(info.bufferram) * unit + static_cast<std::uint64_t>(info.sharedram) * unit;
            return free_bytes / (1024ull * 1024ull);
        }
        return 0;
#endif
    }

    std::uint64_t total_ram_mb()
    {
#if defined(_WIN32)
        MEMORYSTATUSEX st;
        st.dwLength = sizeof(st);
        if (GlobalMemoryStatusEx(&st))
        {
            return static_cast<std::uint64_t>(st.ullTotalPhys / (1024ull * 1024ull));
        }
        return 0;
#elif defined(__APPLE__)
        std::uint64_t mem = 0;
        size_t len = sizeof(mem);
        int mib[2] = {CTL_HW, HW_MEMSIZE};
        if (sysctl(mib, 2, &mem, &len, nullptr, 0) == 0)
        {
            return mem / (1024ull * 1024ull);
        }
        return 0;
#else // Linux
        struct sysinfo info;
        if (sysinfo(&info) == 0)
        {
            std::uint64_t unit = info.mem_unit ? info.mem_unit : 1;
            std::uint64_t total_bytes = static_cast<std::uint64_t>(info.totalram) * unit;
            return total_bytes / (1024ull * 1024ull);
        }
        return 0;
#endif
    }

    std::string local_ip()
    {
#if defined(_WIN32)
        WSADATA wsaData;
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            return "0.0.0.0";
        char hostname[256] = {0};
        if (gethostname(hostname, sizeof(hostname)) != 0)
        {
            WSACleanup();
            return "0.0.0.0";
        }
        addrinfo hints = {}, *info = nullptr;
        hints.ai_family = AF_INET; // prefer IPv4 for simplicity
        if (getaddrinfo(hostname, nullptr, &hints, &info) != 0)
        {
            WSACleanup();
            return "0.0.0.0";
        }
        std::string result = "0.0.0.0";
        for (auto p = info; p != nullptr; p = p->ai_next)
        {
            sockaddr_in *addr = reinterpret_cast<sockaddr_in *>(p->ai_addr);
            char buf[INET_ADDRSTRLEN];
            if (inet_ntop(AF_INET, &(addr->sin_addr), buf, sizeof(buf)))
            {
                result = buf;
                break;
            }
        }
        freeaddrinfo(info);
        WSACleanup();
        return result;
#else
        struct ifaddrs *ifaddr = nullptr;
        if (getifaddrs(&ifaddr) == -1)
            return "0.0.0.0";
        std::string result = "0.0.0.0";
        for (auto *ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
        {
            if (!ifa->ifa_addr)
                continue;
            if (ifa->ifa_addr->sa_family == AF_INET)
            {
                auto *sa = reinterpret_cast<sockaddr_in *>(ifa->ifa_addr);
                char buf[INET_ADDRSTRLEN];
                if (inet_ntop(AF_INET, &sa->sin_addr, buf, sizeof(buf)))
                {
                    // Skip loopback
                    if (std::strcmp(buf, "127.0.0.1") != 0)
                    {
                        result = buf;
                        break;
                    }
                }
            }
        }
        freeifaddrs(ifaddr);
        return result;
#endif
    }

} // namespace sysinfo
