#include "server_app.hpp"

#include <iostream>
#include <string>

// Точка входа серверной программы.
// Из аргументов командной строки могут быть переданы:
// 1) порт;
// 2) путь к файлу базы пользователей;
// 3) путь к файлу журнала.
int main(int argc, char* argv[]) {
    netcourse::ServerConfig config;

    // При отсутствии аргументов используются значения по умолчанию
    // из структуры ServerConfig.
    if (argc > 1) {
        config.port = std::stoi(argv[1]);
    }
    if (argc > 2) {
        config.user_db_path = argv[2];
    }
    if (argc > 3) {
        config.log_path = argv[3];
    }

    // Сообщение в консоль показывает, что сервер готов ожидать подключения.
    std::cout << "Server listening on " << config.host << ':' << config.port << '\n';
    netcourse::ServerApp server(config);
    return server.run();
}
