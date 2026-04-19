#include "server_app.hpp"

#include <iostream>
#include <string>

int main(int argc, char* argv[]) {
    netcourse::ServerConfig config;

    if (argc > 1) {
        config.port = std::stoi(argv[1]);
    }
    if (argc > 2) {
        config.user_db_path = argv[2];
    }
    if (argc > 3) {
        config.log_path = argv[3];
    }

    std::cout << "Server listening on " << config.host << ':' << config.port << '\n';
    netcourse::ServerApp server(config);
    return server.run();
}
