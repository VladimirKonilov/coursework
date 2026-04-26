#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

namespace netcourse {

// Файловое хранилище зарегистрированных пользователей.
// Используется сервером для регистрации и аутентификации.
class UserStore {
public:
    // path - путь к файлу, где хранится база пользователей.
    explicit UserStore(std::string path);

    // Регистрирует нового пользователя и сохраняет его в файл.
    bool registerUser(const std::string& username, const std::string& password, std::string& error);
    // Проверяет логин и пароль существующего пользователя.
    bool authenticate(const std::string& username, const std::string& password, std::string& error);

private:
    // Вычисляет хеш пароля для хранения в файле.
    std::string hashPassword(const std::string& password) const;
    // Загружает базу пользователей с диска.
    bool load();
    // Сохраняет базу пользователей на диск.
    bool save(std::string& error);

    // Путь к файлу базы пользователей.
    std::string path_;
    // Отображение "логин -> хеш пароля".
    std::unordered_map<std::string, std::string> users_;
    // Защита базы пользователей при многопоточном доступе.
    std::mutex mutex_;
};

}  // namespace netcourse
