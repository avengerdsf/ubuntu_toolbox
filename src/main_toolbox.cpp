#include <cstdlib> // system()
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <unistd.h> // geteuid()

#include "sub_menu.h"
#include "unit_color.h"

// -------------------- Global Variables --------------------
const std::string TEMP_DIR = "./temp";

// -------------------- Utility Functions --------------------
void check_root_exit() {
  if (geteuid() != 0) {
    std::cout << RED << "\nUsage: sudo ./main_toolbox" << NC << std::endl;
    std::exit(1);
  }
}

// 创建 temp 文件夹（如果不存在）
void create_temp_dir() {
  if (access(TEMP_DIR.c_str(), F_OK) != 0) { // 不存在就创建
    mkdir(TEMP_DIR.c_str(), 0755);
  }
}

// 清理 temp 文件夹
void cleanup_temp() {
  std::string cmd = "rm -rf " + TEMP_DIR;
  system(cmd.c_str());
}

// -------------------- Main Menu --------------------
int main() {
  check_root_exit();
  create_temp_dir();
  atexit(cleanup_temp); // register cleanup

  while (true) {
    system("clear");
    std::cout << CYAN << "==========================================" << NC
              << std::endl;
    std::cout << CYAN << "          " << RUL_YELLOW << "System Tools Toolbox"
              << CYAN << "          " << NC << std::endl;
    std::cout << CYAN << "==========================================" << NC
              << std::endl;
    std::cout << std::endl;

    std::cout << "1. USB Serial Port Manager" << std::endl;
    std::cout << "2. Shell Script Template Generator" << std::endl;
    std::cout << "3. External Library / Driver Manager" << std::endl;
    std::cout << "4. External SoftWare Manager" << std::endl;
    std::cout << "0. Exit" << std::endl;
    std::cout << "------------------------------------------" << std::endl;

    std::cout << "Choice: ";
    std::string choice_str;
    std::getline(std::cin, choice_str);

    int choice = -1;
    try {
      choice = std::stoi(choice_str); // 转换为整数
    } catch (...) {
      choice = -1; // 非数字输入
    }

    switch (choice) {
    case 1:
      usb_serial_menu();
      break;
    case 2:
      template_generator_menu();
      break;
    case 3:
      std::cout << "External Library / Driver Manager\n";
      break;
    case 4:
      external_software();
      break;
    case 0:
      return 0; // 退出程序
    default:
      std::cout << "Invalid choice\n";
      system("sleep 1");
      break;
    }
  }

  return 0;
}
