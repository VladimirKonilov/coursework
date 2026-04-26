#include "client_lib.hpp"

#include <iostream>
#include <limits>
#include <string>

namespace {

// Унифицированный ввод строкового значения из консоли клиента.
std::string readText(const std::string& prompt) {
    std::cout << prompt;
    std::string value;
    std::getline(std::cin, value);
    return value;
}

// Ввод целого числа, которое затем будет переведено сервером
// в обратный и дополнительный код.
long long readNumber(const std::string& prompt) {
    while (true) {
        try {
            return std::stoll(readText(prompt));
        } catch (...) {
            std::cout << "Введите корректное целое число.\n";
        }
    }
}

// Ввод разрядности двоичного представления числа.
int readInt(const std::string& prompt) {
    while (true) {
        try {
            return std::stoi(readText(prompt));
        } catch (...) {
            std::cout << "Введите корректное целое число.\n";
        }
    }
}

// Отображение результата выполнения команды прикладного протокола.
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
        // Подключение клиента к серверу по TCP.
        client.connectToServer();

        std::cout << "Соединение с сервером установлено.\n";
        // Пока пользователь не прошел регистрацию или вход,
        // сервер не разрешает выполнять команду CALC.
        bool authenticated = false;
        while (!authenticated) {
            // Клиент предоставляет два режима, требуемые ТЗ:
            // регистрация нового пользователя и вход существующего.
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
                // Сценарий регистрации нового пользователя.
                response = client.registerUser(username, password);
            } else if (choice == "2") {
                // Сценарий входа уже существующего пользователя.
                response = client.login(username, password);
            } else {
                std::cout << "Неизвестная команда.\n";
                continue;
            }

            printResponse(response);
            authenticated = response.ok;
        }

        while (true) {
            // Клиент обеспечивает ввод числа и разрядности,
            // затем отправляет их на сервер.
            const long long number = readNumber("Введите число: ");
            const int bits = readInt("Введите разрядность: ");
            const auto response = client.calculate(number, bits);
            printResponse(response);
            if (response.ok && response.fields.size() >= 2) {
                // Клиент отображает обратный и дополнительный код,
                // полученные от серверной программы.
                std::cout << "Обратный код: " << response.fields[0] << '\n';
                std::cout << "Дополнительный код: " << response.fields[1] << '\n';
            }

            const std::string repeat = readText("Выполнить еще одно вычисление? (y/n): ");
            if (repeat != "y" && repeat != "Y") {
                // Перед завершением клиент уведомляет сервер о закрытии сеанса.
                client.quit();
                break;
            }
        }
    } catch (const std::exception& ex) {
        // Любые ошибки подключения, обмена или отказа сервера
        // выводятся пользователю в явном виде.
        std::cerr << "Ошибка клиента: " << ex.what() << '\n';
        return 1;
    }

    return 0;
}
