#include "server_app.hpp"

#include "conversion.hpp"
#include "protocol.hpp"

#include <arpa/inet.h>
#include <chrono>
#include <csignal>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <thread>
#include <unistd.h>

namespace netcourse {

namespace {

ServerApp* g_server_instance = nullptr;

bool sendAll(int socketFd, const std::string& payload) {
    std::size_t sent = 0;
    while (sent < payload.size()) {
        const ssize_t written = send(socketFd, payload.data() + sent, payload.size() - sent, MSG_NOSIGNAL);
        if (written <= 0) {
            return false;
        }
        sent += static_cast<std::size_t>(written);
    }
    return true;
}

bool readLine(int socketFd, std::string& line) {
    line.clear();
    char ch = '\0';
    while (true) {
        const ssize_t received = recv(socketFd, &ch, 1, 0);
        if (received == 0) {
            return false;
        }
        if (received < 0) {
            if (errno == EINTR) {
                continue;
            }
            return false;
        }
        if (ch == '\n') {
            return true;
        }
        if (ch != '\r') {
            line.push_back(ch);
        }
    }
}

long long parseLongLong(const std::string& text) {
    std::size_t processed = 0;
    const long long value = std::stoll(text, &processed);
    if (processed != text.size()) {
        throw std::runtime_error("Invalid integer");
    }
    return value;
}

int parseInt(const std::string& text) {
    std::size_t processed = 0;
    const int value = std::stoi(text, &processed);
    if (processed != text.size()) {
        throw std::runtime_error("Invalid integer");
    }
    return value;
}

void signalHandler(int) {
    if (g_server_instance != nullptr) {
        g_server_instance->stop();
    }
}

std::string buildPeer(const sockaddr_in& address) {
    char buffer[INET_ADDRSTRLEN] = {};
    inet_ntop(AF_INET, &address.sin_addr, buffer, sizeof(buffer));
    std::ostringstream stream;
    stream << buffer << ':' << ntohs(address.sin_port);
    return stream.str();
}

}  // namespace

ServerApp::ServerApp(ServerConfig config)
    : config_(std::move(config)),
      user_store_(config_.user_db_path),
      listen_socket_(-1),
      running_(true),
      active_clients_(0) {
    std::filesystem::create_directories(std::filesystem::path(config_.log_path).parent_path());
    log_stream_.open(config_.log_path, std::ios::app);
}

ServerApp::~ServerApp() {
    stop();
}

int ServerApp::run() {
    listen_socket_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_socket_ < 0) {
        std::cerr << "Cannot create socket\n";
        return 1;
    }

    const int reuse = 1;
    setsockopt(listen_socket_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse));

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(static_cast<uint16_t>(config_.port));
    if (inet_pton(AF_INET, config_.host.c_str(), &serverAddress.sin_addr) <= 0) {
        std::cerr << "Invalid host address\n";
        return 1;
    }

    if (bind(listen_socket_, reinterpret_cast<sockaddr*>(&serverAddress), sizeof(serverAddress)) < 0) {
        std::cerr << "Cannot bind socket\n";
        return 1;
    }

    if (listen(listen_socket_, config_.max_clients) < 0) {
        std::cerr << "Cannot listen on socket\n";
        return 1;
    }

    g_server_instance = this;
    std::signal(SIGINT, signalHandler);
    std::signal(SIGPIPE, SIG_IGN);
    writeLog("Server started on " + config_.host + ":" + std::to_string(config_.port));
    acceptLoop();
    writeLog("Server stopped");
    return 0;
}

void ServerApp::stop() {
    const bool wasRunning = running_.exchange(false);
    if (wasRunning && listen_socket_ >= 0) {
        close(listen_socket_);
        listen_socket_ = -1;
    }
}

void ServerApp::acceptLoop() {
    while (running_) {
        sockaddr_in clientAddress{};
        socklen_t clientLength = sizeof(clientAddress);
        const int clientSocket = accept(listen_socket_, reinterpret_cast<sockaddr*>(&clientAddress), &clientLength);
        if (clientSocket < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }

        if (active_clients_.load() >= config_.max_clients) {
            sendBusyError(clientSocket);
            close(clientSocket);
            continue;
        }

        active_clients_.fetch_add(1);
        const std::string peer = buildPeer(clientAddress);
        std::thread(&ServerApp::handleClient, this, clientSocket, peer).detach();
    }
}

