#include <iostream>
#include <winsock2.h>

#define PORT 9909

struct sockaddr_in srv;
fd_set fr, fw, fe;
int nMaxFd;

int main()
{
    int nRet = 0;

    WSADATA ws;
    if (WSAStartup(MAKEWORD(2, 2), &ws) < 0)
    {
        std::cerr << std::endl
                  << "WSA failed to initialize";
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        std::cout << std::endl
                  << "WSA initialize";
    }

    int nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (nSocket < 0)
    {
        std::cerr << std::endl
                  << "Socket failed to open";
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        std::cout << std::endl
                  << "Socket opened at 0x"
                  << nSocket;
    }

    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT);
    srv.sin_addr.s_addr = INADDR_ANY;
    memset(&(srv.sin_zero), 0, 8);

    u_long optval = 0;
    nRet = ioctlsocket(nSocket, FIONBIO, &optval);
    if (nRet != 0)
    {
        std::cout << std::endl
                  << "ioctlsocket call failed";
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        std::cout << std::endl
                  << "ioctlsocket call passed";
    }

    nRet = bind(nSocket, (sockaddr *)&srv, sizeof(sockaddr));
    if (nRet < 0)
    {
        std::cerr << std::endl
                  << "Failed to bind to local port";
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        std::cout << std::endl
                  << "Successfully binded to local port";
    }

    nRet = listen(nSocket, 8);
    if (nRet < 0)
    {
        std::cerr << std::endl
                  << "Failed to start listening to local port";
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        std::cout << std::endl
                  << "Started listening to local port";
    }

    nMaxFd = nSocket;
    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;

    while (true)
    {
        FD_ZERO(&fr);
        FD_ZERO(&fw);
        FD_ZERO(&fe);

        FD_SET(nSocket, &fr);
        FD_SET(nSocket, &fe);

        std::cout << std::endl
                  << "Before select calls: " << fr.fd_count << std::endl;
        nRet = select(nMaxFd + 1, &fr, &fw, &fe, &tv);
        if (nRet > 0)
        {
            std::cout << nRet << " on port: " << PORT;
        }
        else if (nRet == 0)
        {
            std::cout << "nothin on port: " << PORT;
        }
        else
        {
            std::cout << "Socket failure";
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        std::cout << std::endl
                  << "After select calls: " << fr.fd_count << std::endl;
    }

    return 0;
}