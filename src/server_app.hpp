#pragma once

#include "user_store.hpp"

#include <atomic>
#include <fstream>
#include <mutex>
#include <string>

namespace netcourse {

// Параметры конфигурации серверной части.
struct ServerConfig {
    std::string host = "127.0.0.1";
    int port = 9090;
    std::string user_db_path = "data/users.db";
    std::string log_path = "logs/server.log";
    int max_clients = 3;
};

// Основной класс сервера.
// Инкапсулирует сетевое взаимодействие, аутентификацию,
// ограничение числа клиентов и журналирование операций.
class ServerApp {
public:
    // Создает сервер с указанной конфигурацией.
    explicit ServerApp(ServerConfig config);
    ~ServerApp();

    // Полный жизненный цикл сервера: создание сокета, bind, listen, accept.
    int run();
    // Останавливает сервер по сигналу или по завершению программы.
    void stop();

private:
    // Основной цикл ожидания и приема клиентов.
    void acceptLoop();
    // Обслуживает конкретного клиента в отдельном потоке.
    void handleClient(int clientSocket, std::string peer);
    // Возвращает ошибку при превышении лимита подключений.
    void sendBusyError(int clientSocket) const;
    // Добавляет запись в журнал сервера.
    void writeLog(const std::string& message);
    // Формирует метку времени для журнала.
    std::string timestamp() const;

    ServerConfig config_;
    // Хранилище зарегистрированных пользователей.
    UserStore user_store_;
    // Сокет, прослушивающий входящие TCP-подключения.
    int listen_socket_;
    // Флаг работы сервера.
    std::atomic<bool> running_;
    // Число клиентов, обслуживаемых в данный момент.
    std::atomic<int> active_clients_;
    // Синхронизация параллельной записи в журнал.
    std::mutex log_mutex_;
    // Поток вывода в лог-файл.
    std::ofstream log_stream_;
};

}  // namespace netcourse
