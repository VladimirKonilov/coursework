#include "client_lib.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace netcourse {

namespace {

void sendAll(int socketFd, const std::string& payload) {
    std::size_t sent = 0;
    while (sent < payload.size()) {
        const ssize_t written = ::send(socketFd, payload.data() + sent, payload.size() - sent, MSG_NOSIGNAL);
        if (written <= 0) {
            throw std::runtime_error("Failed to send request to server");
        }
        sent += static_cast<std::size_t>(written);
    }
}

}  // namespace

ClientConnection::ClientConnection(std::string host, int port)
    : host_(std::move(host)), port_(port), socket_(-1) {}

ClientConnection::~ClientConnection() {
    if (socket_ >= 0) {
        ::close(socket_);
    }
}

void ClientConnection::connectToServer() {
    socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        throw std::runtime_error(std::string("Cannot create client socket: ") + std::strerror(errno));
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port_));
    if (inet_pton(AF_INET, host_.c_str(), &address.sin_addr) <= 0) {
        throw std::runtime_error("Invalid server address");
    }

    if (::connect(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error(std::string("Cannot connect to server: ") + std::strerror(errno));
    }

    const Response response = parseResponse(readLine());
    if (!response.ok) {
        throw std::runtime_error("Server rejected connection: " + response.code + " - " + response.message);
    }
}

Response ClientConnection::registerUser(const std::string& username, const std::string& password) {
    return sendRequest({"REGISTER", username, password});
}

Response ClientConnection::login(const std::string& username, const std::string& password) {
    return sendRequest({"LOGIN", username, password});
}

Response ClientConnection::calculate(long long number, int bits) {
    return sendRequest({"CALC", std::to_string(number), std::to_string(bits)});
}

Response ClientConnection::quit() {
    return sendRequest({"QUIT"});
}

Response ClientConnection::sendRequest(const std::vector<std::string>& parts) {
    sendAll(socket_, joinMessage(parts));
    return parseResponse(readLine());
}

std::string ClientConnection::readLine() {
    std::string line;
    char ch = '\0';
    while (true) {
        const ssize_t received = ::recv(socket_, &ch, 1, 0);
        if (received == 0) {
            throw std::runtime_error("Server closed the connection");
        }
        if (received < 0) {
            throw std::runtime_error(std::string("Failed to read response from server: ") + std::strerror(errno));
        }
        if (ch == '\n') {
            return line;
        }
        if (ch != '\r') {
            line.push_back(ch);
        }
    }
}

}  // namespace netcourse
