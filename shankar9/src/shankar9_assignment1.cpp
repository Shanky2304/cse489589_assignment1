/**
 * @shankar9_assignment1
 * @author  Vivek Shankar <shankar9@buffalo.edu>,
 * @author  William Hiltz <wrhiltz@buffalo.edu>,
 * @author  Yutian Yan <yutianya@buffalo.edu>
 * @version 1.1
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
 * Took references/code snippets from - https://beej.us/guide/bgnet/html/
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
    char buffer[2561];
};

struct list_data *list_data_ptr[5];

struct server_msg_model
{
    char cmd[20];
    char sender_ip[32];
    char data[256];
    struct list_data list_entries[5];
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

    // Is valid range of ports >
    if (atoi(argv[2]) < 1 || atoi(argv[2]) > 65535) {
        perror("Invalid port, out of acceptable range [1 - 65535].");
        exit(-1);
    }

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
        list_data_ptr[i]->snd_msg_count = 0;
        list_data_ptr[i]->rcv_msg_count = 0;
    }

    //init for block client list
    // for(int i = 0; i < 5; i++) {
    //     block_ptr[i] = (struct block_list_client *) malloc(sizeof(struct block_list_client));
    //     block_ptr[i]-> blocked_client1_ip  = "empty";
    //     block_ptr[i]-> blocked_client2_ip  = "empty";
    //     block_ptr[i]-> blocked_client3_ip  = "empty";
    //     block_ptr[i]-> blocked_client4_ip  = "empty";
    // }

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
                        // cout << "Command = " << command << endl;
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
                                fflush(stdout);
                            }
                            cse4589_print_and_log("[LIST:END]\n");
                            fflush(stdout);
                        } else if (!strcmp(command, "STATISTICS")) {
                            cse4589_print_and_log("[STATISTICS:SUCCESS]\n");
                            int idx = 1;
                            for(auto i : list_data_ptr) {
                                if (i->id == 0) {
                                    continue;
                                }
                                cse4589_print_and_log("%-5d%-35s%-8d%-8d%-8s\n",
                                                      idx, i->host_name, i->snd_msg_count, i->rcv_msg_count, i->status);
                                idx ++;
                            }
                            cse4589_print_and_log("[STATISTICS:END]\n");
                        } else if (!strcmp(command, "BLOCKED")) {
                        //COMMENTED OUT ENTIRE BLOCK OF CODE FOR STABILITY, BECAUSE IT COULD INCORRECTLY EFFECT STATISTICS OF MESSAGES SENT/RECEIVED  
                             // Need to figure out how to work with 2nd argument here maybe strtok()
                        //     int blocked_client_count = 0;
                        //     for (auto i: list_data_ptr) {
                        //         if (i-> id == 0) {
                        //             continue;
                        //         }
                        //         blocked_client_count++;
                        //     }
                        //     sort(list_data_ptr, list_data_ptr + blocked_client_count + 1, compare_list_data);

                        //     cse4589_print_and_log("[BLOCKED:SUCCESS]\n");
                        //     int idx = 1;
                        //     for(auto i : list_data_ptr) {
                        //         if (i-> list_data_ptr == 0) {
                        //             continue;
                        //         }
                        //         cse4589_print_and_log ("%-5d%-35s%-20s%-8d\n", idx, i->host_name, i->ip, i->port);
                        //         idx ++;
                        //         fflush(stdout);
                        //     }
                        //     cse4589_print_and_log("[BLOCKED:END]\n");
                        //     fflush(stdout);
                        // } else {
                        //     // Unidentified command
                        //     cout << "Unidentified command, ignoring..." << endl;
                        // }
                        // free(cmd);
                        } else {
                            // Unidentified command
                            cout << "Unidentified command, ignoring..." << endl;
                        }
                        free(cmd);
                    }
                    /* Check if new client is requesting connection */
                    else if (sock_index == server_socket) {
                        bool re_logon = 0;
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
                            if (!strcmp(i->ip, client_ip)) {
                                re_logon = 1;
                                break;
                            }
                            client_count++;
                        }
                        if (!re_logon) {
                            client_count = 0;
                            for (auto i: list_data_ptr) {
                                if (i->id == 0) {
                                    break;
                                }
                                client_count++;
                            }
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

                        strcpy(server_msg.cmd, "login_list");
                        int counter = 0;
                        for (auto i : list_data_ptr) {
                            server_msg.list_entries[counter] = *i;
                            counter++;
                        }
                        if(send(fdaccept, &server_msg, sizeof(server_msg), 0) == sizeof(server_msg)) {
                            cout<<"Sent login_list to the client that just logged in."<<endl;
                            cout.flush();
                        }
                    }
                    /* Read from existing clients */
                    else {
                        /* Initialize buffer to receieve response */
                        memset(&client_msg, '\0', sizeof (client_msg));

                        if (recv(sock_index, &client_msg, sizeof (client_msg), 0) <= 0) {
                            close(sock_index);
                            printf("Remote Host terminated connection!\n");

                            // Update logged out status
                            for (auto i : list_data_ptr) {
                                if (i->socket == sock_index) {
                                    strcpy(i->status, LOGGED_OUT);
                                    break;
                                }
                            }

                            /* Remove from watched list */
                            FD_CLR(sock_index, &master_list);
                        } else {
                            //Process incoming data from existing clients here ...
                            if(!strcmp(client_msg.cmd, "logout")) {
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
                            } else if (!strcmp(client_msg.cmd, "refresh")) {
                                // Send list to the client
                                strcpy(server_msg.cmd, "refresh_data");
                                int counter = 0;
                                for (auto i : list_data_ptr) {
                                    server_msg.list_entries[counter] = *i;
                                    counter++;
                                }
                                if(send(sock_index, &server_msg, sizeof(server_msg), 0) == sizeof(server_msg)) {
                                    cout<<"Sent refresh_data to the client."<<endl;
                                    cout.flush();
                                }
                            } else if (!strcmp(client_msg.cmd, "send")) {
                                bool found_ip = 0, target_client_logged_out = 0;
                                char sender_client_ip[32];
                                // Who's thr sender?
                                for (auto k: list_data_ptr) {
                                    if (k->socket == sock_index) {
                                        strcpy(sender_client_ip, k->ip);
                                        k->snd_msg_count++;
                                    }
                                }
                                // Who's the receiver?
                                for (auto i: list_data_ptr) {
                                    if (!strcmp(i->ip, client_msg.client_ip)) {
                                        found_ip = 1;

                                        i->rcv_msg_count++;
                                        if (!strcmp(i->status, LOGGED_OUT)) {
                                            strcat(i->buffer, client_msg.data);
                                            target_client_logged_out = 1;
                                            break;
                                        } else {
                                            strcpy(server_msg.cmd, "relayed_msg");
                                            strcpy(server_msg.sender_ip, sender_client_ip);
                                            strcpy(server_msg.data, client_msg.data);
                                            if (send(i->socket, &server_msg, sizeof(server_msg), 0) ==
                                                sizeof(server_msg)) {
                                                cse4589_print_and_log("[RELAYED:SUCCESS]\n");
                                                cse4589_print_and_log("msg from:%s, to:%s\n[msg]:%s\n",
                                                                      sender_client_ip, i->ip, server_msg.data);
                                                cse4589_print_and_log("[RELAYED:END]\n");
                                                strcpy (client_msg.cmd, "send_success");
                                                fflush(stdout);
                                                // Maybe we should do some error-handling here as well
                                                if (send (sock_index, &client_msg, sizeof (client_msg), 0) ==
                                                sizeof (client_msg)) {
                                                    break;
                                                }
                                            }
                                            cse4589_print_and_log("[RELAYED:ERROR]\n");
                                            cse4589_print_and_log("[RELAYED:END]\n");
                                            strcpy (client_msg.cmd, "send_failure");
                                            // Maybe we should do some error-handling here as well
                                            send (sock_index, &client_msg, sizeof (client_msg), 0);
                                            break;
                                        }
                                    }
                                }
                                if (!found_ip) {
                                    cout<<"Couldn't find a client with IP - "<<client_msg.client_ip<<endl;
                                    cse4589_print_and_log("[RELAYED:ERROR]\n");
                                    cse4589_print_and_log("[RELAYED:END]\n");
                                    strcpy (client_msg.cmd, "send_failure");
                                    // Maybe we should do some error-handling here as well
                                    send (sock_index, &client_msg, sizeof (client_msg), 0);
                                } else if (target_client_logged_out) {
                                    cse4589_print_and_log("[RELAYED:ERROR]\n");
                                    cout<<"The target client with IP "<<client_msg.client_ip<<" is logged out."<<endl;
                                    cse4589_print_and_log("[RELAYED:END]\n");
                                    strcpy (client_msg.cmd, "send_failure");
                                    // Maybe we should do some error-handling here as well
                                    send (sock_index, &client_msg, sizeof (client_msg), 0);
                                }
                            }
                            //COMMENTED OUT ENTIRE BLOCK OF CODE FOR STABILITY, BECAUSE IT COULD INCORRECTLY EFFECT STATISTICS OF MESSAGES SENT/RECEIVED  
                            // else if (!strcmp(client_msg.cmd, "broadcast")) {
                            //     char sender_client_ip[32];
                            //     // Who's thr sender?
                            //     for (auto k: list_data_ptr) {
                            //         if (k->socket == sock_index) {
                            //             strcpy(sender_client_ip, k->ip);
                            //             k->snd_msg_count++;
                            //         }
                            //     }
                            //     // Who's the receiver?
                            //     for (auto i: list_data_ptr) {
                            //         if (!strcmp(i->ip, client_msg.client_ip)) {
                            //             found_ip = 1;

                            //             i->rcv_msg_count++;
                            //             if (!strcmp(i->status, LOGGED_OUT)) {
                            //                 strcat(i->buffer, client_msg.data);
                            //                 target_client_logged_out = 1;
                            //                 break;
                            //             } else {
                            //                 strcpy(server_msg.cmd, "relayed_msg");
                            //                 strcpy(server_msg.sender_ip, sender_client_ip);
                            //                 strcpy(server_msg.data, client_msg.data);
                            //                 if (send(i->socket, &server_msg, sizeof(server_msg), 0) ==
                            //                     sizeof(server_msg)) {
                            //                     cse4589_print_and_log("[RELAYED:SUCCESS]\n");
                            //                     cse4589_print_and_log("msg from:%s, to:255.255.255.255\n[msg]:%s\n",
                            //                                           sender_client_ip, server_msg.data);
                            //                     cse4589_print_and_log("[RELAYED:END]\n");
                            //                     strcpy (client_msg.cmd, "send_success");
                            //                     fflush(stdout);
                            //                     // Maybe we should do some error-handling here as well
                            //                     if (send (sock_index, &client_msg, sizeof (client_msg), 0) ==
                            //                     sizeof (client_msg)) {
                            //                         break;
                            //                     }
                            //                 }
                            //                 cse4589_print_and_log("[RELAYED:ERROR]\n");
                            //                 cse4589_print_and_log("[RELAYED:END]\n");
                            //                 strcpy (client_msg.cmd, "send_failure");
                            //                 // Maybe we should do some error-handling here as well
                            //                 send (sock_index, &client_msg, sizeof (client_msg), 0);
                            //                 break;
                            //             }
                            //         }
                            //     }
                            // }
                        }
                    }
                }
            }
        }
    }
}

