#include "../../include/external_software.h"
#include "../../include/unit_color.h"
#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <regex>
#include <string>
#include <unistd.h>
#include <vector>

#include <pwd.h>
#include <sys/types.h>

namespace fs = std::filesystem;

SogouInstaller::SogouInstaller(const std::string &pkg_id,
                               const std::string &dispaly_name,
                               const std::string &website_url)
    : SoftwareInstaller(pkg_id, dispaly_name), website_url(website_url) {}

std::string SogouInstaller::get_default_download_dir() {
  std::string home = get_user_home();
  std::string cn_dir = home + "/下载";
  if (fs::exists(cn_dir))
    return cn_dir;
  std::string en_dir = home + "/Downloads";
  if (fs::exists(en_dir))
    return en_dir;
  return home; // fallback
}

std::string SogouInstaller::find_deb_file(const std::string &user_home,
                                          const std::string &arch) {
  // 搜索目录
  std::vector<std::string> search_dirs = {get_default_download_dir(), ".",
                                          user_home};

  // 构造正则，根据架构匹配文件
  std::regex deb_pattern("sogoupinyin_.*_" + arch + R"(\.deb)");

  for (auto &dir : search_dirs) {
    if (!fs::exists(dir))
      continue;
    for (auto &p : fs::directory_iterator(dir)) {
      if (p.is_regular_file()) {
        std::string filename = p.path().filename().string();
        if (std::regex_match(filename, deb_pattern)) {
          return p.path().string();
        }
      }
    }
  }

  return "";
}

void SogouInstaller::install() {
  if (is_installed()) {
    print_info(display_name + " already installed.", RUL_YELLOW);
    return;
  }

  print_info("Starting installation for: " + display_name, BLUE);

  std::string arch = detect_arch();
  if (arch != "amd64" && arch != "arm64") {
    print_info("Unsupported architecture: " + arch, RED);
    return;
  }

  std::string user_home = get_user_home();

  std::cout << RUL_YELLOW
            << "!!! Please manually download the deb file before proceeding !!!"
            << NC << std::endl;
  std::cout << "Official website: " << website_url << std::endl;
  std::cout << "System architecture: " << arch << std::endl;
  std::cout << "Recommended download path: " << get_default_download_dir()
            << std::endl;
  std::string deb_file;
  std::cout << "Enter full path to .deb file (or press Enter to search for "
               "/home/user/Download/sougou_*_"
            << arch << ".deb): ";
  std::getline(std::cin, deb_file);

  if (deb_file.empty()) {
    deb_file = find_deb_file(user_home, arch);
  }

  if (deb_file.empty() || !fs::exists(deb_file)) {
    print_info("Error: Matching .deb file not found.", RED);
    std::cout << "Please ensure the downloaded file is accessible. Filename "
                 "should match: sougou_*_"
              << arch << ".deb" << std::endl;
    return;
  }

  print_info("Found valid .deb file: " + deb_file, GREEN);

  std::string temp_dir = create_temp_dir();
  std::string cmd_copy = "cp \"" + deb_file + "\" \"" + temp_dir + "\"";
  exec_system(cmd_copy);

  std::string install_cmd =
      "apt update && apt install -y libqt5qml5 libqt5quick5 "
      "libqt5quickwidgets5 qml-module-qtquick2 libgsettings-qt1 fcitx && dpkg "
      "-i \"" +
      temp_dir + "/" + fs::path(deb_file).filename().string() +
      "\" || apt install -f -y";
  exec_system(install_cmd);
}

void SogouInstaller::configure() {
  print_info("Setting up fcitx autostart...", BLUE);
  if (fs::exists("/usr/share/applications/fcitx.desktop")) {
    exec_system("cp /usr/share/applications/fcitx.desktop /etc/xdg/autostart/ "
                "2>/dev/null || true");
  }

  print_info("Configuring input method system to fcitx...", BLUE);
  std::string env_file = "/etc/environment";
  std::string check_cmd = "grep -q XMODIFIERS " + env_file;
  if (exec_system(check_cmd) != 0) {
    exec_system(
        "echo 'GTK_IM_MODULE=fcitx' | tee -a /etc/environment > /dev/null");
    exec_system(
        "echo 'QT_IM_MODULE=fcitx' | tee -a /etc/environment > /dev/null");
    exec_system(
        "echo 'XMODIFIERS=@im=fcitx' | tee -a /etc/environment > /dev/null");
    print_info("Input method environment variables configured.", GREEN);
  } else {
    print_info("Input method environment variables already exist, skipping "
               "configuration.",
               RUL_YELLOW);
  }
}
