#include <cstdlib> // system()
#include <cerrno>
#include <limits>
#include <fcntl.h>
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

// 创建 temp 文件夹，并将所有权交给 sudo 的原始用户。
void create_temp_dir() {
  auto fail = []() {
    std::cerr << RED << "Failed to prepare " << TEMP_DIR << NC << std::endl;
    std::exit(EXIT_FAILURE);
  };

  if (mkdir(TEMP_DIR.c_str(), 0755) != 0 && errno != EEXIST) {
    fail();
  }

  // 不跟随符号链接，确保修改的是实际目录。
  int fd = open(TEMP_DIR.c_str(), O_RDONLY | O_DIRECTORY | O_NOFOLLOW);
  if (fd < 0) {
    fail();
  }

  const char* uid_text = std::getenv("SUDO_UID");
  const char* gid_text = std::getenv("SUDO_GID");
  if (uid_text && gid_text) {
    auto parse_id = [](const char* value, unsigned long max_id,
                       unsigned long& result) {
      if (!*value) return false;
      for (const char* p = value; *p; ++p) {
        if (*p < '0' || *p > '9') return false;
      }
      errno = 0;
      char* end = nullptr;
      result = std::strtoul(value, &end, 10);
      return errno == 0 && *end == '\0' && result < max_id;
    };

    unsigned long uid, gid;
    if (!parse_id(uid_text, std::numeric_limits<uid_t>::max(), uid) ||
        !parse_id(gid_text, std::numeric_limits<gid_t>::max(), gid) ||
        fchown(fd, static_cast<uid_t>(uid), static_cast<gid_t>(gid)) != 0 ||
        fchmod(fd, 0755) != 0) {
      close(fd);
      fail();
    }
  } else {
    close(fd);
    std::cerr << RED << "Start with sudo from your regular user account."
              << NC << std::endl;
    std::exit(EXIT_FAILURE);
  }

  close(fd);
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
    std::cout << "5. External SoftWare Manager" << std::endl;
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
    case 5:
      conda_setup_menu();
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
