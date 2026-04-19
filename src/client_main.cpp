#include "client_lib.hpp"

#include <iostream>
#include <limits>
#include <string>

namespace {

std::string readText(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

long long readNumber(const std::string& prompt) {
    while (true) {
        try {
            return std::stoll(readText(prompt));
        } catch (...) {
            std::cout << "Введите корректное целое число.\n";
        }
    }
}

int readInt(const std::string& prompt) {
    while (true) {
        try {
            return std::stoi(readText(prompt));
        } catch (...) {
            std::cout << "Введите корректное целое число.\n";
        }
    }
}

void printResponse(const netcourse::Response& response) {
    if (response.ok) {
        std::cout << "Успех: " << response.message << '\n';
    } else {
        std::cout << "Ошибка [" << response.code << "]: " << response.message << '\n';
    }
}

}  // namespace

int main(int argc, char* argv[]) {
    const std::string host = (argc > 1) ? argv[1] : "127.0.0.1";
    const int port = (argc > 2) ? std::stoi(argv[2]) : 9090;

    try {
        netcourse::ClientConnection client(host, port);
        client.connectToServer();

        std::cout << "Соединение с сервером установлено.\n";
        bool authenticated = false;
        while (!authenticated) {
            std::cout << "1 - регистрация\n2 - вход\n0 - выход\n";
            const std::string choice = readText("Выберите действие: ");
            if (choice == "0") {
                client.quit();
                return 0;
            }

            const std::string username = readText("Логин: ");
            const std::string password = readText("Пароль: ");

            netcourse::Response response{};
            if (choice == "1") {
                response = client.registerUser(username, password);
            } else if (choice == "2") {
                response = client.login(username, password);
            } else {
                std::cout << "Неизвестная команда.\n";
                continue;
            }

            printResponse(response);
            authenticated = response.ok;
        }

        while (true) {
            const long long number = readNumber("Введите число: ");
            const int bits = readInt("Введите разрядность: ");
            const auto response = client.calculate(number, bits);
            printResponse(response);
            if (response.ok && response.fields.size() >= 2) {
                std::cout << "Обратный код: " << response.fields[0] << '\n';
                std::cout << "Дополнительный код: " << response.fields[1] << '\n';
            }

            const std::string repeat = readText("Выполнить еще одно вычисление? (y/n): ");
            if (repeat != "y" && repeat != "Y") {
                client.quit();
                break;
            }
        }
    } catch (const std::exception& ex) {
        std::cerr << "Ошибка клиента: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
