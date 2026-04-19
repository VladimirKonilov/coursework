#include "user_store.hpp"

#include "protocol.hpp"

#include <filesystem>
#include <fstream>
#include <sstream>

namespace netcourse {

namespace {

std::string fnv1a64(const std::string& input) {
    unsigned long long hash = 1469598103934665603ULL;
    for (unsigned char ch : input) {
        hash ^= ch;
        hash *= 1099511628211ULL;
    }

    std::ostringstream stream;
    stream << std::hex << hash;
    return stream.str();
}

}  // namespace

UserStore::UserStore(std::string path) : path_(std::move(path)) {
    load();
}

bool UserStore::registerUser(const std::string& username, const std::string& password, std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!isPrintableToken(username) || !isPrintableToken(password)) {
        error = "Username and password must not contain separators or control characters";
        return false;
    }
    if (users_.count(username) != 0U) {
        error = "User already exists";
        return false;
    }

    users_[username] = hashPassword(password);
    return save(error);
}

bool UserStore::authenticate(const std::string& username, const std::string& password, std::string& error) {
    std::lock_guard<std::mutex> lock(mutex_);

    const auto it = users_.find(username);
    if (it == users_.end()) {
        error = "User not found";
        return false;
    }
    if (it->second != hashPassword(password)) {
        error = "Invalid password";
        return false;
    }
    return true;
}

std::string UserStore::hashPassword(const std::string& password) const {
    return fnv1a64(password);
}

bool UserStore::load() {
    std::lock_guard<std::mutex> lock(mutex_);

    users_.clear();
    std::ifstream input(path_);
    if (!input.is_open()) {
        return true;
    }

    std::string line;
    while (std::getline(input, line)) {
        if (line.empty()) {
            continue;
        }
        const auto separator = line.find(':');
        if (separator == std::string::npos) {
            continue;
        }
        users_[line.substr(0, separator)] = line.substr(separator + 1);
    }
    return true;
}

bool UserStore::save(std::string& error) {
    std::filesystem::create_directories(std::filesystem::path(path_).parent_path());

    std::ofstream output(path_, std::ios::trunc);
    if (!output.is_open()) {
        error = "Cannot open user database file";
        return false;
    }

    for (const auto& [username, passwordHash] : users_) {
        output << username << ':' << passwordHash << '\n';
    }
    return true;
}

}  // namespace netcourse
