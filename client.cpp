#include <netdb.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <iostream>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include<thread>

using namespace std;

#define MAX 1024
#define SA struct sockaddr

int PORT = 5000;
string ipaddr = "127.0.0.1";

void read_thread_func(int socket_id)
{
    while(true)
    {
        char buffer[MAX];
        int n;
        bzero(buffer, sizeof(buffer));
        int len = read(socket_id, buffer, MAX);
        if (len > 0)
        {
            n = 0;
            while (buffer[n] != '\0')
                cout << buffer[n++];
            cout << endl;
        }
    }
}

void func(int socket_id)
{
    char buffer[MAX];
    int n;
    while (true)
    {
        // if(cin.get())
        // {
            string s;
            getline(cin, s);
            bzero(buffer, sizeof(buffer));
            n = 0;
            for (int i = 0; i < s.length(); i++)
                buffer[n++] = s[i];
            buffer[n] = '\0';
            if (strncmp(buffer, "quit", 4) == 0)
                break;
            write(socket_id, buffer, sizeof(buffer));
        // }
    }
}

int main()
{
    int socket_id, connection_id;
    sockaddr_in server_addr, client_addr;

    socket_id = socket(AF_INET, SOCK_STREAM, 0);
    if (socket_id == -1)
    {
        cout << "Socket creation failed\n";
        exit(0);
    }

    bzero(&server_addr, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(ipaddr.c_str());
    server_addr.sin_port = htons(PORT);

    if (connect(socket_id, (SA *)&server_addr, sizeof(server_addr)) != 0)
    {
        cout << "cant connect with server\n";
        exit(0);
    }

    thread read_thread(read_thread_func, socket_id);
    
    func(socket_id);
    close(socket_id);
    return 0;
}