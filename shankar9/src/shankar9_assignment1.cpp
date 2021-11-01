/**
 * @shankar9_assignment1
 * @author  Vivek Shankar <shankar9@buffalo.edu>
 * @version 1.0
 *
 * @section LICENSE
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details at
 * http://www.gnu.org/copyleft/gpl.html
 *
 * @section DESCRIPTION
 *
 * Took references from -
 * https://github.com/vaibhavchincholkar/CSE-489-589-Modern-Networking-Concepts/blob/master/cse489589_assignment1/vchincho/src/vchincho_assignment1.c
 */
#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <netdb.h>
#include <unistd.h>
#include <algorithm>

#include "../include/global.h"
#include "../include/logger.h"

#define DNS_IP "8.8.8.8"
#define DNS_PORT "53"
#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 100
#define BUFFER_SIZE 256
#define LOGGED_IN "logged-in"
#define LOGGED_OUT "logged-out"

using namespace std;
// "1" in server mode, "0" in client mode
bool mode;

// Socket used by the client
int client_socket;

void print_author_statement();

void print_ip_address();

void server(char *port);

void client(char *port);

int connect_to_host(char *server_ip, char *server_port);

bool socket_bind(int client_port);

bool isvalidIP(char *ip);

bool compare_list_data (const struct list_data *a, const struct list_data *b);

struct list_data
{
    int id;
    char host_name[40];
    char ip[32];
    int port;
    int socket;
    int rcv_msg_count;
    int snd_msg_count;
    char status[20];

};

struct list_data *list_data_ptr[5];

struct server_msg_model
{
    char cmd[20];
    char sender_ip[32];
    char data[256];
    struct list_data list_entry;
};

struct client_msg_model
{
    char cmd[20];
    char client_ip[32];
    char data[256];
};



/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv) {
    if (argc != 3) {
        cout << "Usage: [./assignment1] [mode] [port]" << endl;
        return -1;
    }
    /*Init. Logger*/
    cse4589_init_log(argv[2]);
    /* Clear LOGFILE*/
    fclose(fopen(LOGFILE, "w"));
    /*Start Here*/

    if (*argv[1] == 's') {
        // server mode
        mode = 1;
        server(argv[2]);
    } else if (*argv[1] == 'c') {
        // client mode
        mode = 0;
        client(argv[2]);
    } else {
        perror("Invalid mode!. Acceptable modes server(s) or client(c).");
        return -1;
    }



// lose the pesky "Address already in use" error message
//    if (setsockopt(listener,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes) == -1) {
//        perror("setsockopt");
//        exit(1);
//    }

    return 0;
}

