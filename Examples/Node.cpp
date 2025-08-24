#include <winsock.h>
#include <iostream>
#include <string>

using std::cin;
using std::cout;
using std::endl;
using std::string;

#define PORT 9909

int main()
{
    WSADATA ws;
    if (WSAStartup(MAKEWORD(2, 2), &ws) != 0)
    {
        std::cerr << "WSA failed to initialize" << endl;
        return 1;
    }

    int nServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (nServerSocket < 0)
    {
        cout << "Socket creation failed" << endl;
        WSACleanup();
        return 1;
    }

    struct sockaddr_in srv;
    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT);
    srv.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(&(srv.sin_zero), 0, 8);

    if (connect(nServerSocket, (struct sockaddr *)&srv, sizeof(srv)) < 0)
    {
        cout << "Connection failed" << endl;
        closesocket(nServerSocket);
        WSACleanup();
        return 1;
    }

    cout << "Connected to server!" << endl;

    // Main send/receive loop
    while (true)
    {
        string input;

        if (send(nServerSocket, input.c_str(), input.size(), 0) < 0)
        {
            cout << "Send failed" << endl;
            break;
        }

        // Receive reply
        char buff[257] = {0};
        int nRet = recv(nServerSocket, buff, 256, 0);
        if (nRet > 0)
        {
            buff[nRet] = '\0';
            cout << "Server: " << buff << endl;
        }
        else if (nRet == 0)
        {
            cout << "Server closed connection" << endl;
            break;
        }
        else
        {
            cout << "Receive error" << endl;
            break;
        }
    }

    closesocket(nServerSocket);
    WSACleanup();
    return 0;
}
