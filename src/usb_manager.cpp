#include "sub_menu.h"
#include "unit_color.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <regex>
#include <sstream>
#include <unistd.h>
#include <vector>

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
    std::cout << "Press Enter to continue...";
    std::cin.get();
    return;
  }

  std::istringstream ss(ls_result);
  std::vector<std::string> devices;
  std::string dev; // 从字符串流中按空格读取一个单词
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
    std::string info = exec_cmd("udevadm info --query=property --name=" +
                                d); // 执行命令返回为字符串
    std::regex vid_re("ID_VENDOR_ID=(.*)");
    std::regex pid_re("ID_MODEL_ID=(.*)");
    std::regex serial_re("ID_SERIAL_SHORT=(.*)");
    std::regex model_re("ID_VENDOR=(.*)\\nID_MODEL=(.*)");
    std::smatch match;

    std::string vid = "", pid = "", serial = "", model = "";
    if (std::regex_search(info, match, vid_re))
      vid = match[1];
    if (std::regex_search(info, match, pid_re))
      pid = match[1];
    if (std::regex_search(info, match, serial_re))
      serial = match[1];
    // 简单处理 Model
    std::string vendor = "", model_name = "";
    std::regex vendor_re("ID_VENDOR=(.*)");
    std::regex modelname_re("ID_MODEL=(.*)");
    if (std::regex_search(info, match, vendor_re))
      vendor = match[1];
    if (std::regex_search(info, match, modelname_re))
      model_name = match[1];
    model = vendor + " " + model_name;

    dev_vid[i] = vid;
    dev_pid[i] = pid;
    dev_serial[i] = serial;
    dev_model[i] = model;

    printf("[%d] %-15s %-30s %-15s\n", i, d.c_str(), model.c_str(),
           serial.empty() ? "None" : serial.c_str());
    ++i;
  }

  // 选择设备
  int choice;
  std::cout << "Select device number: ";
  std::cin >> choice;
  if (choice < 1 || choice >= i)
    return;

  std::string suffix;
  while (true) {
    std::cout << "Enter suffix (e.g. px4, lidar): ";
    std::cin >> suffix;
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

  // 写入规则文件
  std::ofstream ofs(RULES_FILE, std::ios::app);
  ofs << "\n# Device mapped | Strategy: "
      << (serial.empty() ? "VID + PID" : "VID + PID + SERIAL") << "\n";
  ofs << rule << "\n";
  ofs.close();

  reload_udev();
  std::cout << GREEN << "Device bound successfully!" << NC << std::endl;
  std::cout << "Press Enter to continue...";
  std::cin.ignore();
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
    std::cout << "Press Enter to continue...";
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
  std::cout << "Select rule number to delete: ";
  std::cin >> idx;
  if (idx < 1 || idx > (int)rules.size())
    return;

  // 删除对应规则
  std::string sed_cmd = "sed -i \"/" + rules[idx - 1] + "/d\" " + RULES_FILE;
  system(sed_cmd.c_str());

  reload_udev();
  std::cout << GREEN << "Binding removed successfully!" << NC << std::endl;
  std::cout << "Press Enter to continue...";
  std::cin.ignore();
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
    std::cin >> choice;
    std::cin.ignore(); // 忽略回车
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
