#include "sub_menu.h"

namespace fs = std::filesystem;

// 定义全局列表
std::vector<SoftwareItem> SOFTWARE_LIST;

// ---------------------------------------------------------
// SoftwareInstaller 实现
// ---------------------------------------------------------

SoftwareInstaller::SoftwareInstaller(SoftwareConfig config) : config(config) {}

void SoftwareInstaller::install() {
  print_info("Checking status for " + config.display_name + "...", CYAN);

  if (is_installed()) {
    print_info(config.display_name + " is already installed.", GREEN);
    return;
  }

  // 1. Pre-install (钩子)
  // 2. Install Dependencies
  install_dependencies();

  // 3. Core Install
  if (perform_core_installation()) {
    // 4. Post-install (钩子)
    post_install();
    print_info("Successfully installed: " + config.display_name, GREEN);
  } else {
    print_info("Installation failed: " + config.display_name, RED);
  }

  std::cout << "Press Enter to continue...";
  std::cin.ignore();
  std::cin.get();
}

void SoftwareInstaller::uninstall() {
  print_info("Uninstalling " + config.display_name + "...", RUL_YELLOW);

  std::string cmd = "apt remove -y " + config.pkg_name;
  int ret = exec_sys(cmd);

  if (ret == 0) {
    post_uninstall();
    print_info("Uninstalled successfully.", GREEN);
  } else {
    print_info("Uninstall failed.", RED);
  }
  system("sleep 1");
}

// --- 工具函数 ---

void SoftwareInstaller::print_info(const std::string &msg,
                                   const std::string &color) {
  std::cout << color << "[INFO] " << msg << NC << std::endl;
}

int SoftwareInstaller::exec_sys(const std::string &cmd) {
  // 简单封装 system，实际项目中可以使用 popen 获取输出
  return system(cmd.c_str());
}

std::string SoftwareInstaller::detect_arch() {
  // 使用 dpkg 探测架构
  std::array<char, 128> buffer;
  std::string result;
  FILE *pipe = popen("dpkg --print-architecture", "r");
  if (!pipe)
    return "amd64"; // fallback
  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
    result += buffer.data();
  }
  pclose(pipe);
  // 去除换行符
  if (!result.empty() && result.back() == '\n') {
    result.pop_back();
  }
  return result.empty() ? "amd64" : result;
}

std::string SoftwareInstaller::get_user_home() {
  // 因为脚本以 sudo 运行，需要获取真实用户的 home
  const char *sudo_user = std::getenv("SUDO_USER");
  if (sudo_user) {
    return "/home/" + std::string(sudo_user);
  }
  return "/root"; // fallback
}

std::string SoftwareInstaller::create_temp_dir() {
  // 直接返回 main_toolbox.cpp 中已定义的全局 TEMP_DIR
  // 确保目录存在 (虽然 main 已经创建，但防御性编程)
  if (!fs::exists(TEMP_DIR)) {
    fs::create_directory(TEMP_DIR);
  }
  return TEMP_DIR;
}

bool SoftwareInstaller::is_installed() {
  // 使用 dpkg -s 检查包状态
  std::string cmd = "dpkg -s " + config.pkg_name + " > /dev/null 2>&1";
  return (system(cmd.c_str()) == 0);
}

void SoftwareInstaller::install_dependencies() {
  if (config.dependencies.empty())
    return;

  print_info("Installing dependencies...", RUL_YELLOW);
  std::string deps = "";
  for (const auto &dep : config.dependencies) {
    deps += dep + " ";
  }
  exec_sys("apt update && apt install -y " + deps);
}

bool SoftwareInstaller::perform_core_installation() {
  std::string temp_path = create_temp_dir();

  switch (config.type) {
  case InstallType::APT: {
    std::string cmd = "apt install -y " + config.pkg_name;
    return (exec_sys(cmd) == 0);
  }
  case InstallType::DEB_URL: {
    std::string arch = detect_arch();
    if (config.arch_urls.find(arch) == config.arch_urls.end()) {
      print_info("Unsupported architecture: " + arch, RED);
      return false;
    }
    std::string url = config.arch_urls[arch];
    std::string deb_file = temp_path + "/" + config.id + ".deb";

    print_info("Downloading from " + url, BLUE);
    std::string wget_cmd = "wget -O " + deb_file + " \"" + url + "\"";
    if (exec_sys(wget_cmd) != 0)
      return false;

    print_info("Installing DEB package...", BLUE);
    // 使用 apt install ./file.deb 可以自动解决部分依赖
    return (exec_sys("apt install -y " + deb_file) == 0);
  }
  case InstallType::DEB_MANUAL: {
    std::string arch = detect_arch();
    std::string deb_path = find_local_deb(arch);
    if (deb_path.empty()) {
      print_info("Please download the .deb package for " + arch, RED);
      print_info("Website: " + config.website, CYAN);
      print_info("Place it in: " + temp_path, RUL_YELLOW);
      std::cout << "Press Enter after you have copied the file...";
      std::cin.ignore();
      std::cin.get();

      // Try again
      deb_path = find_local_deb(arch);
      if (deb_path.empty()) {
        print_info("File not found. Aborting.", RED);
        return false;
      }
    }
    return (exec_sys("apt install -y " + deb_path) == 0);
  }
  default:
    return false;
  }
}

