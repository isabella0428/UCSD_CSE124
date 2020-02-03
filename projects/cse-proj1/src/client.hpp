# include <unordered_map>

#ifndef CLIENT_HPP
#define CLIENT_HPP

class Client
{
public:
    int  connect_to_server(int port_num);
    void sendRequest(char *message, int socket_id);
    void receive(int socket_id, std::string path);
    void saveFile(std::string filename, std::string file, std::string path);

protected:
    std::unordered_map<std::string, std::string> MIME_Map;
};

#endif //CLIENT_HPP
