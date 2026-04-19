#pragma once

#include <mutex>
#include <string>
#include <unordered_map>

namespace netcourse {

class UserStore {
public:
    explicit UserStore(std::string path);

    bool registerUser(const std::string& username, const std::string& password, std::string& error);
    bool authenticate(const std::string& username, const std::string& password, std::string& error);

private:
    std::string hashPassword(const std::string& password) const;
    bool load();
    bool save(std::string& error);

    std::string path_;
    std::unordered_map<std::string, std::string> users_;
    std::mutex mutex_;
};

}  // namespace netcourse
