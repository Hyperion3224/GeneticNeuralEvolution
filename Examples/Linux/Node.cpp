#include <iostream>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <thread>
#include <fstream>

using std::cin;
using std::cout;
using std::endl;
using std::string;

#define PORT 9909

std::string getEthernetIP();
std::string getRam();
std::string getThreads();

int main()
{

    int servSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (servSocket < 0)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in srv{};
    srv.sin_family = AF_INET;
    srv.sin_port = htons(9909);
    inet_pton(AF_INET, "192.168.88.1", &srv.sin_addr);

    if (connect(servSocket, (sockaddr *)&srv, sizeof(srv)) < 0)
    {
        perror("connect failed");
        return 1;
    }
    std::cout << "Connected!" << std::endl;

    string connectionMessage = "";

    connectionMessage += getThreads() + "\n" + getRam() + "\n" + getEthernetIP();

    send(servSocket, connectionMessage.c_str(), connectionMessage.size(), 0);

    close(servSocket);

    return 0;
}

std::string getRam()
{
    std::ifstream meminfo("/proc/meminfo");
    std::string key;
    long value;
    std::string unit;
    std::string out;

    while (meminfo >> key >> value >> unit)
    {
        if (key == "MemTotal:")
        {
            out += std::to_string(value / 1024);
        }
    }
    return out;
}

std::string getThreads()
{
    std::string out;

    unsigned int threads = std::thread::hardware_concurrency();
    out += std::to_string(threads);

    return out;
}

std::string getEthernetIP()
{
    struct ifaddrs *ifaddr, *ifa;
    void *tmpAddrPtr = nullptr;

    std::string out;

    if (getifaddrs(&ifaddr) == -1)
    {
        perror("getifaddrs");
        return out;
    }

    for (ifa = ifaddr; ifa != nullptr; ifa = ifa->ifa_next)
    {
        if (ifa->ifa_addr && ifa->ifa_addr->sa_family == AF_INET)
        {
            tmpAddrPtr = &((struct sockaddr_in *)ifa->ifa_addr)->sin_addr;
            char addressBuffer[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, tmpAddrPtr, addressBuffer, INET_ADDRSTRLEN);
            if (strcmp(ifa->ifa_name, "enp5s0") == 0 || strcmp(ifa->ifa_name, "enp3s0f0") == 0)
            {
                out += addressBuffer;
                freeifaddrs(ifaddr);
                return out;
            }
        }
    }

    freeifaddrs(ifaddr);
    return out;
}
