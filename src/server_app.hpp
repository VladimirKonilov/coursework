#pragma once

#include "user_store.hpp"

#include <atomic>
#include <fstream>
#include <mutex>
#include <string>

namespace netcourse {

struct ServerConfig {
    std::string host = "127.0.0.1";
    int port = 9090;
    std::string user_db_path = "data/users.db";
    std::string log_path = "logs/server.log";
    int max_clients = 3;
};

class ServerApp {
public:
    explicit ServerApp(ServerConfig config);
    ~ServerApp();

    int run();
    void stop();

private:
    void acceptLoop();
    void handleClient(int clientSocket, std::string peer);
    void sendBusyError(int clientSocket) const;
    void writeLog(const std::string& message);
    std::string timestamp() const;

    ServerConfig config_;
    UserStore user_store_;
    int listen_socket_;
    std::atomic<bool> running_;
    std::atomic<int> active_clients_;
    std::mutex log_mutex_;
    std::ofstream log_stream_;
};

}  // namespace netcourse
