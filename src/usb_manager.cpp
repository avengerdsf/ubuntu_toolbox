#include "sub_menu.h"


const std::string RULES_FILE = "/etc/udev/rules.d/99-usb-serial.rules";

// -------------------- 工具函数 --------------------
void clear_screen() { system("clear"); }
void sleep_sec(int sec) { system(("sleep " + std::to_string(sec)).c_str()); }

// 运行命令并返回输出
std::string exec_cmd(const std::string &cmd) {
  std::array<char, 128> buffer;
  std::string result;
  FILE *pipe = popen(cmd.c_str(), "r");
  if (!pipe)
    return "";
  while (fgets(buffer.data(), buffer.size(), pipe) != nullptr) {
    result += buffer.data();
  }
  pclose(pipe);
  return result;
}

// -------------------- 菜单标题 --------------------
void usb_serial_title() {
  clear_screen();
  std::cout << CYAN << "==========================================" << NC
            << std::endl;
  std::cout << CYAN << "        " << RUL_YELLOW << "USB Serial Port Manager"
            << CYAN << "        " << NC << std::endl;
  std::cout << CYAN << "==========================================" << NC
            << std::endl;
}

// -------------------- udev 规则重载 --------------------
void reload_udev() {
  std::cout << BLUE << "\n--- Applying Udev Rules ---" << NC << std::endl;
  system("udevadm control --reload-rules");
  system("udevadm trigger");
  sleep_sec(1);
  std::cout << GREEN << "Rules reloaded successfully." << NC << std::endl;
}

// -------------------- 绑定设备 --------------------
void bind_device() {
  std::cout << BLUE << "\n=== Scanning for USB Serial Devices ===" << NC
            << std::endl;

  // 扫描 /dev/ttyUSB* 和 /dev/ttyACM*
  std::string ls_result = exec_cmd("ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null");
  if (ls_result.empty()) {
    std::cout << RED << "No USB serial devices found." << NC << std::endl;
    std::cout << "Press Enter to return...";
    std::cin.get();
    return;
  }

  std::istringstream ss(ls_result);
  std::vector<std::string> devices;
  std::string dev;
  while (ss >> dev)
    devices.push_back(dev);

  // 读取设备信息
  std::map<int, std::string> dev_model, dev_serial, dev_vid, dev_pid;

  std::cout << "---------------------------------------------------------------"
               "-----------------\n";
  std::cout << CYAN
            << "No.   Port           Model                         Serial" << NC
            << std::endl;
  std::cout << "---------------------------------------------------------------"
               "-----------------\n";

  int i = 1;
  for (const auto &d : devices) {
    std::string info = exec_cmd("udevadm info --query=property --name=" + d);
    std::regex vid_re("ID_VENDOR_ID=(.*)");
    std::regex pid_re("ID_MODEL_ID=(.*)");
    std::regex serial_re("ID_SERIAL_SHORT=(.*)");
    std::regex model_re("ID_VENDOR=(.*)\\nID_MODEL=(.*)");
    std::smatch match;

    std::string vid = "", pid = "", serial = "", model = "";
    if (std::regex_search(info, match, vid_re)) vid = match[1];
    if (std::regex_search(info, match, pid_re)) pid = match[1];
    if (std::regex_search(info, match, serial_re)) serial = match[1];
    
    // 简单处理 Model
    std::string vendor = "", model_name = "";
    std::regex vendor_re("ID_VENDOR=(.*)");
    std::regex modelname_re("ID_MODEL=(.*)");
    if (std::regex_search(info, match, vendor_re)) vendor = match[1];
    if (std::regex_search(info, match, modelname_re)) model_name = match[1];
    model = vendor + " " + model_name;

    dev_vid[i] = vid;
    dev_pid[i] = pid;
    dev_serial[i] = serial;
    dev_model[i] = model;

    printf("[%d] %-15s %-30s %-15s\n", i, d.c_str(), model.c_str(),
           serial.empty() ? "None" : serial.c_str());
    ++i;
  }
  // [新增] 打印返回选项
  printf("[0] %-15s\n", "Cancel / Return");

  // 选择设备
  int choice;
  while (true) {
    std::cout << "Select device number (0 to cancel): ";
    if (!(std::cin >> choice)) {
      std::cin.clear();
      std::cin.ignore(1024, '\n');
      std::cout << RED << "Invalid input." << NC << std::endl;
      continue;
    }
    // [新增] 处理返回逻辑
    if (choice == 0) {
      std::cin.ignore(1024, '\n'); // 清理缓冲区
      return; 
    }
    
    if (choice < 1 || choice >= i) {
      std::cout << RED << "Invalid device number." << NC << std::endl;
      continue;
    }
    std::cin.ignore(1024, '\n');
    break;
  }

  std::string suffix;
  while (true) {
    std::cout << "Enter suffix (e.g. px4, lidar) or '0' to cancel: ";
    std::cin >> suffix;
    
    // [新增] 处理返回逻辑
    if (suffix == "0") return;

    if (std::regex_match(suffix, std::regex("[a-zA-Z0-9_]+")))
      break;
    std::cout << RED << "Invalid suffix." << NC << std::endl;
  }

  std::string symlink = "tty_" + suffix;
  std::string vid = dev_vid[choice], pid = dev_pid[choice],
              serial = dev_serial[choice];
  std::string rule_cond = serial.empty()
                              ? "ATTRS{idVendor}==\"" + vid +
                                    "\", ATTRS{idProduct}==\"" + pid + "\""
                              : "ATTRS{idVendor}==\"" + vid +
                                    "\", ATTRS{idProduct}==\"" + pid +
                                    "\", ATTRS{serial}==\"" + serial + "\"";
  std::string rule =
      "SUBSYSTEM==\"tty\", " + rule_cond + ", SYMLINK+=\"" + symlink + "\"";

  // -------------------- 重复绑定检测 --------------------
  std::ifstream ifs(RULES_FILE);
  std::string line;
  bool same_device = false;
  bool same_symlink = false;

  while (std::getline(ifs, line)) {
    if (!serial.empty()) {
      if (line.find("ATTRS{idVendor}==\"" + vid + "\"") != std::string::npos &&
          line.find("ATTRS{idProduct}==\"" + pid + "\"") != std::string::npos &&
          line.find("ATTRS{serial}==\"" + serial + "\"") != std::string::npos) {
        same_device = true;
      }
    }
    if (line.find("SYMLINK+=\"" + symlink + "\"") != std::string::npos) {
      same_symlink = true;
    }
  }
  ifs.close();

  if (same_device || same_symlink) {
    std::cout << RUL_YELLOW << "Existing binding detected:" << NC << std::endl;

    if (same_device)
      std::cout << " - This device (with same serial) is already bound."
                << std::endl;
    if (same_symlink)
      std::cout << " - Symlink name already exists." << std::endl;

    std::cout << "Overwrite existing binding? (y/n): ";
    char ans;
    std::cin >> ans;
    std::cin.ignore(1024, '\n');

    if (ans != 'y' && ans != 'Y') {
      std::cout << "Operation cancelled." << std::endl;
      std::cout << "Press Enter to return...";
      std::cin.get();
      return;
    }

    std::string sed_cmd;
    if (same_device) {
      sed_cmd += "sed -i '/ATTRS{idVendor}==\"" + vid +
                 "\".*ATTRS{idProduct}==\"" + pid +
                 "\".*ATTRS{serial}==\"" + serial + "\"/d' " + RULES_FILE;
      system(sed_cmd.c_str());
    }

    if (same_symlink) {
      sed_cmd = "sed -i '/SYMLINK+=\"" + symlink + "\"/d' " + RULES_FILE;
      system(sed_cmd.c_str());
    }
  }

  // 写入规则文件
  std::ofstream ofs(RULES_FILE, std::ios::app);
  ofs << "\n# Device mapped | Strategy: "
      << (serial.empty() ? "VID + PID" : "VID + PID + SERIAL") << "\n";
  ofs << rule << "\n";
  ofs.close();

  reload_udev();
  std::cout << GREEN << "Device bound successfully!" << NC << std::endl;
  std::cout << "Press Enter to continue...";
  std::cin.ignore(); // 此时缓冲区应该是空的，但为了保险
  std::cin.get();
}

