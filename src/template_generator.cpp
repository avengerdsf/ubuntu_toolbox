// template_generator.cpp
#include "sub_menu.h"
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <termios.h>
#include <unistd.h>
#include <vector>

using namespace std;

// ANSI Colors
const string RED = "\033[0;31m";
const string GREEN = "\033[0;32m";
const string YELLOW = "\033[1;33m";
const string CYAN = "\033[0;36m";
const string PURPLE = "\033[0;35m";
const string NC = "\033[0m";

// --- Module Configuration ---
vector<pair<string, string>> MODULES; // first: description, second: command
vector<int> SELECTED_STATE;
int HIGHLIGHT_IDX = 0;
vector<string> FINAL_COMMANDS;

// --- Load Modules ---
void load_modules() {
  // 简化版：直接硬编码模块，方便 C++ 使用
  MODULES = {{"ROSCore", "roscore"},
             {"UavLaunch", "roslaunch uav uav_control.launch"},
             {"Open RViz", "rviz"},
             {"Custom Command / Launch", "__CUSTOM__"}};
  SELECTED_STATE.assign(MODULES.size(), 0);
}

// --- Draw Checklist ---
void draw_checklist(const string &final_message) {
  cout << "\033[2J\033[H"; // 清屏 + 光标移动到左上角
  cout << CYAN
       << "===================================================================="
       << NC << "\n";
  cout << CYAN << "     " << YELLOW
       << "C++ Shell Script Template Generator (Native Mode)" << CYAN << "     "
       << NC << "\n";
  cout << CYAN
       << "===================================================================="
       << NC << "\n";
  cout << "Use " << YELLOW << "↑/↓" << NC << " to navigate, " << YELLOW
       << "SPACE" << NC << " to select, " << YELLOW << "ENTER" << NC
       << " to confirm, " << YELLOW << "Q" << NC << " to quit.\n";
  cout << "--------------------------------------------------------------------"
          "\n";

  for (size_t i = 0; i < MODULES.size(); ++i) {
    string check_mark =
        SELECTED_STATE[i] ? "[" + GREEN + "X" + NC + "]" : "[ ]";
    string line_start, line_end = NC;
    if ((int)i == HIGHLIGHT_IDX) {
      line_start = PURPLE + "\033[7m";
      line_end = "\033[0m" + NC;
    }
    cout << line_start << check_mark << " " << MODULES[i].first << "\033[K"
         << line_end << "\n";
  }

  cout << "More coming soon...\n";
  cout << "--------------------------------------------------------------------"
          "\n";
  cout << "Status: " << final_message << NC << "\n";
}

// --- Handle Input ---
int handle_input() {
  struct termios oldt, newt;
  tcgetattr(STDIN_FILENO, &oldt);
  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &newt);

  char key = 0;
  read(STDIN_FILENO, &key, 1);

  if (key == 27) { // ESC sequence
    char seq[2];
    read(STDIN_FILENO, seq, 2);
    if (seq[0] == '[') {
      if (seq[1] == 'A')
        HIGHLIGHT_IDX = (HIGHLIGHT_IDX - 1 + MODULES.size()) % MODULES.size();
      if (seq[1] == 'B')
        HIGHLIGHT_IDX = (HIGHLIGHT_IDX + 1) % MODULES.size();
    }
  } else if (key == ' ') { // SPACE
    SELECTED_STATE[HIGHLIGHT_IDX] = !SELECTED_STATE[HIGHLIGHT_IDX];
  } else if (key == '\n' || key == '\r') { // ENTER
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 1;
  } else if (key == 'q' || key == 'Q') { // Q
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    return 2;
  }

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  return 0;
}

// --- Run Checklist ---
int run_checklist() {
  string final_message = "Select modules above.";
  int loop_status = 0;
  cout << "\033[?25l"; // 隐藏光标
  while (loop_status == 0) {
    draw_checklist(final_message);
    loop_status = handle_input();
  }
  cout << "\033[?25h"; // 显示光标
  return loop_status;
}

// --- Process Selected Commands ---
int process_selected_commands() {
  FINAL_COMMANDS.clear();
  int pane_count = 0;
  for (size_t i = 0; i < MODULES.size(); ++i) {
    if (SELECTED_STATE[i]) {
      ++pane_count;
      if (MODULES[i].second == "__CUSTOM__") {
        cout << "Enter command for pane #" << pane_count << " ("
             << MODULES[i].first << "): ";
        string custom_cmd;
        getline(cin, custom_cmd);
        FINAL_COMMANDS.push_back(custom_cmd);
      } else {
        FINAL_COMMANDS.push_back(MODULES[i].second);
      }
    }
  }
  return pane_count;
}

