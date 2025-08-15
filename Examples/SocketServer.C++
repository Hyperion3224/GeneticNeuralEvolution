#include <iostream>
#include <winsock2.h>

#define PORT 9909

struct sockaddr_in srv;
fd_set fr, fw, fe;
int nMaxFd;
int nSocket;
int nArrClient[8];

using std::cout;
using std::endl;

void ProcessNewMessage(int nClientSocket)
{
    cout << endl
         << "Processing the message for client socket " << nClientSocket;

    char buff[257] = {0};
    int nRet = recv(nClientSocket, buff, 256, 0);

    if (nRet > 0)
    {
        buff[nRet] = '\0';
        cout << endl
             << "Received: " << buff;

        send(nClientSocket, "Processed your request", 23, 0);

        cout << endl
             << "~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~";
    }
    else if (nRet == 0)
    {
        cout << endl
             << "Client disconnected.";
        closesocket(nClientSocket);

        for (int nIndex = 0; nIndex < 8; nIndex++)
        {
            if (nArrClient[nIndex] == nClientSocket)
            {
                nArrClient[nIndex] = 0;
                break;
            }
        }
    }
    else
    {
        cout << endl
             << "Error on recv(). Closing connection.";
        closesocket(nClientSocket);

        for (int nIndex = 0; nIndex < 8; nIndex++)
        {
            if (nArrClient[nIndex] == nClientSocket)
            {
                nArrClient[nIndex] = 0;
                break;
            }
        }
    }
}

void ProcessTheNewRequest()
{
    if (FD_ISSET(nSocket, &fr))
    {
        int nLen = sizeof(struct sockaddr);
        int nClientSocket = accept(nSocket, NULL, &nLen);
        if (nClientSocket > nMaxFd)
        {
            nMaxFd = nClientSocket;
        }
        if (nClientSocket > 0)
        {
            // put it into the client fd set
            int nIndex = 0;

            // move this before accepting client so that it takes less resources
            for (nIndex = 0; nIndex < 8; nIndex++)
            {
                if (nArrClient[nIndex] == 0)
                {
                    nArrClient[nIndex] = nClientSocket;
                    send(nClientSocket, "Connection success", 19, 0);
                    cout << endl
                         << "message sent";
                    break;
                }
            }
            if (nIndex == 8)
            {
                cout << endl
                     << "No space for a new connection";
            }
        }
    }
    for (int nIndex = 0; nIndex < 8; nIndex++)
    {
        if (FD_ISSET(nArrClient[nIndex], &fr))
        {
            ProcessNewMessage(nArrClient[nIndex]);
            // got the new message from the client
            // recieved new message
            // queue for new worker of server to fulfill
        }
    }
}

int main()
{

    // network return

    int nRet = 0;

    // WSA

    WSADATA ws;
    if (WSAStartup(MAKEWORD(2, 2), &ws) < 0)
    {
        std::cerr << endl
                  << "WSA failed to initialize";
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // open socket

    nSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (nSocket < 0)
    {
        std::cerr << endl
                  << "Socket failed to open";
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT);
    srv.sin_addr.s_addr = INADDR_ANY;
    memset(&(srv.sin_zero), 0, 8);

    // io control socket

    u_long optval = 0;
    nRet = ioctlsocket(nSocket, FIONBIO, &optval);
    if (nRet != 0)
    {
        cout << endl
             << "ioctlsocket call failed";
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // set sock option

    int nOptVal = 0;
    int nOptLen = sizeof(nOptVal);
    nRet = setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&nOptVal, nOptLen);
    if (nRet != 0)
    {
        cout << endl
             << "The setsockopt call failed";
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // bind

    nRet = bind(nSocket, (sockaddr *)&srv, sizeof(sockaddr));
    if (nRet < 0)
    {
        std::cerr << endl
                  << "Failed to bind to local port";
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    // listen

    nRet = listen(nSocket, 8);
    if (nRet < 0)
    {
        std::cerr << endl
                  << "Failed to start listening to local port";
        WSACleanup();
        exit(EXIT_FAILURE);
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

        for (int nIndex = 0; nIndex < 8; nIndex++)
        {
            if (nArrClient[nIndex] != 0)
            {
                FD_SET(nArrClient[nIndex], &fr);
                FD_SET(nArrClient[nIndex], &fe);
            }
        }

        nRet = select(nMaxFd + 1, &fr, &fw, &fe, &tv);
        if (nRet > 0)
        {
            ProcessTheNewRequest();
        }
        else if (nRet != 0)
        {
            cout << "Socket failure";
            WSACleanup();
            exit(EXIT_FAILURE);
        }
    }

    WSACleanup();
    return 0;
}