/**
 * Comparator used to sort a list of pointers to struct list_data in increasing order of port numbers.
 * Designed specifically for sort() method in <algorithm>.
 * E.g. - sort(list_data_ptr, list_data_ptr + size, compare_list_data);
 * @return returns true if a < b
 */
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

    // Init
    for(int i = 0; i < 5; i++) {
        list_data_ptr[i] = (struct list_data *) malloc(sizeof(struct list_data));
        list_data_ptr[i]->id = 0;
    }

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
                            if (!logged_in) {
                                cse4589_print_and_log("[LIST:ERROR]\n");
                                cout<<"You need to be logged in to execute this command!"<<endl;
                                cse4589_print_and_log("[LIST:END]\n");
                                continue;
                            }
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
                                fflush(stdout);
                            }
                            cse4589_print_and_log("[LIST:END]\n");
                            fflush(stdout);
                        } else if (!strcmp(command, "LOGIN")) {
                            bool error = 0;
                            // Need to extract just the first string from command and parse the other 2 here
                            char *server_ip = strtok_r(NULL, " ", &saved_context);
                            char *server_port = strtok_r(NULL, " ", &saved_context);
                            
                            //cout<<"Server IP: "<<server_ip<<"Server Port: "<<server_port<<endl;
                            //cout.flush(); 

                            if (server_ip == NULL || server_port == NULL) {
                                cse4589_print_and_log("[LOGIN:ERROR]\n");
                                cout<<"Incorrect Usage: LOGIN [server IP] [server port]"<<endl;
                                cse4589_print_and_log("[LOGIN:END]\n");
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
                                cse4589_print_and_log("[LOGIN:ERROR]\n");
                                perror("Invalid port, contains non-digits. Try again.");
                                cse4589_print_and_log("[LOGIN:END]\n");
                                continue;
                            }
                            // Is valid range of ports ?
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
                            cse4589_print_and_log("[LOGIN:END]\n");

                        } else if (!strcmp(command, "REFRESH")) {
                            if (!logged_in) {
                                cse4589_print_and_log("[REFRESH:ERROR]\n");
                                cout << "You need to be logged in to execute this command!" << endl;
                                cse4589_print_and_log("[REFRESH:END]\n");
                                continue;
                            }
                            strcpy(client_msg.cmd, "refresh");
                            if (send(server_sock, &client_msg, sizeof(client_msg), 0) == sizeof(client_msg))
                            {
                                cse4589_print_and_log("[REFRESH:SUCCESS]\n");
                                int idx = 1;
                                for(auto i : list_data_ptr) {
                                    if (i->id == 0 || strcmp(i->status, LOGGED_IN)) {
                                        continue;
                                    }
                                    cse4589_print_and_log ("%-5d%-35s%-20s%-8d\n", idx, i->host_name, i->ip, i->port);
                                    idx ++;
                                    fflush(stdout);
                                }
                                cse4589_print_and_log("[REFRESH:END]\n");
                            }
                        } else if (!strcmp(command, "SEND")) {
                            if (!logged_in) {
                                cse4589_print_and_log("[SEND:ERROR]\n");
                                cout<<"You need to be logged in to execute this command!"<<endl;
                                cse4589_print_and_log("[SEND:END]\n");
                                continue;
                            }
                            bool error = 0;
                            char *client_ip = strtok_r(NULL, " ", &saved_context);
                            char *data = strtok_r(NULL, " ", &saved_context);

                            if (client_ip == NULL || data == NULL) {
                                cse4589_print_and_log("[SEND:ERROR]\n");
                                cout<<"Incorrect Usage: SEND [client IP] [client msg]"<<endl;
                                cse4589_print_and_log("[SEND:END]\n");
                                continue;
                            }
                            if (!isvalidIP(client_ip)) {
                                cse4589_print_and_log("[SEND:ERROR]\n");
                                perror("Invalid IP!");
                                cse4589_print_and_log("[SEND:END]\n");
                                continue;
                            }
                            cout.flush();
                            strcpy(client_msg.cmd, "send");
                            strcpy(client_msg.client_ip, client_ip);
                            strcpy(client_msg.data, data);
                            if (send(server_sock, &client_msg, sizeof (client_msg), 0) != sizeof (client_msg)) {
                                cse4589_print_and_log("[SEND:ERROR]\n");
                                cout<<"send failed!"<<endl;
                                cout.flush();
                                cse4589_print_and_log("[SEND:END]\n");
                                fflush(stdout);
                            }
                        } else if (!strcmp(command, "BROADCAST")) {
                            //COMMENTED OUT ENTIRE BLOCK OF CODE FOR STABILITY, BECAUSE IT COULD INCORRECTLY EFFECT STATISTICS OF MESSAGES SENT/RECEIVED 
                            // if (!logged_in) {
                            //     cout<<"You need to be logged in to execute this command!"<<endl;
                            //     cse4589_print_and_log("[BROADCAST:ERROR]\n");
                            //     cse4589_print_and_log("[BROADCAST:END]\n");
                            //     continue;
                            // }
                            // bool error = 0;
                            // char *client_ip = strtok_r(NULL, " ", &saved_context);
                            // char *data = strtok_r(NULL, " ", &saved_context);

                            // if (client_ip == NULL || data == NULL) {
                            //     cout<<"Incorrect Usage: BROADCAST [client IP] [client msg]"<<endl;
                            //     cse4589_print_and_log("[BROADCAST:ERROR]\n");
                            //     cse4589_print_and_log("[BROADCAST:END]\n");
                            //     continue;
                            // }
                            // if (!isvalidIP(client_ip)) {
                            //     perror("Invalid IP!");
                            //     cse4589_print_and_log("[BROADCAST:ERROR]\n");
                            //     cse4589_print_and_log("[BROADCAST:END]\n");
                            //     continue;
                            // }
                            // cout.flush();
                            // strcpy(client_msg.cmd, "broadcast");
                            // strcpy(client_msg.client_ip, client_ip);
                            // strcpy(client_msg.data, data);
                            // if (send(server_sock, &client_msg, sizeof (client_msg), 0) != sizeof (client_msg)) {
                            //     cout<<"send failed!"<<endl;
                            //     cout.flush();
                            //     cse4589_print_and_log("[BROADCAST:ERROR]\n");
                            //     cse4589_print_and_log("[BROADCAST:END]\n");
                            //     fflush(stdout);
                            // }

                        } else if (!strcmp(command, "BLOCK")) {
                            // if (!logged_in) {
                            //     cout<<"You need to be logged in to execute this command!"<<endl;
                            //     cse4589_print_and_log("[BLOCK:ERROR]\n");
                            //     cse4589_print_and_log("[BLOCK:END]\n");
                            //     continue;
                            // }
                            // bool error = 0;
                            // char *client_ip = strtok_r(NULL, " ", &saved_context);
                            // //
                            // if (client_ip == NULL) {
                            //     cout<<"Incorrect Usage: BLOCK [client IP]"<<endl;
                            //     cse4589_print_and_log("[BLOCK:ERROR]\n");
                            //     cse4589_print_and_log("[BLOCK:END]\n");
                            //     continue;
                            // }
                            // if (!isvalidIP(client_ip)) {
                            //     perror("Invalid IP!");
                            //     cse4589_print_and_log("[BLOCK:ERROR]\n");
                            //     cse4589_print_and_log("[BLOCK:END]\n");
                            //     continue;
                            // }

                        } else if (!strcmp(command, "UNBLOCK")) {

                        } else if (!strcmp(command, "LOGOUT")) {
                            if (!logged_in) {
                                cse4589_print_and_log("[SEND:ERROR]\n");
                                cout<<"Not logged in!"<<endl;
                                cse4589_print_and_log("[SEND:END]\n");
                                continue;
                            }
                            // Tell server we're logging out.
                            strcpy(client_msg.cmd,"logout");
                            if (send(server_sock, &client_msg, sizeof (client_msg), 0) == sizeof (client_msg)) {
                                cse4589_print_and_log("[LOGOUT:SUCCESS]\n");
                                logged_in = 0;
                                /* To deal with "Connect failed: Bad file descriptor" for clients trying to
                                 * login after a logout we need to bind the port to client_socket here since
                                 * we closed server sock.
                                 * */
                                if(!socket_bind(atoi(port)))
                                    exit(-1);
                                server_sock=close(server_sock);
                            }
                            cse4589_print_and_log("[LOGOUT:END]\n");
                        } else if (!strcmp(command, "EXIT")) {
                            // Logout if logged-in
                            if (logged_in) {
                                // Tell server we're logging out forever
                                strcpy(client_msg.cmd,"client_exit");
                                if (send(server_sock, &client_msg, sizeof (client_msg), 0) == sizeof (client_msg)) {
                                    cse4589_print_and_log("[EXIT:SUCCESS]\n");
                                    logged_in = 0;
                                    server_sock = close(server_sock);
                                }
                                cse4589_print_and_log("[EXIT:END]\n");
                            }
                            // Free up dynamically allocated mem
                            for (auto i : list_data_ptr)
                                free(i);
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

                            if(!strcmp (msg_rcvd.cmd, "login_list")) {
                                // Refresh LIST data
                                for (auto k : list_data_ptr) {
                                    k->id = 0;
                                }
                                for (auto k : msg_rcvd.list_entries) {
                                    cout<<"Id : "<<k.id<<" name : "<<k.host_name<<" status : "<<k.status<<endl;
                                    cout.flush();
                                }
                                int client_count = 0;
                                for (auto i : msg_rcvd.list_entries) {
                                    *list_data_ptr[client_count] = i;
                                    client_count++;
                                }
                                cse4589_print_and_log("[LOGIN:END]\n");
                            } else if (!strcmp (msg_rcvd.cmd, "refresh_data")) {
                                // Refresh LIST data
                                for (auto k : list_data_ptr) {
                                    k->id = 0;
                                }
                                /*for (auto k : msg_rcvd.list_entries) {
                                    cout<<"Id : "<<k.id<<" name : "<<k.host_name<<" status : "<<k.status<<endl;
                                    cout.flush();
                                }*/
                                int client_count = 0;
                                for (auto i : msg_rcvd.list_entries) {
                                    *list_data_ptr[client_count] = i;
                                    client_count++;
                                }
                                cse4589_print_and_log("[REFRESH:SUCCESS]\n");
                                cse4589_print_and_log("[REFRESH:END]\n");
                            } else if (!strcmp(msg_rcvd.cmd, "send_success")) {
                                // SEND success response from server
                                cse4589_print_and_log("[SEND:SUCCESS]\n");
                                cse4589_print_and_log("[SEND:END]\n");
                            } else if (!strcmp(msg_rcvd.cmd, "send_failure")) {
                                // SEND success response from server
                                cse4589_print_and_log("[SEND:ERROR]\n");
                                cse4589_print_and_log("[SEND:END]\n");
                            } else if (!strcmp(msg_rcvd.cmd, "relayed_msg")) {
                                cse4589_print_and_log("[RECEIVED:SUCCESS]\n");
                                cse4589_print_and_log("msg from:%s\n[msg]:%s\n",
                                                      msg_rcvd.sender_ip, msg_rcvd.data);
                                cse4589_print_and_log("[RECEIVED:END]\n");
                                fflush(stdout);
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

/**
 * Creates a dummy unbounded UDP socket and connects to google DNS server.
 * Leverages getsockname to get the external IP.
 * Approach referenced from - https://ubmnc.wordpress.com/2010/09/22/on-getting-the-ip-name-of-a-machine-for-chatty/
 */
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

/**
 * This method is only used by the client to bind the client_socket to the client port.
 * After a server close on a LOGOUT we might have to call this again to allocate a new socket in-case
 * the client logs in again.
 * @param client_port - Port on which the client was started.
 * @return - Boolean value representing the status of socket creation and bind.
 */
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

/**
 * Used by the client to connect to a server.
 * @param server_ip
 * @param server_port
 * @return - Client socket file descriptor if connect was successful, else -1. Needs to be handled by caller.
 */
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
