#include<iostream>
#include <string.h>  
#include<string>  
#include <stdlib.h>  
#include <errno.h>  
#include <unistd.h>   
#include <arpa/inet.h> 
#include <sys/types.h>  
#include <sys/socket.h>  
#include <netinet/in.h>  
#include <sys/time.h>
#include<fstream>

using namespace std;

#define MAX 1024*1024
#define ACTIVE 100
#define INACTIVE 102
#define ONHOLD 101

typedef sockaddr SA;
const int max_clients = 30;
int PORT = 5000;
int client_sockets[max_clients], client_status[max_clients];
string client_names[max_clients];
int master_socket;
string filepath = "./files/";

void sendtoallexcept(int new_client_socket, string message)
{
    for(int i = 0; i < max_clients; i++)
    {
        if(client_sockets[i] != new_client_socket && client_sockets[i] != 0 && client_status[i] == ACTIVE)
        {
            send(client_sockets[i], message.c_str(), message.length(), 0);
        }
    }
}

void sendtoall(int socket_index, string message)
{
    for(int i = 0; i < max_clients; i++)
    {
        if(client_sockets[i] == 0)
        {
            send(client_sockets[i], message.c_str(), message.length(), 0);
        }
    }
}

void getfile(int client_socket_id, string filename)
{
    char buffer[MAX];
    bzero(buffer, sizeof(buffer));
    read(client_socket_id, buffer, sizeof(buffer));

    string data = "";
    int n = 0;
    while(buffer[n] != '\0' && n < MAX)
        data = data + buffer[n++];

    ofstream file;
    file.open(filepath + filename, ios::out);
    if(file)
    {
        file<<data;
    }
    else
    {
        cout<<"some error occurred\n";
    }
}

void givefile(int client_socket_id, string filename)
{
    ifstream file;
    file.open(filepath + filename, ios::in);
    string data, tmp;
    if(file)
    {
        while(getline(file, tmp))
        {
            data.append(tmp);
            data = data + "\n";
        }
        
        data = data + '\0';
        write(client_socket_id, data.c_str(), data.length());
    }
    else
    {
        cout<<"file not found\n";
        string file_not_found_message = "Requested file not found\n";
        write(client_socket_id, file_not_found_message.c_str(), file_not_found_message.length());
    }
}

int main()
{
    int opt = true;
    int addrlen;
    int max_sd, sd, activity, new_socket;
    sockaddr_in master_socket_address, client_socket_address;
    char buffer[MAX];
    string welcome_message = "Welcome to the chat room !!!\n";
    string enter_name_message = "Enter name ....";

    fd_set socket_set;

    for(int i = 0; i < max_clients; i++)
    {
        client_sockets[i] = 0;
        client_status[i] = INACTIVE;
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
            if(client_status[i] != INACTIVE)
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
                if(client_status[i] == INACTIVE)
                {
                    client_sockets[i] = new_socket;
                    client_status[i] = ONHOLD;
                    break;
                }
            }
            getpeername(new_socket, (SA*)&client_socket_address, (socklen_t*)&addrlen);
            send(new_socket, enter_name_message.c_str(), enter_name_message.length(), 0);
        }


        for(int i = 0; i < max_clients; i++)
        {
            sd = client_sockets[i];
            if(FD_ISSET(sd, &socket_set))
            {
                bzero(buffer, sizeof(buffer));
                int length_read = read(sd, buffer, MAX);
                if(length_read <= 0)
                {
                    sendtoallexcept(client_sockets[i], client_names[i] + " disconnected");
                    close(client_sockets[i]);
                    client_sockets[i] = 0;
                    client_status[i] = INACTIVE;
                }
                else if(client_status[i] == ACTIVE)
                {
                    string message = "";
                    int n = 0;
                    while(buffer[n] != '\0')
                        message = message + buffer[n++];

                    // cout<<"message : " << message<<endl;

                    if(message.find("upload") != string::npos)
                    {
                        getfile(client_sockets[i], message.substr(7));
                    }
                    else if(message.find("download") != string::npos)
                    {
                        givefile(client_sockets[i], message.substr(9));
                    }
                    if(message.length() > 0)
                        sendtoallexcept(sd, client_names[i] + " : " + message);
                }
                else if(client_status[i] == ONHOLD)
                {
                    string name = "";
                    int n = 0;
                    while(buffer[n] != '\0')
                        name = name + buffer[n++];

                    client_names[i] = name;
                    sendtoallexcept(client_sockets[i], name + " just joined the chat room");
                    client_status[i] = ACTIVE;
                    send(client_sockets[i], welcome_message.c_str(), welcome_message.length(), 0);
                }
            }
        }
    }

    return 0;
}