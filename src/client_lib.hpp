#pragma once

#include "protocol.hpp"

#include <string>

namespace netcourse {

// Класс клиентского подключения к серверу.
// Через него выполняются все команды прикладного протокола.
class ClientConnection {
public:
    // Сохраняет параметры подключения к серверу.
    ClientConnection(std::string host, int port);
    ~ClientConnection();

    // Выполняет TCP-подключение и получает приветствие сервера.
    void connectToServer();
    // Отправляет команду регистрации пользователя.
    Response registerUser(const std::string& username, const std::string& password);
    // Отправляет команду аутентификации пользователя.
    Response login(const std::string& username, const std::string& password);
    // Отправляет число и разрядность для вычисления кодов.
    Response calculate(long long number, int bits);
    // Завершает работу клиента.
    Response quit();

private:
    // Общая функция отправки команды и чтения ответа.
    Response sendRequest(const std::vector<std::string>& parts);
    // Читает одну строку ответа сервера.
    std::string readLine();

    // IP-адрес или имя хоста сервера.
    std::string host_;
    // TCP-порт сервера.
    int port_;
    // Клиентский сокет активного соединения.
    int socket_;
};

}  // namespace netcourse
