#include <winsock.h>
#include <iostream>

using std::cout;
using std::endl;

#define PORT 9909

int nClientSocket;
struct sockaddr_in srv;

int main()
{
    int nRet = 0;

    // wsa

    WSADATA ws;
    if (WSAStartup(MAKEWORD(2, 2), &ws) < 0)
    {
        std::cerr << endl
                  << "WSA failed to initialize";
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    nClientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (nClientSocket < 0)
    {
        cout << endl
             << "socket call failed";
        WSACleanup();
        exit(EXIT_FAILURE);
    }

    srv.sin_family = AF_INET;
    srv.sin_port = htons(PORT);
    srv.sin_addr.s_addr = inet_addr("127.0.0.1");
    memset(&(srv.sin_zero), 0, 8);

    nRet = connect(nClientSocket, (struct sockaddr *)&srv, sizeof(srv));
    if (nRet < 0)
    {
        cout << endl
             << "connection failed";
        WSACleanup();
        exit(EXIT_FAILURE);
    }
    else
    {
        cout << endl
             << "connected to server";
    }

    char buff[255] = {
        0,
    };
    recv(nClientSocket, buff, 255, 0);

    cout << endl
         << buff;

    return 0;
}