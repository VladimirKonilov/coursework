#pragma once

#include "protocol.hpp"

#include <string>

namespace netcourse {

class ClientConnection {
public:
    ClientConnection(std::string host, int port);
    ~ClientConnection();

    void connectToServer();
    Response registerUser(const std::string& username, const std::string& password);
    Response login(const std::string& username, const std::string& password);
    Response calculate(long long number, int bits);
    Response quit();

private:
    Response sendRequest(const std::vector<std::string>& parts);
    std::string readLine();

    std::string host_;
    int port_;
    int socket_;
};

}  // namespace netcourse
