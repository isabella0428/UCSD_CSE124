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
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>

void sendResponse(int socket_id, int is_Good)
{
    char *request = (char *)malloc(4 * sizeof(char));
    if (is_Good)
    {
        request = "200";
    }
    else
    {
        request = "404";
    }

    int r;
    if ((r = send(socket_id, request, sizeof(request), 0) == 0))
    {
        perror("Send Connction Request Failed!");
    }
}

char *parseRequest(char *request, char *absolute_path)
{
    // Check if it starts with "GET"
    if (request[0] != 'G' || request[1] != 'E' || request[2] != 'T' || request[3] != ' ')
    {
        absolute_path = NULL;
    }

    // Parse filename
    int i = 4, j = 0;
    char filename[200];
    while (request[i] != ' ')
    {
        filename[j] = request[i];
        i++;
        j++;
    }
    filename[j] = '\0';
    i++;

    // Expand relative path to absolote path
    realpath(filename, absolute_path);

    if (request[i] != 'H' || request[i + 1] != 'T' 
            || request[i + 2] != 'T' || request[i + 3] != 'P' || request[i + 4] != '/'){
        absolute_path = NULL;
    }
}

sockaddr_in *setIpAddr(char *server_name, sockaddr_in *address)
{
    // Set up local addr
    hostent *host_info = gethostbyname(server_name);

    address->sin_family = AF_INET;
    if (host_info)
    {
        address->sin_addr = *((in_addr *)(host_info->h_addr_list[0]));
        // Reverse sequence of the bytes
        address->sin_addr.s_addr = ntohl(address->sin_addr.s_addr);
    }
    return address;
}

int main() {
    std::cout << "Server side" << std::endl;

    // Create a socket
    int socket_id, r;
    if ((r = socket_id = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Failed to create a socket");
        return EXIT_FAILURE;
    }

    std::cout<<"Socket Created"<<std::endl;

    // Bind the socket to port 8080
    int port_num = 8080;
    sockaddr_in *address = (sockaddr_in *)malloc(sizeof(sockaddr_in));
    address->sin_port = htons(port_num);
    address->sin_addr.s_addr = INADDR_ANY;
    if ((r = bind(socket_id, (sockaddr *)address, socklen_t(sizeof(*address)))) != 0) {
        perror("Failed to bind the socket");
        close(socket_id);
        return EXIT_FAILURE;
    }
    free(address);

    std::cout << "Socket Binded" << std::endl;

    // Listen for requests
    if (listen(socket_id, 10) != 0) {
        perror("Listen failed");
        return EXIT_FAILURE;
    }

    std::cout << "Start Listening" << std::endl;

    // Accept client's request
    sockaddr_in *client_addr;
    int new_sock;
    if((new_sock = accept(socket_id, (sockaddr *)client_addr, (socklen_t *)(sizeof(&client_addr)))) == 0) {
        perror("Failed to accept the request!");
        sendResponse(new_sock, 0);
    } else {
        sendResponse(new_sock, 1);
    }

    std::cout << "Accept the request and Sent response" << std::endl;

    // Receive request
    char *message = (char *)malloc(sizeof(char) * 2000);
    if ((r = recv(new_sock, message, 200, 0)) == 0) {
        perror("Failed to receive request from Client");
    }

    std::cout << "Request received" << std::endl;

    // parse Request
    char filename[200];
    parseRequest(message, filename);
    std::cout << filename << std::endl;
    if (!filename) {
        perror("Client Request Illegal!");
    }

    std::cout << "Request valid" << std::endl;
    std::cout << "Start transmitting file" << std::endl;

    // Send file
    int fd = open(filename, 0);
    if (fd == -1) {
        perror("Error occurred in opening the target file");
        return EXIT_FAILURE;
    }
    char buf[220];
    while(read(fd, buf, 200) != 0) {
        if ((r = send(new_sock, buf, sizeof(buf), 0)) == 0) {
            perror("Error during file transformission!");
            return EXIT_FAILURE;
        }
    }

    std::cout << "Finish transmitting file" << std::endl;

    close(socket_id);
    return 0;
}



