#include "sub_menu.h"
#include "unit_color.h"
#include "common_utils.h"

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>
#include <limits>
#include <cstdio>
#include <memory>
#include <array>
#include <sstream>
#include <termios.h> // 用于控制终端输入模式
#include <algorithm> // for find

namespace fs = std::filesystem;

// ==========================================
// 全局变量 & 基础工具
// ==========================================
std::string FOUND_CONDA_PATH = "";
std::string CURRENT_USER = "";

std::string find_conda_executable() {
    if (!FOUND_CONDA_PATH.empty()) return FOUND_CONDA_PATH;
    std::string user = get_real_user();
    CURRENT_USER = user;
    std::string home_dir = (user == "root") ? "/root" : "/home/" + user;
    std::vector<std::string> candidates = {
        home_dir + "/anaconda3/bin/conda",
        home_dir + "/miniconda3/bin/conda",
        home_dir + "/miniforge3/bin/conda",
        home_dir + "/opt/anaconda3/bin/conda",
        "/opt/anaconda3/bin/conda",
        "/usr/local/bin/conda"
    };
    for (const auto& path : candidates) {
        if (access(path.c_str(), X_OK) == 0) {
            FOUND_CONDA_PATH = path;
            return FOUND_CONDA_PATH;
        }
    }
    if (system("which conda > /dev/null 2>&1") == 0) {
        FOUND_CONDA_PATH = "conda";
        return "conda";
    }
    return "";
}

std::string exec_cmd_get_output(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return "";
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

std::vector<std::string> get_conda_envs() {
    std::vector<std::string> envs;
    std::string cmd = FOUND_CONDA_PATH + " env list";
    std::string output = exec_cmd_get_output(cmd);
    std::stringstream ss(output);
    std::string line;
    while (std::getline(ss, line)) {
        if (line.empty() || line[0] == '#') continue;
        std::stringstream line_ss(line);
        std::string env_name;
        line_ss >> env_name;
        if (!env_name.empty()) envs.push_back(env_name);
    }
    return envs;
}

static bool run_cmd(const std::string& cmd) {
    int ret = system(cmd.c_str());
    return (ret == 0);
}

// ==========================================
// 交互式多选菜单模块 (参考你的模板代码)
// ==========================================

// 获取单个按键输入，不回显
int getch() {
    struct termios oldt, newt;
    int ch;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ICANON | ECHO); // 关闭缓冲和回显
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    ch = getchar();
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return ch;
}

// 绘制界面
void draw_checklist(const std::vector<std::string>& options, const std::vector<bool>& selected, int highlight_idx) {
    // 移动光标到左上角 (不清屏整个终端，只重绘菜单区域，防止闪烁太严重)
    // 这里为了简单，我们用清屏命令，如果想保留上方Log，可以去掉 \033[2J
    std::cout << "\033[2J\033[H"; 
    
    std::cout << CYAN << "==========================================" << NC << "\n";
    std::cout << CYAN << "      " << RUL_YELLOW << "Multi-Select Conda Environments" << CYAN << "     " << NC << "\n";
    std::cout << CYAN << "==========================================" << NC << "\n";
    std::cout << "Controls: " << RUL_YELLOW << "↑/↓" << NC << " Navigate, " 
              << RUL_YELLOW << "SPACE" << NC << " Toggle, " 
              << RUL_YELLOW << "ENTER" << NC << " Confirm, " 
              << RUL_YELLOW << "q" << NC << " Cancel\n";
    std::cout << "------------------------------------------\n";

    for (size_t i = 0; i < options.size(); ++i) {
        std::string check_mark = selected[i] ? "[" + GREEN + "X" + NC + "]" : "[ ]";
        std::string line_start = (i == highlight_idx) ? PURPLE + "> " : "  ";
        std::string line_end = NC; // Reset color
        
        // 高亮行背景 (可选)
        if (i == highlight_idx) std::cout << "\033[7m"; // 反转颜色

        std::cout << line_start << check_mark << " " << options[i];
        
        // 补全行尾空白以便背景色铺满 (可选)
        std::cout << "\033[K"; 
        
        if (i == highlight_idx) std::cout << "\033[0m"; // 恢复颜色
        
        std::cout << "\n";
    }
    std::cout << "------------------------------------------\n";
}

// 核心选择函数：返回用户选择的环境列表
std::vector<std::string> interactive_select_envs(const std::vector<std::string>& all_envs) {
    if (all_envs.empty()) return {};

    std::vector<bool> selected(all_envs.size(), false);
    int highlight_idx = 0;
    std::vector<std::string> result;

    std::cout << "\033[?25l"; // 隐藏光标

    while (true) {
        draw_checklist(all_envs, selected, highlight_idx);

        int c = getch();

        if (c == 'q' || c == 'Q') {
            // 用户取消
            result.clear();
            break;
        } else if (c == '\n' || c == '\r') {
            // 用户确认 (Enter)
            for (size_t i = 0; i < all_envs.size(); ++i) {
                if (selected[i]) {
                    result.push_back(all_envs[i]);
                }
            }
            break;
        } else if (c == ' ') {
            // 空格切换选中状态
            selected[highlight_idx] = !selected[highlight_idx];
        } else if (c == 27) { // 处理箭头键 (ESC [ A / B)
            if (getch() == '[') {
                switch (getch()) {
                    case 'A': // Up
                        highlight_idx = (highlight_idx - 1 + all_envs.size()) % all_envs.size();
                        break;
                    case 'B': // Down
                        highlight_idx = (highlight_idx + 1) % all_envs.size();
                        break;
                }
            }
        }
    }

    std::cout << "\033[?25h"; // 恢复光标
    std::cout << "\033[2J\033[H"; // 退出前再次清屏
    return result;
}

