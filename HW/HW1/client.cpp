/*
Author: Yi Lyu
Email:  isabella_aus_china@sjtu.edu.cn
Date:   2020.1.28
*/

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>

int main(int argc, char *argv[]) {
    std::cout << "Client side" << std::endl;

    // Create a socket
    int socket_id, port_num, r;
    port_num = 8080;

    if ((socket_id = socket(AF_INET, SOCK_STREAM,0)) == 0) {
        perror("Socket Creation Failed!");
        return EXIT_FAILURE;
    }

    std::cout << "Socket Created" << std::endl;

    // Set up remote socket address
    sockaddr_in *address = (sockaddr_in *)malloc(sizeof(sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(port_num);

    // Server address
    char *server_name = "localhost";
    const hostent *host_info = gethostbyname(server_name);

    if (host_info){
        address->sin_addr = *((in_addr *)(host_info->h_addr_list[0]));
    }

    // Connect to the server
    if ((r = connect(socket_id, (struct sockaddr *)address, sizeof(*address))) != 0) {
        perror("Socket Connection Failed!");
        return EXIT_FAILURE;
    }

    std::cout << "Connected to Server" << std::endl;

    // Send the request
    char *request = "GET ./test.txt HTTP/1.1";
    if ((r = send(socket_id, request, sizeof(request) + 30, 0) == 0)) {
        perror("Send Connction Request Failed!");
        return EXIT_FAILURE;
    }

    std::cout << "Request Sent" << std::endl;

    // Receive respond from server
    char respond[200];
    if ((r = recv(socket_id, respond, sizeof(respond), 0)) == 0){
        perror("Receive Response from server Failed");
        return EXIT_FAILURE;
    }
    if (respond == "404") {
        perror("Failed to establish connection with server!");
        return EXIT_FAILURE;
    }

    std::cout << "File available" << std::endl;

    std::cout << "Start receiving file" << std::endl;

    // Receive Message
    char input[2000];
    while(1) {
        if ((r = read(socket_id, input, sizeof(input))) == 0) {
            break;
        }
    }

    std::cout<<"----------------------"<< std::endl;
    std::cout << "File received: " << std::endl;
    std::cout << input << std::endl;
    return 0;
}