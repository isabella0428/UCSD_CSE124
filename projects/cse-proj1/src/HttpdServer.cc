#include <sysexits.h>
#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <sys/socket.h>
#include <limits.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>
#include <unordered_map>
#include <fstream>
#include <string>
#include <unistd.h>

#include "logger.hpp"
#include "HttpdServer.hpp"

HttpdServer::HttpdServer(INIReader& t_config)
	: config(t_config)
{
	auto log = logger();

	string pstr = config.Get("httpd", "port", "");
	if (pstr == "") {
		log->error("port was not in the config file");
		exit(EX_CONFIG);
	}
	port = pstr;

	string dr = config.Get("httpd", "doc_root", "");
	if (dr == "") {
		log->error("doc_root was not in the config file");
		exit(EX_CONFIG);
	}
	doc_root = dr;

	timeout = config.GetInteger("httpd", "timeout", 5);
	// setUpMIMEMap((char *)config.Get("httpd", "mime_types", "").c_str());
	char path[30] = "mime.types";
	setUpMIMEMap(path);
}

void HttpdServer::launch()
{
	auto log = logger();

	log->info("Launching web server");
	log->info("Port: {}", port);
	log->info("doc_root: {}", doc_root);

	// Set working path
	chdir(config.Get("httpd", "doc_root", "").c_str());

	// Create a socket
	int socket_id, r;
	if ((r = socket_id = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		log->error("Failed to create a socket");
		return;
	}

	log->info("Socket created");

	// Bind the socket to port 8080
	sockaddr_in *address = (sockaddr_in *)malloc(sizeof(sockaddr_in));
	address->sin_port = htons(stoi(port));
	address->sin_addr.s_addr = INADDR_ANY;

	// Bind Socket to the server
	if ((r = ::bind(socket_id, (sockaddr *)address, socklen_t(sizeof(*address)))) != 0)
	{
		log->error("Failed to bind the socket");
		close(socket_id);
	}
	free(address);
	log->info("Socket Binded");

	// Listen for requests
	if (listen(socket_id, 10) != 0) {
		log->error("Failed to listen");
		close(socket_id);
		return;
	}
	log->info("Start Listening");

	thread *t;
	while(1) {
		// Accept client's request
		sockaddr_in *client_addr = nullptr;
		int new_sock;
		if ((new_sock = accept(socket_id, (sockaddr *)client_addr, (socklen_t *)(sizeof(&client_addr)))) == 0)
		{
			log->error("Failed to accept the request!");
			return;
		}

		// processRequest(new_sock);
		// INIReader temp("myconfig.ini");
		t = new thread([this](int i) { this->processRequest(i); }, new_sock);
	}
}

void HttpdServer::processRequest(int socket_id) 
{
	auto log = logger();
	log->info("Start Processing Request");
	// Receive request
	int r;
	char *message = (char *)malloc(sizeof(char) * 2000);
	if ((r = recv(socket_id, message, 200, 0)) == 0)
	{
		log->error("Failed to receive request from Client");
		return;
	}

	char *filename = (char *)malloc(500*sizeof(char));
	parseRequest(message, filename);
	if(strcmp(filename, "") == 0) {
		log->info("Illegal Request");
	}

	sprintf(message, "Start Transmitting file: %s", filename);

	// Transmit files
	transmitFile(filename, socket_id);
	close(socket_id);
}

void HttpdServer::parseRequest(char *request, char *absolute_path) 
{
	int i = 0, j = 0;
	while(request[i] == ' ') {
		i ++;
	}

	char filename[200];
	// Check if it starts with "GET"
	if (request[i] != 'G' || request[i + 1] != 'E' || request[i + 2] != 'T') {
		absolute_path = NULL;
		return;
	}

	// Parse filename
	i = i + 3;
	while (request[i] == ' '){
		i++;
	}

	while (request[i] != ' ') {
		filename[j] = request[i];
		// Escape the doc root
		if(request[i] == '.' && request[i - 1] == '.') {
			absolute_path = NULL;
			return;
		}
		i++;
		j++;
	}
	filename[j] = '\0';
	i++;

	// Expand relative path to absolote path
	realpath(filename, absolute_path);
	

	if (request[i] != 'H' || request[i + 1] != 'T' || request[i + 2] != 'T' 
			|| request[i + 3] != 'P' || request[i + 4] != '/') 
		absolute_path = NULL;
	return;
}

void HttpdServer::transmitFile(char *filename, int socket_id) {
	auto log = logger();
	std::string message;

	if(filename == NULL) {
		message.append(parseResponse("404", NULL));
		send(socket_id, (char *)message.c_str(), sizeof(char)*message.length(), 0);
		return;
	}

	if (access(filename, F_OK) != 0)
	{
		message.append(parseResponse("404", NULL));
		send(socket_id, (char *)message.c_str(), 20000, 0);
		return;
	}

	if (access(filename, R_OK) != 0)
	{
		message.append(parseResponse("403", NULL));
		send(socket_id, (char *)message.c_str(), sizeof(char) * message.length(), 0);
		return;
	}

	// Open file
	int fd = open(filename, O_RDONLY);
	if (fd == -1) {
		message.append(parseResponse("404", NULL));
		send(socket_id, (char *)message.c_str(), sizeof(char) * message.length(), 0);
		return;
	}

	message.append(parseResponse("200", filename));
	send(socket_id, (char *)message.c_str(), message.length() * sizeof(char), 0);
	char buf[220];
	int len;
	while ((len = read(fd, buf, sizeof(buf))) != 0)
	{
		message.append(buf);
		send(socket_id, buf, sizeof(char) * len, 0);
		bzero(buf, 220);
	}

	log->info("Finish Transmitting File");
	return;
}

std::string HttpdServer::parseResponse(string message, char* path)
{
	string r = "";
	char *temp = (char *)malloc(100);
	sprintf(temp, "HTTP/1.1 %s ", message.c_str());
	r.append(temp);
	if(message == "200") {
		r.append("OK\r\n");
	} else {
		r.append("\r\n");
	}

	r.append("Server: Myserver 1.0\r\n");

	if (message == "200") {
		r.append("Filename: ");
		// Get filename from path
		string absolute_path;
		absolute_path.append(path);
		int idx = absolute_path.find_last_of("/");
		r.append(absolute_path.substr(idx+1));
		r.append("\r\n");	

		r.append("Last modified: ");
		struct tm *ptr;
		time_t lt = time(NULL);
		ptr = localtime(&lt);
		strftime(temp, 100, "%a, %d %b %y %T %z\r\n", ptr);
		r.append(temp);
		
		struct stat stat_buf;
		int rc = stat(path, &stat_buf);
		int length = rc == 0 ? stat_buf.st_size : -1;
		sprintf(temp, "Content-Length: %d\r\n", length);
		r.append(temp);

		r.append("Content-Type: ");
		// Find file extension
		char extension[20];
		int i = 0, j = 0;
		while(path[i] != '.') {
			i++;
		}
		while(path[i] != '\0') {
			extension[j] = path[i];
			j++;
			i++;
		}

		r.append(searchMIME(extension));
		r.append("\r\n");
	}
	r.append("\r\n");
	return r;
}

string HttpdServer::searchMIME(string extension)
{
	auto res = MIME_map.find(extension);
	if(res == MIME_map.end()) {
		return "application/octet-stream";
	}
	return res->second;
}

void HttpdServer::setUpMIMEMap(char *path)
{
	ifstream in(path);

	string line;
	while (getline(in, line))
	{
		int i = 0;
		while(line[i] != ' ') {
			++i;
		}
		string ext = line.substr(0, i);
		while (line[i] == ' ')
		{
			++i;
		}
		string content = line.substr(i, line.length() - i);
		MIME_map.insert(make_pair<>(ext, content));
	}
}
