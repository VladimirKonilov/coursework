#include "client_lib.hpp"

#include <arpa/inet.h>
#include <cerrno>
#include <cstring>
#include <stdexcept>
#include <sys/socket.h>
#include <unistd.h>

namespace netcourse {

namespace {

// Отправляет сформированную клиентом команду на сервер целиком.
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
    // Закрытие сокета при уничтожении объекта предотвращает утечки ресурсов.
    if (socket_ >= 0) {
        ::close(socket_);
    }
}

void ClientConnection::connectToServer() {
    // Клиент также использует TCP-сокет, как и требуется по ТЗ.
    socket_ = ::socket(AF_INET, SOCK_STREAM, 0);
    if (socket_ < 0) {
        throw std::runtime_error(std::string("Cannot create client socket: ") + std::strerror(errno));
    }

    sockaddr_in address{};
    // Настройка адреса удаленного сервера, к которому подключается клиент.
    address.sin_family = AF_INET;
    address.sin_port = htons(static_cast<uint16_t>(port_));
    if (inet_pton(AF_INET, host_.c_str(), &address.sin_addr) <= 0) {
        throw std::runtime_error("Invalid server address");
    }

    if (::connect(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        throw std::runtime_error(std::string("Cannot connect to server: ") + std::strerror(errno));
    }

    // После установления TCP-соединения клиент обязан получить
    // прикладное подтверждение CONNECTED либо отказ SERVER_BUSY.
    const Response response = parseResponse(readLine());
    if (!response.ok) {
        // Важный сценарий ТЗ: сервер может немедленно отказать,
        // если превышено максимальное число клиентов.
        throw std::runtime_error("Server rejected connection: " + response.code + " - " + response.message);
    }
}

Response ClientConnection::registerUser(const std::string& username, const std::string& password) {
    // Команда регистрации нового пользователя.
    return sendRequest({"REGISTER", username, password});
}

Response ClientConnection::login(const std::string& username, const std::string& password) {
    // Команда аутентификации пользователя.
    return sendRequest({"LOGIN", username, password});
}

Response ClientConnection::calculate(long long number, int bits) {
    // Команда отправки числа и его разрядности на сервер для вычисления кодов.
    return sendRequest({"CALC", std::to_string(number), std::to_string(bits)});
}

Response ClientConnection::quit() {
    // Завершение сеанса работы клиента.
    return sendRequest({"QUIT"});
}

Response ClientConnection::sendRequest(const std::vector<std::string>& parts) {
    // Клиент формирует сообщение прикладного протокола и ожидает ответ сервера.
    sendAll(socket_, joinMessage(parts));
    return parseResponse(readLine());
}

std::string ClientConnection::readLine() {
    // Чтение ответа сервера в формате "одна строка - одно сообщение".
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
