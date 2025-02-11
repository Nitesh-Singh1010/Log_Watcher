#include "web_socket_server.hpp"
#include <iostream>
#include <string>

int main(int argc, char *argv[])
{
    if (argc != 4)
    {
        std::cerr << "Usage: " << argv[0] << " <websocket_port> <http_port> <log_file>\n";
        return 1;
    }

    try
    {
        uint16_t ws_port = static_cast<uint16_t>(std::stoi(argv[1]));
        uint16_t http_port = static_cast<uint16_t>(std::stoi(argv[2]));
        std::string log_file = argv[3];

        WebSocketServer server(ws_port, http_port, log_file);
        std::cout << "Log Watcher started\n";
        std::cout << "Watching file: " << log_file << "\n";

        server.run();
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }

    return 0;
}