void server(char *port) {

    int yes = 1; // For setsockopt
    int server_socket, head_socket, selret, sock_index, fdaccept = 0, caddr_len;
    struct server_msg_model server_msg;
    struct client_msg_model client_msg;
    struct sockaddr_in client_addr;
    struct addrinfo hints, *res;
    fd_set master_list, watch_list;

    /* Set up hints structure */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    /* Fill up address structures */
    if (getaddrinfo(NULL, port, &hints, &res) != 0)
        perror("getaddrinfo failed");

    /* Socket */
    server_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (server_socket < 0)
        perror("Cannot create socket");

    // Gets rid of socket in use error
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    /* Bind */
    if (::bind(server_socket, res->ai_addr, res->ai_addrlen) < 0)
        perror("Bind failed");

    freeaddrinfo(res);
    //cout<<"Can print here4!"<<endl;
    //cout<<"[PA1-Server@CSE489/589]$ ";
    //cout<<"[PA1-Server@CSE489/589]$ ";

    /* Listen */
    if (listen(server_socket, BACKLOG) < 0)
        perror("Unable to listen on port");

    // Init
    for(int i = 0; i < 5; i++) {
        list_data_ptr[i] = (struct list_data *) malloc(sizeof(struct list_data));
        list_data_ptr[i]->id = 0;
    }

    /* Zero select FD sets */
    FD_ZERO(&master_list);
    FD_ZERO(&watch_list);

    /* Register the listening socket */
    FD_SET(server_socket, &master_list);
    /* Register STDIN */
    FD_SET(STDIN, &master_list);

    head_socket = server_socket;
    string command_str;

    while (1) {
        memcpy(&watch_list, &master_list, sizeof(master_list));

        cout << "[PA1-Server@CSE489/589]$ ";
        cout.flush();
        /* select() system call. This will BLOCK */
        selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
        if (selret < 0)
            perror("select failed.");

        /* Check if we have sockets/STDIN to process */
        if (selret > 0) {
            /* Loop through socket descriptors to check which ones are ready */
            for (sock_index = 0; sock_index <= head_socket; sock_index += 1) {

                if (FD_ISSET(sock_index, &watch_list)) {

                    /* Check if new command on STDIN */
                    if (sock_index == STDIN) {
                        char *cmd = (char *) malloc(sizeof(char) * CMD_SIZE);
                        memset(cmd, '\0', CMD_SIZE);
                        if (fgets(cmd, CMD_SIZE - 1, stdin) == NULL)
                            exit(-1);
                        // Deal with the newline added to end of STDIN file
                        cmd[strcspn(cmd, "\n")] = 0;
                        // Need to use strtok() to parse the command and arguments separately
                        char *command = strtok(cmd, " ");
                        cout << "Command = " << command << endl;
                        cout.flush();

                        // Check command & invoke the apt method below
                        if (!strcmp(command, "AUTHOR")) {
                            print_author_statement();
                        } else if (!strcmp(command, "IP")) {
                            print_ip_address();
                        } else if (!strcmp(command, "PORT")) {
                            cse4589_print_and_log("[%s:SUCCESS]\n", command);
                            cse4589_print_and_log("PORT:%s\n", port);
                            cse4589_print_and_log("[%s:END]\n", command);
                        } else if (!strcmp(command, "LIST")) {
                            // TODO: Handle LIST command
                            int client_count = 0;
                            for (auto i: list_data_ptr) {
                                if (i->id == 0) {
                                    continue;
                                }
                                client_count++;
                            }
                            sort(list_data_ptr, list_data_ptr + client_count + 1, compare_list_data);

                            cse4589_print_and_log("[LIST:SUCCESS]\n");
                            int idx = 1;
                            for(auto i : list_data_ptr) {
                                if (i->id == 0 || strcmp(i->status, LOGGED_IN)) {
                                    continue;
                                }
                                cse4589_print_and_log ("%-5d%-35s%-20s%-8d\n", idx, i->host_name, i->ip, i->port);
                                idx ++;
                            }
                            cse4589_print_and_log("[LIST:END]\n");
                        } else if (!strcmp(command, "STATISTICS")) {
                            // TODO: Handle STATISTICS command
                        } else if (!strcmp(command, "BLOCKED")) {
                            // Need to figure out how to work with 2nd argument here maybe strtok()
                        } else {
                            // Unidentified command
                            cout << "Unidentified command, ignoring..." << endl;
                        }
                        free(cmd);
                    }
                    /* Check if new client is requesting connection */
                    else if (sock_index == server_socket) {
                        caddr_len = sizeof(client_addr);
                        fdaccept = accept(server_socket, (struct sockaddr *) &client_addr, (socklen_t * ) & caddr_len);
                        if (fdaccept < 0)
                            perror("Accept failed.");
                        char client_ip[INET_ADDRSTRLEN];
                        inet_ntop(AF_INET, &client_addr.sin_addr.s_addr, client_ip, INET_ADDRSTRLEN);

                        cout<<"Remote Host connected! With IP: "<<client_ip<<endl;
                        cout.flush();

                        /* Add to watched socket list */
                        FD_SET(fdaccept, &master_list);
                        if (fdaccept > head_socket) head_socket = fdaccept;

                        char client_host_name[1024];
                        getnameinfo((struct sockaddr *)&client_addr, caddr_len, client_host_name, sizeof(client_host_name), 0, 0, 0);
                        cout<<"Client host name : "<<client_host_name<<" port # : "<<ntohs(client_addr.sin_port)<<endl;
                        cout.flush();

                        // Initialize a list entry for this client
                        int client_count = 0;
                        for (auto i: list_data_ptr) {
                            if (i->id == 0) {
                                break;
                            }
                            client_count++;
                        }

                        list_data_ptr[client_count]->id = client_count + 1;
                        list_data_ptr[client_count]->port = ntohs(client_addr.sin_port);
                        list_data_ptr[client_count]->socket = fdaccept;
                        list_data_ptr[client_count]->snd_msg_count = 0;
                        list_data_ptr[client_count]->rcv_msg_count = 0;
                        strcpy(list_data_ptr[client_count]->status, LOGGED_IN);
                        strcpy(list_data_ptr[client_count]->ip, client_ip);
                        strcpy(list_data_ptr[client_count]->host_name, client_host_name);

                        // Sort the list by port number
                        sort(list_data_ptr, list_data_ptr + client_count + 1, compare_list_data);

                        strcpy(server_msg.cmd, "some_command");
                        if(send(fdaccept, &server_msg, sizeof(server_msg), 0) == sizeof(server_msg)) {
                            cout<<"Sent command to the client that just logged in."<<endl;
                            cout.flush();
                        }
                    }
                    /* Read from existing clients */
                    else {
                        /* Initialize buffer to receieve response */
//                        char *buffer = (char *) malloc(sizeof(char) * BUFFER_SIZE);
//                        memset(buffer, '\0', BUFFER_SIZE);
                        memset(&client_msg, '\0', sizeof (client_msg));

                        if (recv(sock_index, &client_msg, sizeof (client_msg), 0) <= 0) {
                            close(sock_index);
                            printf("Remote Host terminated connection!\n");

                            /* Remove from watched list */
                            FD_CLR(sock_index, &master_list);
                        } else {
                            //Process incoming data from existing clients here ...
                            if(!strcmp(client_msg.cmd, "LOGOUT")) {
                                for (auto i : list_data_ptr) {
                                    if (i->socket == sock_index) {
                                        strcpy(i->status, LOGGED_OUT);
                                        close(sock_index);
                                        FD_CLR(sock_index, &master_list);
                                        int client_count = 0;
                                        for (auto i: list_data_ptr) {
                                            if (i->id == 0) {
                                                break;
                                            }
                                            client_count++;
                                        }
                                        sort(list_data_ptr, list_data_ptr + client_count + 1, compare_list_data);
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

bool compare_list_data (const struct list_data *a, const struct list_data *b) {

    if (a->port < b->port)
        return 1;
    else
        return 0;
}

void client(char *port) {

    int logged_in = 0, client_sock_index, client_head_socket=0;
    fd_set client_master_list, client_watch_list;
   
    //cout<<"Inside client"<<endl;
    //cout.flush();

    // Bind port since the client will listen to server messages here
    if(!socket_bind(atoi(port)))
        exit(-1);
    //cout<<"Socket bind done!"<<endl;
    //cout.flush();
    // Socket server talks to
    int server_sock = 0;
    int selret;
    struct client_msg_model client_msg;

    FD_ZERO(&client_master_list);//Initializes the file descriptor set fdset to have zero bits for all file descriptors.
    FD_ZERO(&client_watch_list);
    FD_SET(STDIN, &client_master_list);
    while (1) {
        
        FD_ZERO(&client_master_list);
        FD_ZERO(&client_watch_list);

        FD_SET(STDIN, &client_master_list);
        FD_SET(server_sock, &client_master_list);
        client_head_socket = server_sock;

        memcpy(&client_watch_list, &client_master_list, sizeof(client_master_list));

        cout << "[PA1-Client@CSE489/589]$ ";
        cout.flush();

        /* select() system call. This will BLOCK */
        selret = select(client_head_socket + 1, &client_watch_list, NULL, NULL, NULL);
        //cout<<"select returned: "<<selret<<endl;
        //cout.flush();
        if(selret < 0)
        {
            perror("select failed.");
            exit(-1);
        }
        //cout<<"Select init done!"<<endl;
        //cout.flush();
        if(selret > 0) {
            /* Loop through socket descriptors to check which ones are ready */
            for (client_sock_index = 0; client_sock_index <= client_head_socket; client_sock_index += 1) {
                if (FD_ISSET(client_sock_index, &client_watch_list)) {

                    if (client_sock_index == STDIN) {
                        char *cmd = (char *) malloc(sizeof(char) * CMD_SIZE), *saved_context;
                        memset(cmd, '\0', CMD_SIZE);
                        if (fgets(cmd, CMD_SIZE - 1, stdin) ==
                            NULL) //Mind the newline character that will be written to cmd
                            exit(-1);
                        // Deal with the newline added to end of STDIN file
                        cmd[strcspn(cmd, "\n")] = 0;
                        // Need to use strtok() to parse the command and arguments separately
                        char *command = strtok_r(cmd, " ", &saved_context);
                        //cout << "Command = " << command << endl;
                        //cout.flush();

                        if (!strcmp(command, "AUTHOR")) {
                            print_author_statement();
                        } else if (!strcmp(command, "IP")) {
                            print_ip_address();
                        } else if (!strcmp(command, "PORT")) {
                            cse4589_print_and_log("[%s:SUCCESS]\n", command);
                            cse4589_print_and_log("PORT:%s\n", port);
                            cse4589_print_and_log("[%s:END]\n", command);
                        } else if (!strcmp(command, "LIST")) {

                        } else if (!strcmp(command, "LOGIN")) {
                            bool error = 0;
                            // Need to extract just the first string from command and parse the other 2 here
                            char *server_ip = strtok_r(NULL, " ", &saved_context);
                            char *server_port = strtok_r(NULL, " ", &saved_context);
                            
                            //cout<<"Server IP: "<<server_ip<<"Server Port: "<<server_port<<endl;
                            //cout.flush(); 

                            if (server_ip == NULL || server_port == NULL) {
                              cout<<"Incorrect Usage: LOGIN [server IP] [server port]"<<endl;
                              continue;
                            }
                            // Validate port & IP
                            size_t length = strlen(server_port);
                            for (size_t i = 0; i < length; i++) {
                                if(!isdigit(server_port[i])) {
                                    error = 1;
                                }
                            }
                            if (error) {
                                perror("Invalid port, contains non-digits. Try again.");
                                cse4589_print_and_log("[LOGIN:ERROR]\n");
                                cse4589_print_and_log("[LOGIN:END]\n");
                                continue;
                            }
                            // Is valid range of ports >
                            if (atoi(server_port) < 1 || atoi(server_port) > 65535) {
                                error = 1;
                            }
                            if (error) {
                                perror("Invalid port, out of acceptable range [1 - 65535]. Try again.");
                                cse4589_print_and_log("[LOGIN:ERROR]\n");
                                cse4589_print_and_log("[LOGIN:END]\n");
                                continue;
                            }
                            if (!isvalidIP(server_ip)) {
                                perror("Invalid IP!");
                                cse4589_print_and_log("[LOGIN:ERROR]\n");
                                cse4589_print_and_log("[LOGIN:END]\n");
                                continue;
                            }
                            // Connect to server and now the client socket is where the server would send messages,
                            // so assign client_socket to server so we can use select() on it.
                            server_sock = connect_to_host(server_ip, server_port);
                            if (server_sock == -1) {
                                perror("Failed to connect to server, verify details.");
                                continue;
                            }

                            FD_SET(server_sock, &client_master_list);
                            client_head_socket=server_sock;
                            logged_in=1;
                            cse4589_print_and_log("[LOGIN:SUCCESS]\n");

                        } else if (!strcmp(command, "REFRESH")) {

                        } else if (!strcmp(command, "SEND")) {

                        } else if (!strcmp(command, "BROADCAST")) {

                        } else if (!strcmp(command, "BLOCK")) {

                        } else if (!strcmp(command, "UNBLOCK")) {

                        } else if (!strcmp(command, "LOGOUT")) {
                            if (!logged_in) {
                                cout<<"Not logged in!"<<endl;
                                continue;
                            }
                            // Tell server we're logging out.
                            strcpy(client_msg.cmd,"LOGOUT");
                            if (send(server_sock, &client_msg, sizeof (client_msg), 0) == sizeof (client_msg)) {
                                cse4589_print_and_log("[LOGOUT:SUCCESS]\n");
                                logged_in = 0;
                                server_sock=close(server_sock);
                            }
                            cse4589_print_and_log("[LOGOUT:END]\n");
                        } else if (!strcmp(command, "EXIT")) {
                            // Logout if logged-in
                            exit(0);
                        } else {
                            // Unidentified command
                            cout << "Unidentified command, ignoring..." << endl;
                        }
                    } else {
                        // Parse and do something with msg received from server
                        struct server_msg_model msg_rcvd;
                        memset(&msg_rcvd, '\0', sizeof(msg_rcvd));

                        if (recv(server_sock, &msg_rcvd, sizeof(msg_rcvd), 0) >= 0) {

                            if(!strcmp (msg_rcvd.cmd, "some_command")) {
                                cse4589_print_and_log("[LOGIN:END]\n");
                            }
                        }
                    }
                }
            }
        }
    }
}

void print_author_statement() {
    char author[] = "shankar9";
    char command_str[] = "AUTHOR";
    cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
    cse4589_print_and_log("I, %s, have read and understood the course academic integrity policy.\n", author);
    cse4589_print_and_log("[%s:END]\n", command_str);
}

void print_ip_address() {
    struct addrinfo hints, *res;
    int dum_socket;
    struct sockaddr_in _self;
    int sa_len = sizeof(_self);
    char command_str[] = "IP";
    bool success = 1;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;

    /* Fill up address structures */
    if (getaddrinfo(DNS_IP, DNS_PORT, &hints, &res) != 0) {
        perror("getaddrinfo failed");
        success = 0;
    }

    /* Socket */
    dum_socket = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (dum_socket < 0) {
        perror("Failed to create socket");
        success = 0;
    }
    if (connect(dum_socket, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Connect failed");
        success = 0;
    }

    memset(&_self, 42, sa_len);
    if (getsockname(dum_socket, (struct sockaddr *) &_self, (socklen_t * ) & sa_len) == -1) {
        perror("getsockname() failed");
        success = 0;
    }

    if (success) {
        cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
        cse4589_print_and_log("IP:%s\n", inet_ntoa(_self.sin_addr));
        cse4589_print_and_log("[%s:END]\n", command_str);
    } else {
        cse4589_print_and_log("[%s:ERROR]\n", command_str);
        cse4589_print_and_log("[%s:END]\n", command_str);
    }
}

bool socket_bind(int client_port) {
    struct sockaddr_in client_addr;
    client_socket = socket(AF_INET, SOCK_STREAM, 0);
    if(client_socket < 0)
    {
        perror("Failed to create socket");
        return 0;
    }
    //cout<<"In bind, socket: "<<client_socket<<endl;
    // setting up the client socket
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(client_port);
    int val=1;
    setsockopt(client_socket, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val));

    if(::bind(client_socket, (struct  sockaddr*) &client_addr, sizeof(struct sockaddr_in)) == 0)
    {
        perror("Client bound to the port successfully\n");
        return 1;
    }
    else
    {
        perror("Error in binding to client port\n");
        return 0;
    }
}

int connect_to_host(char *server_ip, char *server_port) {
    struct addrinfo hints, *res;
    bool error = 0;

    /* Set up hints structure */
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

    /* Fill up address structures */
    if (getaddrinfo(server_ip, server_port, &hints, &res) != 0) {
        perror("getaddrinfo failed");
        error = 1;
    }

    /* Connect */
    cout<<"Trying to connect..."<<endl;
    cout.flush();
    if (connect(client_socket, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Connect failed");
        error = 1;
    }

    freeaddrinfo(res);

    return error ? -1 : client_socket;
}

bool isvalidIP(char *ip) {
    struct sockaddr_in addr;
    int val = inet_pton(AF_INET, ip, &addr.sin_addr);
    if (val == 1)
        return val;
    else
        return 0;
}
