#include "client_lib.hpp"

#include <cassert>
#include <chrono>
#include <csignal>
#include <filesystem>
#include <iostream>
#include <stdexcept>
#include <string>
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

}  // namespace

int main(int argc, char* argv[]) {
    if (argc < 1) {
        return 1;
    }

    const auto buildDir = std::filesystem::absolute(std::filesystem::path(argv[0])).parent_path();
    const auto serverPath = executablePath(argv[0], "netcourse_server");
    const auto tempDir = buildDir / "smoke_test_data";
    std::filesystem::remove_all(tempDir);
    std::filesystem::create_directories(tempDir);

    const int port = 9094;
    ServerProcess server(serverPath.string(), port, (tempDir / "users.db").string(), (tempDir / "server.log").string());

    netcourse::ClientConnection client("127.0.0.1", port);
    client.connectToServer();
    auto response = client.registerUser("user1", "pass1");
    assert(response.ok);

    response = client.calculate(-5, 8);
    assert(response.ok);
    assert(response.fields.size() == 2);
    assert(response.fields[0] == "11111010");
    assert(response.fields[1] == "11111011");
    client.quit();

    netcourse::ClientConnection badClient("127.0.0.1", port);
    badClient.connectToServer();
    response = badClient.login("user1", "wrong");
    assert(!response.ok);
    badClient.quit();

    netcourse::ClientConnection c1("127.0.0.1", port);
    netcourse::ClientConnection c2("127.0.0.1", port);
    netcourse::ClientConnection c3("127.0.0.1", port);
    c1.connectToServer();
    c2.connectToServer();
    c3.connectToServer();

    netcourse::ClientConnection c4("127.0.0.1", port);
    try {
        c4.connectToServer();
        assert(false);
    } catch (const std::exception& ex) {
        assert(std::string(ex.what()).find("SERVER_BUSY") != std::string::npos);
    }

    std::cout << "Smoke test passed\n";
    return 0;
}
