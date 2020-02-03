# include <stdlib.h>
# include <unistd.h>
# include <iostream>
# include <string>

# include "client.hpp"

int main() {
    Client *client = new Client();
    int socket_id = client->connect_to_server(8080);

    std::string request = "GET ./sample_htdocs/UCSD_Seal.png HTTP/1.1";
    client->sendRequest((char *)request.c_str(), socket_id);
    client->receive(socket_id, "../download");
}