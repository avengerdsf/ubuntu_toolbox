#include "../../include/external_software.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <pwd.h>
#include <unistd.h>

namespace fs = std::filesystem;

TmuxInstaller::TmuxInstaller(const std::string &pkg_id,
                             const std::string &display_name)
    : SoftwareInstaller(pkg_id, display_name) {}

void TmuxInstaller::install() {
  std::string user_home = get_user_home();

  if (is_installed()) {
    print_info("Tmux already installed.", "\033[1;33m"); // 黄色
    return;
  }

  print_info("Preparing installation: Tmux Terminal Multiplexer (via apt)",
             "\033[0;34m");

  int ret = exec_system("apt update && apt install -y " + pkg_id);
  if (ret == 0) {
    print_info("Installation completed successfully.", "\033[0;32m"); // 绿色
    configure();
  } else {
    print_info("Installation failed.", "\033[0;31m"); // 红色
  }
}

void TmuxInstaller::configure() {
  std::string user_home = get_user_home();
  if (user_home.empty()) {
    print_info("Cannot determine user home, skipping configuration.",
               "\033[0;31m");
    return;
  }

  std::string tmux_conf = user_home + "/.tmux.conf";
  std::string config_line = "set-option -g mouse on";

  print_info("Configuring TMUX for user: " + tmux_conf, "\033[0;34m");

  std::ifstream infile(tmux_conf);
  bool exists = false;
  if (infile.is_open()) {
    std::string line;
    while (std::getline(infile, line)) {
      if (line == config_line) {
        exists = true;
        break;
      }
    }
    infile.close();
  }

  if (!exists) {
    std::ofstream outfile(tmux_conf, std::ios::app);
    if (outfile.is_open()) {
      outfile << config_line << "\n";
      outfile.close();

      // 更改归属
      const char *sudo_user = getenv("SUDO_USER");
      const char *user = sudo_user ? sudo_user : getenv("USER");
      if (user && std::string(user) != "root") {
        exec_system(("chown " + std::string(user) + ":" + std::string(user) +
                     " " + tmux_conf)
                        .c_str());
      }

      print_info("Configuration added to " + tmux_conf, "\033[0;32m");
    } else {
      print_info("Failed to write to " + tmux_conf, "\033[0;31m");
    }
  } else {
    print_info("Configuration already present in " + tmux_conf, "\033[1;33m");
  }
}
