// include/common_utils.h
#ifndef COMMON_UTILS_H
#define COMMON_UTILS_H

#include <string>
#include <cstdlib> // for getenv
#include <iostream>

// 获取真实用户 (SUDO_USER 优先)
std::string get_real_user();

// 获取真实用户的家目录
std::string get_real_home_dir();

// 展开路径中的 ~ 符号
std::string expand_user_path(const std::string& path);

#endif // COMMON_UTILS_H