#pragma once

#include <vector>
#include <string>

#include <netinet/in.h>
#include <arpa/inet.h>

namespace Distributed
{
    struct PartitionConfig
    {
        int NumComputers;
        std::vector<int> LayerTypes;
        in_addr_t PrevNode;
        in_addr_t NextNode;
    };

    struct ComputerDetails
    {
        in_addr_t IP;
        int ThreadCount;
        int RAM;
        int Socket;

        ComputerDetails *nextComputer;

        ComputerDetails(const in_addr_t _IP, const int _Socket, const int _ThreadCount, const int _RAM, ComputerDetails *_nextComputer) : IP(_IP), Socket(_Socket), ThreadCount(_ThreadCount), RAM(_RAM), nextComputer(_nextComputer)
        {
        }
    };

    ComputerDetails *ToComputerDetails(const std::string *IP_ADDRS, int *Sockets, const int *threads, const int *RAM, const int NumComputers)
    {
        const int index = NumComputers - 1;
        if (NumComputers > 0)
        {
            return new ComputerDetails(inet_addr(IP_ADDRS[index].c_str()), Sockets[index], threads[index], RAM[index], ToComputerDetails(IP_ADDRS, Sockets, threads, RAM, index));
        }
        else
        {
            return nullptr;
        }
    }

    std::vector<PartitionConfig> PartitionResources(ComputerDetails *head)
    {
        std::vector<PartitionConfig> List;
        int TotalThreads = 0, TotalRAM = 0;

        ComputerDetails *TrueHead = head;
        while (head != nullptr)
        {
            TotalRAM += head->RAM;
            TotalThreads += head->ThreadCount;
            head = head->nextComputer;
        }
    }

    void FreeComputerDetails(ComputerDetails *head)
    {
        while (head != nullptr)
        {
            ComputerDetails *next = head->nextComputer;
            delete head;
            head = next;
        }
    }
}