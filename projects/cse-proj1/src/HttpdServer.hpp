#ifndef HTTPDSERVER_HPP
#define HTTPDSERVER_HPP

#include "inih/INIReader.h"
#include "logger.hpp"

class HttpdServer {
public:
	HttpdServer(INIReader& t_config);

	void launch();
	void processRequest(int);
	void transmitFile(char*, int);
	void parseRequest(char*, char*);
	string parseResponse(string, char*);
	void setUpMIMEMap(char*);
	string searchMIME(string);

protected:
	INIReader& config;
	string port;
	string doc_root;
	int timeout;
	std::unordered_map<std::string, std::string> MIME_map;
};

#endif // HTTPDSERVER_HPP
