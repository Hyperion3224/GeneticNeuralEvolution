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
#include "../Libraries/ThreadPool.hpp"
#include "../Libraries/NeuralNetwork.hpp"

using std::cin;
using std::cout;
using std::endl;
using std::string;

#define PORT 9909

std::string getEthernetIP();
unsigned int getRam();
unsigned int getThreads();

int main()
{
    int WorkerThreads = getThreads() - 1;

    int servSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (servSocket < 0)
    {
        perror("socket");
        return 1;
    }

    sockaddr_in MasterComputer{};
    MasterComputer.sin_family = AF_INET;
    MasterComputer.sin_port = htons(9909);
    inet_pton(AF_INET, "192.168.88.1", &MasterComputer.sin_addr);

    if (connect(servSocket, (sockaddr *)&MasterComputer, sizeof(MasterComputer)) < 0)
    {
        perror("connect failed");
        return 1;
    }
    std::cout << "Connected!" << std::endl;

    string connectionMessage = "";
    connectionMessage += std::to_string(WorkerThreads) + "\n" + std::to_string(getRam()) + "\n" + getEthernetIP();
    send(servSocket, connectionMessage.c_str(), connectionMessage.size(), 0);

    close(servSocket);
    return 0;
}

// functions

unsigned int getRam()
{
    std::ifstream meminfo("/proc/meminfo");
    std::string key;
    long value;
    std::string unit;
    unsigned int out;

    while (meminfo >> key >> value >> unit)
    {
        if (key == "MemTotal:")
        {
            out += (value / 1024);
        }
    }
    return out;
}

unsigned int getThreads()
{
    unsigned int threads = std::thread::hardware_concurrency();
    return threads;
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
            if (strcmp(ifa->ifa_name, "enp5s0") == 0 || strcmp(ifa->ifa_name, "enp3s0f0") == 0 || strcmp(ifa->ifa_name, "enp4s0f0") == 0)
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