void ServerApp::handleClient(int clientSocket, std::string peer) {
    writeLog("Client connected: " + peer);
    if (!sendAll(clientSocket, joinMessage({"OK", "CONNECTED", "Connection established"}))) {
        writeLog("Failed to send greeting to " + peer);
        close(clientSocket);
        active_clients_.fetch_sub(1);
        return;
    }

    bool authenticated = false;
    std::string username;
    std::string line;

    while (readLine(clientSocket, line)) {
        const auto parts = splitMessage(line);
        if (parts.empty()) {
            sendAll(clientSocket, joinMessage({"ERROR", "BAD_REQUEST", "Empty request"}));
            continue;
        }

        const std::string& command = parts[0];

        if (command == "REGISTER") {
            if (parts.size() != 3) {
                sendAll(clientSocket, joinMessage({"ERROR", "BAD_REQUEST", "REGISTER requires username and password"}));
                continue;
            }

            std::string error;
            if (!user_store_.registerUser(parts[1], parts[2], error)) {
                sendAll(clientSocket, joinMessage({"ERROR", "REGISTER_FAILED", error}));
                writeLog("Registration failed for " + parts[1] + " from " + peer + ": " + error);
                continue;
            }

            authenticated = true;
            username = parts[1];
            sendAll(clientSocket, joinMessage({"OK", "REGISTERED", "User registered and authenticated"}));
            writeLog("User registered: " + username + " from " + peer);
            continue;
        }

        if (command == "LOGIN") {
            if (parts.size() != 3) {
                sendAll(clientSocket, joinMessage({"ERROR", "BAD_REQUEST", "LOGIN requires username and password"}));
                continue;
            }

            std::string error;
            if (!user_store_.authenticate(parts[1], parts[2], error)) {
                sendAll(clientSocket, joinMessage({"ERROR", "AUTH_FAILED", error}));
                writeLog("Authentication failed for " + parts[1] + " from " + peer + ": " + error);
                continue;
            }

            authenticated = true;
            username = parts[1];
            sendAll(clientSocket, joinMessage({"OK", "AUTHENTICATED", "Authentication completed"}));
            writeLog("User authenticated: " + username + " from " + peer);
            continue;
        }

        if (command == "CALC") {
            if (parts.size() != 3) {
                sendAll(clientSocket, joinMessage({"ERROR", "BAD_REQUEST", "CALC requires number and bit width"}));
                continue;
            }
            if (!authenticated) {
                sendAll(clientSocket, joinMessage({"ERROR", "NOT_AUTHENTICATED", "Authenticate before calculations"}));
                continue;
            }

            try {
                const long long number = parseLongLong(parts[1]);
                const int bits = parseInt(parts[2]);
                const ConversionResult result = convertToCodes(number, bits);
                sendAll(clientSocket, joinMessage(
                    {"OK", "RESULT", "Calculation completed", result.reverse_code, result.additional_code}
                ));
                writeLog(
                    "Calculation by " + username + " from " + peer + ": number=" + std::to_string(number)
                    + ", bits=" + std::to_string(bits) + ", reverse=" + result.reverse_code
                    + ", additional=" + result.additional_code
                );
            } catch (const std::exception& ex) {
                sendAll(clientSocket, joinMessage({"ERROR", "CALC_FAILED", ex.what()}));
                writeLog("Calculation failed for " + username + " from " + peer + ": " + ex.what());
            }
            continue;
        }

        if (command == "QUIT") {
            sendAll(clientSocket, joinMessage({"OK", "BYE", "Connection closed"}));
            break;
        }

        sendAll(clientSocket, joinMessage({"ERROR", "UNKNOWN_COMMAND", "Unknown command"}));
    }

    writeLog("Client disconnected: " + peer);
    close(clientSocket);
    active_clients_.fetch_sub(1);
}

void ServerApp::sendBusyError(int clientSocket) const {
    sendAll(clientSocket, joinMessage({"ERROR", "SERVER_BUSY", "Maximum number of clients reached"}));
}

void ServerApp::writeLog(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    if (log_stream_.is_open()) {
        log_stream_ << '[' << timestamp() << "] " << message << '\n';
        log_stream_.flush();
    }
}

std::string ServerApp::timestamp() const {
    const auto now = std::chrono::system_clock::now();
    const std::time_t current = std::chrono::system_clock::to_time_t(now);
    std::tm tm{};
    localtime_r(&current, &tm);

    std::ostringstream stream;
    stream << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return stream.str();
}

}  // namespace netcourse
