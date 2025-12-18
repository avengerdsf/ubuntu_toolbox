#include "external_software.h"
#include "sub_menu.h"
#include "unit_color.h"
#include <iostream>
#include <map>
#include <memory>
#include <vector>

// ---------------- Menu Rendering ----------------
void external_manager_title() {
  std::cout << CYAN << "==========================================" << NC
            << std::endl;
  std::cout << CYAN << "     " << RUL_YELLOW << "External Software Manager"
            << CYAN << "     " << NC << std::endl;
  std::cout << CYAN << "==========================================" << NC
            << std::endl;
}

void software_action_title(const std::string &title) {
  std::cout << CYAN << "==========================================" << NC
            << std::endl;
  std::cout << CYAN << "          " << RUL_YELLOW << title << CYAN
            << "          " << NC << std::endl;
  std::cout << CYAN << "==========================================" << NC
            << std::endl;
}

// ---------------- Action Drivers ----------------
void run_install_action(SoftwareInstaller *installer, const std::string &type) {
  if (!installer)
    return;

  if (type == "deb")
    installer->install();
  else if (type == "apt")
    installer->install();
  else
    std::cout << RED << "[ERROR] Unknown software type: " << type << NC
              << std::endl;

  std::cout << "Press Enter...";
  std::cin.ignore();
}

void run_uninstall_action(SoftwareInstaller *installer,
                          const std::string &type) {
  if (!installer)
    return;

  if (type == "deb" || "apt")
    installer->uninstall();
  else
    std::cout << RED << "[ERROR] Unknown software type: " << type << NC
              << std::endl;

  std::cout << "Press Enter...";
  std::cin.ignore();
}

// ---------------- Level 2 Menu ----------------
void software_action_menu(SoftwareItem &item) {
  while (true) {
    system("clear");
    software_action_title(item.name + " Manager");

    std::cout << "------------------------------------------" << std::endl;
    std::cout << "1. Install " << item.name << std::endl;
    std::cout << "2. Uninstall " << item.name << std::endl;
    if (item.has_config)
      std::cout << "3. Configure " << item.name << std::endl;

    std::cout << "0. Return" << std::endl;
    std::cout << "------------------------------------------";
    std::cout << "\nChoice: ";

    std::string choice;
    std::cin >> choice;
    std::cin.ignore(); // 吸收换行符

    if (choice == "1")
      run_install_action(item.installer.get(), item.type);
    else if (choice == "2")
      run_uninstall_action(item.installer.get(), item.type);
    else if (choice == "3") {
      item.installer->configure();
      std::cout << "Press Enter...";
      std::cin.ignore();
    } else if (choice == "0")
      return;
    else
      std::cout << RED << "Invalid choice." << NC << std::endl;
  }
}

// ---------------- Level 1 Menu ----------------
void external_software() {
  while (true) {
    system("clear");
    external_manager_title();
    std::cout << std::endl;

    std::map<int, int> menu_map;
    int index = 1;

    for (size_t i = 0; i < SOFTWARE_LIST.size(); ++i) {
      std::cout << index << ". " << SOFTWARE_LIST[i].name << std::endl;
      menu_map[index] = i;
      ++index;
    }

    std::cout << "\nMore software coming soon..." << std::endl;
    std::cout << "0. Return to Main Toolbox" << std::endl;
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "Choice: ";

    std::string user_choice;
    std::cin >> user_choice;
    std::cin.ignore(); // 吸收换行符

    if (user_choice == "0")
      return;
    int choice = std::stoi(user_choice);
    if (menu_map.count(choice)) {
      software_action_menu(SOFTWARE_LIST.at(menu_map[choice]));
    } else {
      std::cout << RED << "Invalid choice." << NC << std::endl;
    }
  }
}