std::string SoftwareInstaller::find_local_deb(const std::string &arch) {
  // 防御性检查：目录不存在直接返回
  if (!fs::exists(TEMP_DIR))
    return "";

  for (const auto &entry : fs::directory_iterator(TEMP_DIR)) {
    // 必须是普通文件，忽略子目录
    if (!entry.is_regular_file())
      continue;

    if (entry.path().extension() == ".deb") {
      std::string filename = entry.path().filename().string();

      // 1. 核心匹配：文件名必须包含 config.id
      // 例如 config.id="sogou"，匹配 "sogoupinyin_4.2.1_amd64.deb"
      if (filename.find(config.id) == std::string::npos) {
        continue;
      }

      // 2. 架构匹配：文件名必须包含当前架构 (如 amd64) 或者 通用架构
      // (all)
      // 这一步至关重要，防止在 amd64 机器上强行安装 arm64 包
      if (filename.find(arch) != std::string::npos ||
          filename.find("_all") != std::string::npos || // 常见命名规则
          filename.find("-all") != std::string::npos) { // 备用命名规则

        return entry.path().string();
      }
    }
  }
  return "";
}

void TmuxInstaller::post_install() {
  print_info("Configuring Tmux...", BLUE);
  std::string home = get_user_home(); // 使用基类提供的 helper
  if (home == "/root" && std::getenv("SUDO_USER")) {
    // 双重检查，确保获取的是普通用户的 home
    home = "/home/" + std::string(std::getenv("SUDO_USER"));
  }

  std::string tmux_conf = home + "/.tmux.conf";
  std::string config_line = "set-option -g mouse on";

  // 检查配置是否存在
  std::ifstream infile(tmux_conf);
  if (infile.is_open()) {
    std::string line;
    while (std::getline(infile, line)) {
      if (line == config_line)
        return; // 已存在，无需写入
    }
  }

  // 写入配置
  std::ofstream outfile(tmux_conf, std::ios::app);
  if (outfile.is_open()) {
    outfile << "\n" << config_line << "\n";
    outfile.close();

    // 修复权限 (因为当前是以 sudo 运行的)
    const char *sudo_user = std::getenv("SUDO_USER");
    if (sudo_user) {
      std::string chown_cmd = "chown " + std::string(sudo_user) + ":" +
                              std::string(sudo_user) + " " + tmux_conf;
      system(chown_cmd.c_str());
    }
    print_info("Added mouse support to " + tmux_conf, GREEN);
  } else {
    print_info("Failed to write to .tmux.conf", RED);
  }
}

void SogouInstaller::post_install() {
  print_info("Configuring Fcitx environment variables...", BLUE);

  // 1. 设置 fcitx 自启动
  if (fs::exists("/usr/share/applications/fcitx.desktop")) {
    system(
        "cp /usr/share/applications/fcitx.desktop /etc/xdg/autostart/ "
        "2>/dev/null || true");
  }

  // 2. 配置 /etc/environment
  // 简单检查是否已配置
  int check = system("grep -q 'XMODIFIERS' /etc/environment");

  if (check != 0) {
    system("echo 'GTK_IM_MODULE=fcitx' | tee -a /etc/environment > /dev/null");
    system("echo 'QT_IM_MODULE=fcitx' | tee -a /etc/environment > /dev/null");
    system(
        "echo 'XMODIFIERS=@im=fcitx' | tee -a /etc/environment > /dev/null");
    print_info("Environment variables set. Please relogin.", GREEN);
  } else {
    print_info("Environment variables already look configured.", RUL_YELLOW);
  }
}