// ==========================================
// 业务逻辑：批量备份
// ==========================================
void backup_conda_env() {
    if (FOUND_CONDA_PATH.empty()) {
        if (find_conda_executable().empty()) {
            std::cout << RED << "[ERROR] Conda path unknown." << NC << std::endl;
            return;
        }
    }

    std::cout << RUL_YELLOW << "[INFO] Fetching environment list..." << NC << std::endl;
    std::vector<std::string> envs = get_conda_envs();

    if (envs.empty()) {
        std::cout << RED << "[ERROR] No environments found." << NC << std::endl;
        // 等待一下，否则清屏太快看不见错误
        system("sleep 2"); 
        return;
    }

    // === 调用交互式选择器 ===
    std::vector<std::string> target_envs = interactive_select_envs(envs);

    if (target_envs.empty()) {
        std::cout << RUL_YELLOW << "[INFO] No environments selected or cancelled." << NC << std::endl;
        return;
    }

    // === 显示选中项并确认 ===
    std::cout << CYAN << "\n--- Batch Backup Configuration ---" << NC << std::endl;
    std::cout << "Selected Environments (" << target_envs.size() << "):" << std::endl;
    for (const auto& env : target_envs) {
        std::cout << " - " << GREEN << env << NC << std::endl;
    }

    // === 输入保存目录 ===
    std::cout << "\nEnter destination directory (default: ./conda_backups): ";
    std::string save_dir;
    std::string input_line;
    std::getline(std::cin, input_line);

    if (input_line.empty()) {
        save_dir = "./conda_backups";
    } else {
        save_dir = expand_user_path(input_line);
    }

    std::cout << BLUE << "[INFO] Resolved path: " << save_dir << NC << std::endl;

    std::cout << "Is this path correct? [y/N]: ";
    char confirm_char;
    std::cin >> confirm_char;

    // 清除输入缓冲区 (吃掉回车符，防止影响后续逻辑)
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 

    if (confirm_char != 'y' && confirm_char != 'Y') {
        std::cout << RUL_YELLOW << "[INFO] Operation cancelled by user." << NC << std::endl;
        return; // 直接退出函数，不执行备份
    }
    // ==========================================

    std::cout << BLUE << "[INFO] Saving to: " << save_dir << NC << std::endl;

    // 创建目录
    try {
        if (!fs::exists(save_dir)) {
            fs::create_directories(save_dir);
            if (!CURRENT_USER.empty() && CURRENT_USER != "root") {
                std::string chown_cmd = "chown -R " + CURRENT_USER + ":" + CURRENT_USER + " " + save_dir;
                system(chown_cmd.c_str());
            }
        }
    } catch (const std::exception& e) {
        std::cout << RED << "[ERROR] Failed to create directory." << NC << std::endl;
        return;
    }

    // === 批量执行备份 ===
    int success_count = 0;
    for (const auto& env_name : target_envs) {
        std::string filename = save_dir + "/" + env_name + "_backup.yml";
        std::string cmd = FOUND_CONDA_PATH + " env export -n " + env_name + " > \"" + filename + "\"";
        
        std::cout << "Exporting " << RUL_YELLOW << env_name << NC << "... ";
        std::cout.flush(); // 强制刷新缓冲区，让用户看到正在进行

        if (run_cmd(cmd)) {
            std::cout << GREEN << "[DONE]" << NC << std::endl;
            success_count++;
            // 修复权限
            if (!CURRENT_USER.empty() && CURRENT_USER != "root") {
                std::string chown_cmd = "chown " + CURRENT_USER + ":" + CURRENT_USER + " \"" + filename + "\"";
                system(chown_cmd.c_str());
            }
        } else {
            std::cout << RED << "[FAILED]" << NC << std::endl;
        }
    }

    std::cout << "\n------------------------------------------" << std::endl;
    std::cout << "Batch operation completed. " << success_count << "/" << target_envs.size() << " successful." << std::endl;
    std::cout << "Press Enter to continue...";
    std::cin.get();
}

// ---------------------------------------------------------
// Conda 一级菜单
// ---------------------------------------------------------
void conda_setup_menu() {
    // 首次进入自动查找
    find_conda_executable(); 

    while (true) {
        system("clear");
        std::cout << CYAN << "==========================================" << NC << std::endl;
        std::cout << CYAN << "          " << RUL_YELLOW << "Conda Setup Manager" << CYAN << "          " << NC << std::endl;
        std::cout << CYAN << "==========================================" << NC << std::endl;
        if (!FOUND_CONDA_PATH.empty()) {
            std::cout << BLUE << "[System] Conda found." << NC << std::endl;
        } else {
             std::cout << RED << "[System] Conda NOT found (auto-search failed)." << NC << std::endl;
        }
        std::cout << std::endl;

        std::cout << "1. Backup Conda Environments (Multi-Select)" << std::endl;
        std::cout << "0. Return to Main Menu" << std::endl;
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "Choice: ";

        std::string choice_str;
        std::cin >> choice_str;
        
        // 吸收掉cin留下的换行符，防止干扰后续getline或getch
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); 

        int choice = -1;
        try { choice = std::stoi(choice_str); } catch (...) { choice = -1; }

        switch (choice) {
            case 1:
                backup_conda_env();
                break;
            case 0:
                return;
            default:
                std::cout << RED << "Invalid choice" << NC << std::endl;
                system("sleep 1");
                break;
        }
    }
}