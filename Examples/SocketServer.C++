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

void ProcessTheNewRequest()
{
    if (FD_ISSET(nSocket, &fr))
    {
        int nLen = sizeof(struct sockaddr);
        int nClientSocket = accept(nSocket, NULL, &nLen);
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
    else
    {
        for (int nIndex = 0; nIndex < 5; nIndex++)
        {
            if (FD_ISSET(nArrClient[nIndex], &fr))
            {
                // got the new message from the client
                // recieved new message
                // queue for new worker of server to fulfill
            }
        }
    }
}

int main()
{
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
    else
    {
        cout << endl
             << "WSA initialize";
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
    else
    {
        cout << endl
             << "Socket opened at 0x"
             << nSocket;
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
    else
    {
        cout << endl
             << "ioctlsocket call passed";
    }

    // set sock option

    int nOptVal = 0;
    int nOptLen = sizeof(nOptVal);
    nRet = setsockopt(nSocket, SOL_SOCKET, SO_REUSEADDR, (const char *)&nOptVal, nOptLen);
    if (!nRet)
    {
        cout << endl
             << "The setsockopt call successful";
    }
    else
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
    else
    {
        cout << endl
             << "Successfully binded to local port";
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
    else
    {
        cout << endl
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

        for (int nIndex = 0; nIndex < 8; nIndex++)
        {
            if (nArrClient[nIndex] != 0)
            {
                FD_SET(nArrClient[nIndex], &fr);
                FD_SET(nArrClient[nIndex], &fe);
            }
        }

        cout << endl
             << "Before select calls: " << fr.fd_count << endl;
        nRet = select(nMaxFd + 1, &fr, &fw, &fe, &tv);
        if (nRet > 0)
        {
            cout << nRet << " on port: " << PORT;
            if (FD_ISSET(nSocket, &fe))
            {
                cout << endl
                     << "there is an exception";
            }

            if (FD_ISSET(nSocket, &fw))
            {
                cout << endl
                     << "ready to write something";
            }

            if (FD_ISSET(nSocket, &fr))
            {
                cout << endl
                     << "ready to read";
                ProcessTheNewRequest();
            }
        }
        else if (nRet == 0)
        {
            cout << "Nothing on port: " << PORT;
        }
        else
        {
            cout << "Socket failure";
            WSACleanup();
            exit(EXIT_FAILURE);
        }

        cout << endl
             << "After select calls: " << fr.fd_count << endl;
    }

    return 0;
}