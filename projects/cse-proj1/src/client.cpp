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
#include <string>
#include <unordered_map>
#include <fstream>
#include <vector>

#include "client.hpp"

int Client::connect_to_server(int port_num) {
    std::cout << "Client side" << std::endl;

    // Create a socket
    int socket_id, r;

    if ((socket_id = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket Creation Failed!");
        return EXIT_FAILURE;
    }

    std::cout << "Socket Created" << std::endl;

    // Set up remote socket address
    sockaddr_in *address = (sockaddr_in *)malloc(sizeof(sockaddr_in));
    address->sin_family = AF_INET;
    address->sin_port = htons(port_num);

    // Server address
    std::string server_name = "localhost";
    const hostent *host_info = gethostbyname(server_name.c_str());

    if (host_info)
    {
        address->sin_addr = *((in_addr *)(host_info->h_addr_list[0]));
    }

    // Connect to the server
    if ((r = connect(socket_id, (struct sockaddr *)address, sizeof(*address))) != 0)
    {
        perror("Socket Connection Failed!");
        return EXIT_FAILURE;
    }

    std::cout << "Connected to Server" << std::endl;
    return socket_id;
}

void Client::sendRequest(char *message, int socket_id) {
    if (send(socket_id, message, sizeof(message) + 100, 0) == 0)
    {
        perror("Send Connction Request Failed!");
    }
}

void Client::receive(int socket_id, std::string path)
{
    std::cout << "Start receiving response" << std::endl;

    std::cout << "----------------------" << std::endl;
    std::cout << "Response received: " << std::endl;

    // Receive respond and file from server
    char respond[2000];
    std::vector<char> fullResponse;
    size_t totalLength = 0;
    size_t len = 0;

    while (1)
    {
        bzero(respond, 2000);
        if ((len = recv(socket_id, respond, sizeof(char) * 2000, 0)) == 0)
        {
            break;
        }
        totalLength += len;
        int lastSize = fullResponse.size();
        fullResponse.resize(totalLength);
        memcpy(&fullResponse[lastSize], respond, len);
    }
    std::string response(&fullResponse[0], totalLength);
    // Find filename
    int idx = response.find("Filename: ");
    if (idx == (int)std::string::npos)
    {
        return;
    }
    int fileContentId = response.find("\r\n\r\n");
    if (fileContentId == (int)std::string::npos)
    {
        return;
    }
    std::string filename = response.substr(idx + 10, response.find("Last") - idx - 12);
    std::string file = response.substr(fileContentId + 4, response.size() - fileContentId - 4);
    saveFile(filename, file, path);
}

void Client::saveFile(std::string filename, std::string file, std::string path) {
    chdir((char *)path.c_str());
    std::ofstream f;
    char absolute_path[300];
    realpath(filename.c_str(), absolute_path);
    f.open(absolute_path);

    if (f.is_open()) {
        f.write(file.c_str(), file.size());
        f.close();
        std::cout<<"File saved to: "<<absolute_path<<std::endl;
    } else {
        std::cout<<("Failed to Save file")<<std::endl;
    }
}



