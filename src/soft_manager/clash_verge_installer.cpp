#include "../../include/external_software.h"
#include "unit_color.h" // 彩色输出
#include <cstdlib>
#include <iostream>
#include <string>
#include <unistd.h>

ClashInstaller::ClashInstaller(const std::string &pkg_id,
                               const std::string &display_name,
                               const std::string &url_amd64,
                               const std::string &url_arm64)
    : SoftwareInstaller(pkg_id, display_name), url_amd64(url_amd64),
      url_arm64(url_arm64) {}

// 根据架构返回下载链接
std::string ClashInstaller::get_target_url() {
  std::string arch = detect_arch();
  if (arch == "amd64")
    return url_amd64;
  else if (arch == "arm64")
    return url_arm64;
  return "";
}

void ClashInstaller::install() {
  if (is_installed()) {
    print_info(pkg_id + " already installed.", RUL_YELLOW);
    return;
  }

  std::string url = get_target_url();
  if (url.empty()) {
    print_info("[ERROR] No download link for this architecture.", RED);
    return;
  }

  print_info("Preparing installation: " + pkg_id + " (via deb package)", BLUE);
  print_info("Architecture: " + detect_arch());

  std::string file_name = url.substr(url.find_last_of('/') + 1);
  std::string temp_dir = create_temp_dir();
  std::string pkg_path = temp_dir + "/" + file_name;

  std::string download_cmd = "curl -L \"" + url + "\" -o \"" + pkg_path + "\"";
  if (exec_system(download_cmd) != 0) {
    print_info("Download failed.", RED);
    return;
  }

  std::string install_cmd = "apt install -y \"" + pkg_path + "\"";
  if (exec_system(install_cmd) == 0) {
    print_info("Installation completed successfully.", GREEN);
  } else {
    print_info("Installation failed.", RED);
  }
}

// void ClashInstaller::uninstall() {
//   if (!is_installed()) {
//     print_info(pkg_id + " not installed.", RUL_YELLOW);
//     return;
//   }

//   print_info("Uninstalling: " + pkg_id, BLUE);
//   std::string uninstall_cmd = "apt purge -y " + pkg_id;
//   if (exec_system(uninstall_cmd) == 0) {
//     print_info("Successfully removed.", GREEN);
//   } else {
//     print_info("Uninstall failed.", RED);
//   }
// }