// --- Generate Script ---
static std::string sanitize_tmux_name(std::string name) {
  for (char &c : name) {
    if (c == '.' || c == '/' || c == '\\' || c == ' ')
      c = '_';
  }
  return name;
}

void generate_tmux_layout(const std::string &full_path, int pane_count,
                          const std::vector<std::string> &commands) {
  /* ---------- ensure directory ---------- */
  std::string::size_type pos = full_path.find_last_of("/\\");
  if (pos != std::string::npos) {
    std::string dir = full_path.substr(0, pos);
    if (!dir.empty())
      mkdir(dir.c_str(), 0755);
  }

  /* ---------- overwrite check ---------- */
  std::ifstream f_check(full_path);
  if (f_check.good()) {
    std::cout << YELLOW << "File '" << full_path
              << "' already exists. Overwrite? [y/N]: " << NC;
    char choice;
    std::cin >> choice;
    std::cin.ignore();
    if (choice != 'y' && choice != 'Y') {
      std::cout << YELLOW << "Generation cancelled." << NC << "\n";
      return;
    }
  }

  /* ---------- session name ---------- */
  std::string base =
      (pos == std::string::npos) ? full_path : full_path.substr(pos + 1);
  base = sanitize_tmux_name(base);

  /* ---------- generate script ---------- */
  std::ofstream ofs(full_path);
  if (!ofs.is_open()) {
    std::cerr << RED << "Failed to open file: " << full_path << NC << "\n";
    return;
  }

  ofs << "#!/bin/bash\n\n";

  ofs << "SESSION=\"" << base << "_$(date +%s)\"\n\n";

  ofs << "if ! command -v tmux &> /dev/null; then\n";
  ofs << "  echo \"Error: tmux is not installed.\"\n";
  ofs << "  exit 1\n";
  ofs << "fi\n\n";

  ofs << "if [ -n \"$SUDO_USER\" ]; then\n";
  ofs << "  TMUX=\"sudo -u $SUDO_USER tmux\"\n";
  ofs << "else\n";
  ofs << "  TMUX=\"tmux\"\n";
  ofs << "fi\n\n";

  ofs << "$TMUX kill-session -t \"$SESSION\" 2>/dev/null || true\n";
  ofs << "$TMUX new-session -d -s \"$SESSION\" -n main\n\n";

  for (int i = 1; i < pane_count; ++i) {
    ofs << "$TMUX split-window -v -t \"$SESSION:main\"\n";
  }

  ofs << "$TMUX select-layout -t \"$SESSION:main\" tiled\n\n";

  ofs << "# --- Commands Injection ---\n";
  for (int i = 0; i < pane_count; ++i) {
    ofs << "$TMUX send-keys -t \"$SESSION:main." << i << "\" \"" << commands[i]
        << "\" C-m\n";
  }

  ofs << "\n$TMUX attach-session -t \"$SESSION\"\n";

  ofs.close();
  chmod(full_path.c_str(), 0644);

  std::cout << GREEN << "Successfully generated '" << full_path << "' with "
            << pane_count << " panes!" << NC << "\n";
  std::cout << YELLOW << "Press ENTER to continue..." << NC;
  std::cin.get();
}

// --- Reset State ---
void reset_selection_state() {
  SELECTED_STATE.assign(MODULES.size(), 0);
  HIGHLIGHT_IDX = 0;
}

std::string expand_user_path(const std::string &path) {
  if (path.empty())
    return path;

  if (path[0] == '~') {
    std::string home;

    // 优先使用 sudo 用户的 home
    const char *sudo_user = getenv("SUDO_USER");
    if (sudo_user) {
      home = "/home/";
      home += sudo_user;
    } else {
      const char *env_home = getenv("HOME");
      if (env_home)
        home = env_home;
    }

    if (home.empty())
      return path; // fallback

    if (path.size() == 1)
      return home; // "~"
    if (path[1] == '/')
      return home + path.substr(1); // "~/xxx"
  }

  return path; // 非 ~ 开头
}

// --- Main Menu Interface ---
void template_generator_menu() {
  load_modules();
  reset_selection_state();
  int status = run_checklist();
  if (status == 2) {
    cout << YELLOW << "Selection cancelled." << NC << "\n";
    return;
  }

  cout << "Enter output path and filename (default: ./start_all.sh): ";
  string full_path;
  getline(cin, full_path);
  if (full_path.empty())
    full_path = "./start_all.sh";
  full_path = expand_user_path(full_path);

  int pane_count = process_selected_commands();
  if (pane_count == 0) {
    cout << YELLOW << "No modules selected. Returning to menu." << NC << "\n";
    return;
  }

  generate_tmux_layout(full_path, pane_count, FINAL_COMMANDS);
}
