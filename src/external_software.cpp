#include "sub_menu.h"



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