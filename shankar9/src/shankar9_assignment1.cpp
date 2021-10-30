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

#include "../include/global.h"
#include "../include/logger.h"

#define DNS_IP "8.8.8.8"
#define DNS_PORT "53"
#define _MAX_PATH 256

using namespace std;
// "1" in server mode, "0" in client mode
bool mode;

//enum Server_Commands {
//    AUTHOR = 1,
//    IP = 2,
//    PORT = 3,
//    LIST = 4,
//    STATISTICS = 5,
//    BLOCKED = 6
//};
//
//enum Client_Commands {
//    CLIENT_AUTHOR = 1,
//    CLIENT_IP = 2,
//    CLIENT_PORT = 3,
//    CLIENT_LIST = 4,
//    LOGIN = 5,
//    REFRESH = 6,
//    SEND = 7,
//    BROADCAST = 8,
//    BLOCK = 9,
//    UNBLOCK = 10,
//    LOGOUT = 11,
//    EXIT = 12
//};



void print_author_statement();
void print_ip_address();
void server (int port);
void client (int port);

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
    int yes=1;

    if (*argv[1] == 's') {
        // server mode
        mode = 1;
        server(atoi(argv[2]));
    } else if (*argv[1] == 'c') {
        // client mode
        mode = 0;
        //client(atoi(argv[2]));
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

void server (int port) {
    string command_str;
    while (1) {
        cout << "[PA1-Server@CSE489/589]$ ";
        cin.getline(command_str, _MAX_PATH);
        // Need to use strtok() to parse the command and arguments separately
        char *command = strtok(command_str, " ");
        if (command == "AUTHOR") {
            print_author_statement();
        } else if (command == "IP") {
            print_ip_address();
        } else if (command == "PORT") {

        } else if (command == "LIST") {

        } else if (command == "STATISTICS") {

        } else if (command == "BLOCKED") {
            // Need to figure out how to work with 2nd argument here maybe strtok()
        } else {
            // Unidentified command
            cout << "Unidentified command, ignoring...";
        }
    }
}

//void client (int port) {
//
//    std::map<std::string, Client_Commands> string_ClientCommand_map;
//    populate_ClientCommand_map(string_ClientCommand_map);
//
//    string command;
//    while (1) {
//        cout<<"[PA1-Client@CSE489/589]$ ";
//        cin.getline(command, _MAX_PATH);
//        // Need to use strtok() to parse the command and arguments separately
//        switch (hash(command))  {
//            case hash("AUTHOR"):
//                print_author_statement();
//                break;
//            case hash("IP"):
//                print_ip_address();
//                break;
//            case hash("PORT"):
//                char command_str[] = "PORT";
//                cse4589_print_and_log("[%s:SUCCESS]\n", command_str);
//                cse4589_print_and_log("PORT:%d\n", port);
//                cse4589_print_and_log("[%s:END]\n", command_str);
//                break;
//            case hash("LIST"):
//                break;
//            case hash("LOGIN"):
//                // Need to extract just the first string from command and parse the other 2 here
//                break;
//            case hash("REFRESH"):
//                break;
//            case hash("SEND"):
//                break;
//            case hash("BROADCAST"):
//                break;
//            case hash("BLOCK"):
//                break;
//            case hash("UNBLOCK"):
//                break;
//            case hash("LOGOUT"):
//                break;
//            case hash("EXIT"):
//                // Logout if logged-in
//                exit(0);
//                break;
//            default:
//                // Unidentified command
//                cout<<"Unidentified command, ignoring...";
//        }
//    }
//
//}

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