// -------------------- 解绑设备 --------------------
void unbind_device() {
  std::cout << BLUE << "\n=== Current USB Serial Bindings ===" << NC
            << std::endl;

  std::string grep_cmd = "grep 'SYMLINK+=' " + RULES_FILE;
  std::string rules_output = exec_cmd(grep_cmd);
  if (rules_output.empty()) {
    std::cout << RUL_YELLOW << "No bindings found." << NC << std::endl;
    std::cout << "Press Enter to return...";
    std::cin.get();
    return;
  }

  std::istringstream ss(rules_output);
  std::vector<std::string> rules;
  std::string line;
  while (std::getline(ss, line))
    rules.push_back(line);

  for (size_t i = 0; i < rules.size(); ++i) {
    std::cout << "[" << (i + 1) << "] " << rules[i] << std::endl;
  }

  int idx;
  // [修改] 提示可以输入 0 退出
  std::cout << "Select rule number to delete (0 to cancel): ";
  if (!(std::cin >> idx)) {
      std::cin.clear();
      std::cin.ignore(1024, '\n');
      return;
  }
  
  // [新增] 处理返回逻辑
  if (idx == 0) {
      return;
  }

  if (idx < 1 || idx > (int)rules.size()) {
      std::cout << RED << "Invalid selection." << NC << std::endl;
      sleep_sec(1);
      return;
  }

  // 从规则中提取 SYMLINK 名称，例如 tty_px4
  std::regex link_re("SYMLINK\\+=\"([^\"]+)\"");
  std::smatch match;
  std::string symlink;

  if (std::regex_search(rules[idx - 1], match, link_re)) {
    symlink = match[1];
  } else {
    std::cout << RED << "Failed to parse SYMLINK." << NC << std::endl;
    std::cin.ignore(1024, '\n');
    std::cin.get();
    return;
  }

  // 仅按 SYMLINK 删除
  std::string sed_cmd =
      "sed -i '/SYMLINK+=\"" + symlink + "\"/d' " + RULES_FILE;
  system(sed_cmd.c_str());

  reload_udev();
  std::cout << GREEN << "Binding removed successfully!" << NC << std::endl;
  std::cout << "Press Enter to continue...";
  std::cin.ignore(1024, '\n'); // 清理掉 cin >> idx 留下的换行符
  std::cin.get();
}

// -------------------- USB 主菜单 --------------------
void usb_serial_menu() {
  while (true) {
    usb_serial_title();
    std::cout << "Current rule file: " << CYAN << RULES_FILE << NC << std::endl;
    std::cout << "\n1. Bind USB Serial Device\n2. Unbind USB Serial Device\n0. "
                 "Return to Main Toolbox\n";
    std::cout << "Choice: ";
    std::string choice;
    if (!(std::cin >> choice)) {
      std::cin.clear();
      std::cin.ignore(1024, '\n');
      continue;
    }
    std::cin.ignore(1024, '\n');

    if (choice == "0")
      break;
    else if (choice == "1")
      bind_device();
    else if (choice == "2")
      unbind_device();
    else
      std::cout << RED << "Invalid choice." << NC << std::endl;
  }
}