// ---------------------------------------------------------
// 0. 初始化软件列表 (数据配置)
// ---------------------------------------------------------
void init_software_list() {
  SOFTWARE_LIST.clear();

  // -------------------------------------------------
  // 1. Clash Verge (标准 DEB_URL 模式)
  // -------------------------------------------------
  SoftwareConfig clash_conf;
  clash_conf.id = "clash-verge";
  clash_conf.display_name = "Clash Verge (Proxy Client)";
  clash_conf.type = InstallType::DEB_URL;
  clash_conf.pkg_name = "clash-verge"; 
  // 注意：Github Release 链接可能会变，建议定期检查
  clash_conf.arch_urls["amd64"] = "https://down.clashchinese.com/soft/CVR/clashchinese.com_Clash.Verge_2.1.2_amd64.deb";
  clash_conf.arch_urls["arm64"] = "https://down.clashchinese.com/soft/CVR/clashchinese.com_Clash.Verge_2.1.2_arm64.deb";
  // 不需要自定义类，直接用基类
  SOFTWARE_LIST.push_back({"clash", "Network", std::make_unique<SoftwareInstaller>(clash_conf)});

  // -------------------------------------------------
  // 2. Tmux (APT 模式 + 自定义配置)
  // -------------------------------------------------
  SoftwareConfig tmux_conf;
  tmux_conf.id = "tmux";
  tmux_conf.display_name = "Tmux Terminal Multiplexer";
  tmux_conf.type = InstallType::APT;
  tmux_conf.pkg_name = "tmux";
  // 使用自定义类 TmuxInstaller
  SOFTWARE_LIST.push_back({"tmux", "Tool", std::make_unique<TmuxInstaller>(tmux_conf)});

  // -------------------------------------------------
  // 3. Sogou Pinyin (DEB_MANUAL 模式 + 自定义配置)
  // -------------------------------------------------
  SoftwareConfig sogou_conf;
  sogou_conf.id = "sogou"; // 必须包含在下载的文件名中，例如 sogoupinyin_xxx.deb
  sogou_conf.display_name = "Sogou Pinyin Input Method";
  sogou_conf.type = InstallType::DEB_MANUAL;
  sogou_conf.pkg_name = "sogoupinyin"; // 安装后的包名
  sogou_conf.website = "https://shurufa.sogou.com/linux";
  
  // 关键：将原来的 apt install -f 依赖项显式列出，让基类自动安装
  sogou_conf.dependencies = {
      "fcitx", 
      "libqt5qml5", 
      "libqt5quick5", 
      "libqt5quickwidgets5", 
      "qml-module-qtquick2", 
      "libgsettings-qt1"
  };
  
  // 使用自定义类 SogouInstaller
  SOFTWARE_LIST.push_back({"sogou", "System", std::make_unique<SogouInstaller>(sogou_conf)});
}

// ---------------------------------------------------------
// 1. 界面辅助函数
// ---------------------------------------------------------
void external_manager_title() {
  std::cout << CYAN << "==========================================" << NC << std::endl;
  std::cout << CYAN << "     " << RUL_YELLOW << "External Software Manager" << CYAN << "     " << NC << std::endl;
  std::cout << CYAN << "==========================================" << NC << std::endl;
}

void software_action_title(const std::string &title) {
  std::cout << CYAN << "==========================================" << NC << std::endl;
  std::cout << CYAN << "          " << RUL_YELLOW << title << CYAN << "          " << NC << std::endl;
  std::cout << CYAN << "==========================================" << NC << std::endl;
}

// ---------------------------------------------------------
// 2. 动作执行函数
// ---------------------------------------------------------
void run_install_action(SoftwareInstaller *installer) {
  if (!installer) return;
  // 不需要传入 type 字符串，installer 内部根据 config 自动判断
  installer->install(); 
}

void run_uninstall_action(SoftwareInstaller *installer) {
  if (!installer) return;
  installer->uninstall();
}

// ---------------------------------------------------------
// 3. 二级菜单 (单个软件操作)
// ---------------------------------------------------------
void software_action_menu(SoftwareItem &item) {
  while (true) {
    system("clear");
    // 获取名字使用 item.installer->get_name()
    software_action_title(item.installer->get_name() + " Manager");

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "1. Install / Update " << item.installer->get_name() << std::endl;
    std::cout << "2. Uninstall " << item.installer->get_name() << std::endl;
    // 注意：Configure 功能在 Header 中未定义，故移除
    
    std::cout << "0. Return" << std::endl;
    std::cout << "------------------------------------------";
    std::cout << "\nChoice: ";

    std::string choice;
    std::cin >> choice;
    // 清除输入缓冲区的剩余字符
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (choice == "1") {
      run_install_action(item.installer.get());
    } else if (choice == "2") {
      run_uninstall_action(item.installer.get());
    } else if (choice == "0") {
      return;
    } else {
      std::cout << RED << "Invalid choice." << NC << std::endl;
      system("sleep 1");
    }
  }
}

// ---------------------------------------------------------
// 4. 一级菜单 (软件列表)
// ---------------------------------------------------------
void external_software() {
  // 确保列表已初始化
  if (SOFTWARE_LIST.empty()) {
      init_software_list();
  }

  while (true) {
    system("clear");
    external_manager_title();
    std::cout << std::endl;

    std::map<int, int> menu_map; // 映射 菜单序号 -> 列表索引
    int index = 1;

    for (size_t i = 0; i < SOFTWARE_LIST.size(); ++i) {
      bool is_inst = SOFTWARE_LIST[i].installer->is_installed(); 
      std::string status = is_inst ? (GREEN + "[Installed]" + NC) : (RUL_YELLOW + "[Not Installed]" + NC);

      std::cout << index << ". " << SOFTWARE_LIST[i].installer->get_name() 
                << " " << status << std::endl;
      
      menu_map[index] = i;
      ++index;
    }

    std::cout << "\n0. Return to Main Toolbox" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "Choice: ";

    std::string user_choice;
    std::cin >> user_choice;
    std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

    if (user_choice == "0") return;

    try {
        int choice = std::stoi(user_choice);
        if (menu_map.count(choice)) {
          software_action_menu(SOFTWARE_LIST.at(menu_map[choice]));
        } else {
          std::cout << RED << "Invalid choice." << NC << std::endl;
          system("sleep 1");
        }
    } catch (...) {
        std::cout << RED << "Invalid input." << NC << std::endl;
        system("sleep 1");
    }
  }
}
