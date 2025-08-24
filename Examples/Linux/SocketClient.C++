#include <iostream>
#include <cstring>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

using std::cin;
using std::cout;
using std::endl;
using std::string;

#define PORT 9909

int main()
{

    int nServerSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (nServerSocket < 0)
    {
        cout << "Socket creation failed" << endl;
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
        close(nServerSocket);
        return 1;
    }

    cout << "Connected to server!" << endl;

    // Main send/receive loop
    while (true)
    {
        string userInput;
        cout << "Enter message (or 'quit' to exit): ";
        std::getline(cin, userInput);

        if (userInput == "quit")
            break;

        // Send message to server
        if (send(nServerSocket, userInput.c_str(), userInput.size(), 0) < 0)
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

    close(nServerSocket);
    return 0;
}
