#include "client_lib.hpp"

#include <arpa/inet.h>
#include <cassert>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <thread>
#include <unistd.h>

namespace {

class ServerProcess {
public:
    ServerProcess(std::string executable, int port, std::string userDb, std::string logFile)
        : pid_(-1) {
        pid_ = fork();
        if (pid_ < 0) {
            throw std::runtime_error("fork failed");
        }

        if (pid_ == 0) {
            execl(
                executable.c_str(),
                executable.c_str(),
                std::to_string(port).c_str(),
                userDb.c_str(),
                logFile.c_str(),
                nullptr
            );
            _exit(1);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    ~ServerProcess() {
        if (pid_ > 0) {
            kill(pid_, SIGTERM);
            waitpid(pid_, nullptr, 0);
        }
    }

private:
    pid_t pid_;
};

std::filesystem::path executablePath(const char* argv0, const std::string& fileName) {
    return std::filesystem::absolute(std::filesystem::path(argv0)).parent_path() / fileName;
}

std::string readLine(int socketFd) {
    std::string line;
    char ch = '\0';
    while (true) {
        const ssize_t received = ::recv(socketFd, &ch, 1, 0);
        if (received <= 0) {
            throw std::runtime_error("recv failed");
        }
        if (ch == '\n') {
            return line;
        }
        if (ch != '\r') {
            line.push_back(ch);
        }
    }
}

std::string sendRaw(const std::string& host, int port, const std::string& request) {
    const int socketFd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0) {
        throw std::runtime_error("socket failed");
    }

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port));
    if (::inet_pton(AF_INET, host.c_str(), &address.sin_addr) <= 0) {
        ::close(socketFd);
        throw std::runtime_error("inet_pton failed");
    }

    if (::connect(socketFd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        ::close(socketFd);
        throw std::runtime_error("connect failed");
    }

    const auto greeting = netcourse::parseResponse(readLine(socketFd));
    if (!greeting.ok) {
        ::close(socketFd);
        throw std::runtime_error("server rejected connection");
    }

    std::size_t sent = 0;
    while (sent < request.size()) {
        const ssize_t written = ::send(socketFd, request.data() + sent, request.size() - sent, MSG_NOSIGNAL);
        if (written <= 0) {
            ::close(socketFd);
            throw std::runtime_error("send failed");
        }
        sent += static_cast<std::size_t>(written);
    }

    const std::string response = readLine(socketFd);
    ::close(socketFd);
    return response;
}

bool containsLineWith(const std::filesystem::path& file, const std::string& needle) {
    std::ifstream input(file);
    std::string line;
    while (std::getline(input, line)) {
        if (line.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 1) {
        return 1;
    }

    const auto buildDir = std::filesystem::absolute(std::filesystem::path(argv[0])).parent_path();
    const auto serverPath = executablePath(argv[0], "netcourse_server");
    const auto tempDir = buildDir / "acceptance_test_data";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);

    const auto userDb = tempDir / "users.db";
    const auto logFile = tempDir / "server.log";
    const int port = 9095;
    ServerProcess server(serverPath.string(), port, userDb.string(), logFile.string());

    netcourse::ClientConnection client("127.0.0.1", port);
    client.connectToServer();

    auto response = client.calculate(7, 8);
    assert(!response.ok);
    assert(response.code == "NOT_AUTHENTICATED");

    response = client.registerUser("user2", "pass2");
    assert(response.ok);

    response = client.calculate(5, 8);
    assert(response.ok);
    assert(response.fields.at(0) == "00000101");
    assert(response.fields.at(1) == "00000101");

    response = client.calculate(-5, 3);
    assert(!response.ok);
    assert(response.code == "CALC_FAILED");
    client.quit();

    netcourse::ClientConnection duplicate("127.0.0.1", port);
    duplicate.connectToServer();
    response = duplicate.registerUser("user2", "pass2");
    assert(!response.ok);
    assert(response.code == "REGISTER_FAILED");
    duplicate.quit();

    netcourse::ClientConnection loginClient("127.0.0.1", port);
    loginClient.connectToServer();
    response = loginClient.login("user2", "pass2");
    assert(response.ok);
    loginClient.quit();

    const auto rawBadRequest = netcourse::parseResponse(sendRaw("127.0.0.1", port, "CALC|1\n"));
    assert(!rawBadRequest.ok);
    assert(rawBadRequest.code == "BAD_REQUEST");

    const auto rawUnknown = netcourse::parseResponse(sendRaw("127.0.0.1", port, "PING\n"));
    assert(!rawUnknown.ok);
    assert(rawUnknown.code == "UNKNOWN_COMMAND");

    assert(std::filesystem::exists(userDb));
    assert(std::filesystem::exists(logFile));
    assert(containsLineWith(userDb, "user2:"));
    assert(containsLineWith(logFile, "User registered: user2"));
    assert(containsLineWith(logFile, "Calculation by user2"));
    assert(containsLineWith(logFile, "Authentication failed") || containsLineWith(logFile, "User authenticated: user2"));

    std::cout << "Acceptance test passed\n";
    return 0;
}
