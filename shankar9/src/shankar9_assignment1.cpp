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
 * This contains the main function. Add further description here....
 */
#include <iostream>
#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <map>
#include <cstring>
#include <netdb.h>
#include <unistd.h>

#include "../include/global.h"
#include "../include/logger.h"

#define DNS_IP "8.8.8.8"
#define DNS_PORT "53"
#define BACKLOG 5
#define STDIN 0
#define TRUE 1
#define CMD_SIZE 100
#define BUFFER_SIZE 256

using namespace std;
// "1" in server mode, "0" in client mode
bool mode;

void print_author_statement();
void print_ip_address();
void server (char* port);
void client (char* port);

/**
 * main function
 *
 * @param  argc Number of arguments
 * @param  argv The argument list
 * @return 0 EXIT_SUCCESS
 */
int main(int argc, char **argv)
{
    if(argc != 3) {
        cout<<"Usage: [./assignment1] [mode] [port]"<<endl;
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

constexpr unsigned int hash_val(const char* str, int h = 0)
{
    return !str[h] ? 5381 : (hash_val(str, h+1)*33) ^ str[h];
}

void server (char* port) {

    int yes=1; // For setsockopt
    int server_socket, head_socket, selret, sock_index, fdaccept=0, caddr_len;
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
    if(server_socket < 0)
        perror("Cannot create socket");
    
    // Gets rid of socket in use error
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    
    /* Bind */
    if(bind(server_socket, res->ai_addr, res->ai_addrlen) < 0 )
        perror("Bind failed");
    
    freeaddrinfo(res);
    //cout<<"Can print here4!"<<endl;
    //cout<<"[PA1-Server@CSE489/589]$ ";
    //cout<<"[PA1-Server@CSE489/589]$ ";
	
    /* Listen */
    if(listen(server_socket, BACKLOG) < 0)
        perror("Unable to listen on port");

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

        cout<<"[PA1-Server@CSE489/589]$ ";
        cout.flush();
        /* select() system call. This will BLOCK */
        selret = select(head_socket + 1, &watch_list, NULL, NULL, NULL);
        if(selret < 0)
            perror("select failed.");

        /* Check if we have sockets/STDIN to process */
        if(selret > 0) {
            /* Loop through socket descriptors to check which ones are ready */
            for (sock_index = 0; sock_index <= head_socket; sock_index += 1) {

                if (FD_ISSET(sock_index, &watch_list)) {

                    /* Check if new command on STDIN */
                    if (sock_index == STDIN) {
                        char *cmd = (char *) malloc(sizeof(char) * CMD_SIZE);

                        memset(cmd, '\0', CMD_SIZE);
                        if (fgets(cmd, CMD_SIZE - 1, stdin) ==
                            NULL) //Mind the newline character that will be written to cmd
                            exit(-1);
			cmd[strcspn(cmd, "\n")] = 0;
                        // Need to use strtok() to parse the command and arguments separately
                        char *command = strtok(cmd, " ");
                        cout <<"Command = " << command<< endl;
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
                        fdaccept = accept(server_socket, (struct sockaddr *) &client_addr, (socklen_t *)&caddr_len);
                        if (fdaccept < 0)
                            perror("Accept failed.");

                        printf("\nRemote Host connected!\n");

                        /* Add to watched socket list */
                        FD_SET(fdaccept, &master_list);
                        if (fdaccept > head_socket) head_socket = fdaccept;
                    }
                    /* Read from existing clients */
                    else {
                        /* Initialize buffer to receieve response */
                        char *buffer = (char *) malloc(sizeof(char) * BUFFER_SIZE);
                        memset(buffer, '\0', BUFFER_SIZE);

                        if (recv(sock_index, buffer, BUFFER_SIZE, 0) <= 0) {
                            close(sock_index);
                            printf("Remote Host terminated sockaddrtion!\n");

                            /* Remove from watched list */
                            FD_CLR(sock_index, &master_list);
                        } else {
                            //Process incoming data from existing clients here ...

                            printf("\nClient sent me: %s\n", buffer);
                            printf("ECHOing it back to the remote host ... ");
                            if (send(fdaccept, buffer, strlen(buffer), 0) == strlen(buffer))
                                printf("Done!\n");
                            fflush(stdout);
                        }

                        free(buffer);
                    }
                }
            }
        }
    }
}

void client (char* port) {
    string command_str;
    while (1) {
        cout<<"[PA1-Client@CSE489/589]$ ";
        getline(cin, command_str);
        char *cstr = new char[command_str.length() + 1];
        strcpy(cstr, command_str.c_str());
        // Need to use strtok() to parse the command and arguments separately
        char *command = strtok(cstr, " ");
        cout<<"Command = "<<command<<endl;
        if (!strcmp(command, "AUTHOR")) {
            print_author_statement();
        } else if (!strcmp(command, "IP")) {
            print_ip_address();
        } else if (!strcmp(command, "PORT")) {
            cse4589_print_and_log("[%s:SUCCESS]\n", command);
            cse4589_print_and_log("PORT:%c\n", port);
            cse4589_print_and_log("[%s:END]\n", command);
        } else if (!strcmp(command, "LIST")) {

        } else if (!strcmp(command, "LOGIN")) {
            // Need to extract just the first string from command and parse the other 2 here
        } else if (!strcmp(command, "REFRESH")) {

        } else if (!strcmp(command, "SEND")) {

        } else if (!strcmp(command, "BROADCAST")) {

        } else if (!strcmp(command, "BLOCK")) {

        } else if (!strcmp(command, "UNBLOCK")) {

        } else if (!strcmp(command, "LOGOUT")) {

        } else if (!strcmp(command, "EXIT")) {
            // Logout if logged-in
            exit(0);
        } else {
            // Unidentified command
            cout<<"Unidentified command, ignoring..."<<endl;
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

void print_ip_address () {
    struct addrinfo hints, *res;
    int dum_socket;
    struct sockaddr_in _self;
    int sa_len = sizeof (_self);
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
    if(dum_socket < 0) {
        perror("Failed to create socket");
        success = 0;
    }
    if (connect(dum_socket, res->ai_addr, res->ai_addrlen) < 0) {
        perror("Connect failed");
        success = 0;
    }

    memset (&_self, 42, sa_len);
    if (getsockname(dum_socket, (struct sockaddr *) &_self, (socklen_t *)&sa_len) == -1) {
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
