#include<iostream>
#include <string.h>   //strlen
#include<string>  
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>   //close  
#include <arpa/inet.h>    //close  
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h>
using namespace std;

#define MAX 1024
typedef sockaddr SA;
const int max_clients = 30;
int PORT = 5000;
int client_sockets[max_clients], client_number[max_clients], client_group[max_clients];
int master_socket;

void sendtoallexcept(int new_client_socket, string message)
{
    for(int i = 0; i < max_clients; i++)
    {
        if(client_sockets[i] != new_client_socket)
        {
            send(client_sockets[i], message.c_str(), message.length(), 0);
        }
    }
}

int main()
{
    int opt = true;
    int addrlen;
    int max_sd, sd, activity, new_socket;
    sockaddr_in master_socket_address, client_socket_address;
    char buffer[MAX];
    string welcome_message = "Welcome to the chat room !!!";

    fd_set socket_set;

    for(int i = 0; i < max_clients; i++)
    {
        client_sockets[i] = 0;
        client_number[i] = -1;
        client_group[i] = -1;
    }

    if((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Server socket creation failed");
        exit(EXIT_FAILURE);
    }

    if( setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt,  
          sizeof(opt)) < 0 )   
    {   
        perror("setsockopt");   
        exit(EXIT_FAILURE);   
    }

    master_socket_address.sin_family = AF_INET;
    master_socket_address.sin_addr.s_addr = INADDR_ANY;
    master_socket_address.sin_port = htons(PORT);

    if(bind(master_socket, (SA*)&master_socket_address, sizeof(master_socket_address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if(listen(master_socket, 3) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(master_socket_address);
    cout<<"Server started succesfully. Waiting for clients ....\n";

    while(true)
    {
        bzero(buffer, sizeof(buffer));
        FD_ZERO(&socket_set);

        FD_SET(master_socket, &socket_set);
        max_sd = master_socket;

        for(int i = 0; i < max_clients; i++)
        {
            sd = client_sockets[i];
            if(sd > 0)
                FD_SET(sd, &socket_set);

            max_sd = max(max_sd, sd);
        }

        activity = select( max_sd + 1, &socket_set, NULL, NULL, NULL);

        if(activity < 0 && errno != EINTR)
        {
            cout<<"Unexpected error occurred\n";
        }

        if(FD_ISSET(master_socket, &socket_set))
        {
            if((new_socket = accept(master_socket, (SA*)&master_socket_address, (socklen_t*)&addrlen)) < 0)
            {
                perror("connection request by client failed");
                exit(EXIT_FAILURE);
            }
            for(int i = 0; i < max_clients; i++)
            {
                if(client_sockets[i] == 0)
                {
                    client_sockets[i] = new_socket;
                    break;
                }
            }
            send(new_socket, welcome_message.c_str(), welcome_message.length(), 0);
            getpeername(new_socket, (SA*)&client_socket_address, (socklen_t*)&addrlen);
            sendtoallexcept(new_socket, "New member with ip address : " + string(inet_ntoa(client_socket_address.sin_addr)) + " has joined the char room");
        }


        for(int i = 0; i < max_clients; i++)
        {
            sd = client_sockets[i];
            if(FD_ISSET(sd, &socket_set))
            {
                bzero(buffer, sizeof(buffer));
                int length_read = read(sd, buffer, MAX);
                if(length_read == 0)
                {
                    sendtoallexcept(sd, "client with ip address : " + string(inet_ntoa(client_socket_address.sin_addr)) + " disconnected");
                }
            }
            else
            {
                
            }
        }
    }

    return 0;
}