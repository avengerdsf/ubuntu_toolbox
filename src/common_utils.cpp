// src/common_utils.cpp
#include "common_utils.h"


std::string get_real_user() {
    const char* sudo_user = std::getenv("SUDO_USER");
    if (sudo_user != nullptr) return std::string(sudo_user);
    
    const char* user = std::getenv("USER");
    if (user != nullptr) return std::string(user);
    
    return "root";
}

std::string get_real_home_dir() {
    std::string user = get_real_user();
    if (user == "root") {
        // 如果真的是 root 登录，尝试获取 HOME 环境变量
        const char* home = std::getenv("HOME");
        return home ? std::string(home) : "/root";
    }
    return "/home/" + user;
}

std::string expand_user_path(const std::string& path) {
    if (path.empty()) return path;

    // 如果路径以 ~ 开头
    if (path[0] == '~') {
        std::string home = get_real_home_dir();
        
        // 情况 1: 只有 "~"
        if (path.size() == 1) {
            return home;
        }
        // 情况 2: 标准 "~/"
        else if (path[1] == '/') {
            return home + path.substr(1);
        }
        // 情况 3: 手误 "~~/" 或者是某些特殊情况
        else if (path[1] == '~' && path.size() > 2 && path[2] == '/') {
            return home + path.substr(2);
        }
    }
    
    return path;
}