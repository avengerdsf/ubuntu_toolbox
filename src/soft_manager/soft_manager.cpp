#include "../../include/external_software.h"
#include "../../include/unit_color.h"

std::vector<SoftwareItem> SOFTWARE_LIST;

static void init_software_list() {
  SOFTWARE_LIST.emplace_back(
      SoftwareItem{"clash-verge", "deb",
                   std::make_unique<ClashInstaller>(
                       "clash-verge", "ClashVerge",
                       "https://down.clashchinese.com/soft/CVR/"
                       "clashchinese.com_Clash.Verge_2.1.2_amd64.deb",
                       "https://down.clashchinese.com/soft/CVR/"
                       "clashchinese.com_Clash.Verge_2.1.2_arm64.deb")});

  SOFTWARE_LIST.emplace_back(SoftwareItem{
      "tmux", "apt", std::make_unique<TmuxInstaller>("tmux", "Tmux"), true});

  SOFTWARE_LIST.emplace_back(SoftwareItem{
      "Sogou IME (搜狗输入法)", "deb",
      std::make_unique<SogouInstaller>("sogoupinyin", "Sogou IME",
                                       "https://pinyin.sogou.com/linux"),
      true});
}

// 确保全局初始化
struct SoftwareListInit {
  SoftwareListInit() { init_software_list(); }
};

static SoftwareListInit _software_list_init;

bool SoftwareInstaller::is_installed() {
  std::string cmd = "dpkg -s " + pkg_id + " > /dev/null 2>&1";
  return system(cmd.c_str()) == 0;
}

int SoftwareInstaller::exec_system(const std::string &cmd) {
  return system(cmd.c_str());
}

std::string SoftwareInstaller::create_temp_dir() {
  if (access(TEMP_DIR.c_str(), F_OK) != 0) { // F_OK 检查是否存在
    mkdir(TEMP_DIR.c_str(), 0755);           // 创建目录
  }
  return TEMP_DIR; // 直接返回固定目录
}

void SoftwareInstaller::print_info(const std::string &msg,
                                   const std::string &color) {
  std::cout << color << msg << "\033[0m" << std::endl;
}

std::string SoftwareInstaller::get_user_home() const {
  const char *sudo_user = getenv("SUDO_USER");
  if (!sudo_user || sudo_user[0] == '\0') {
    // 理论上不会走到这里，强制 sudo
    return "";
  }
  return std::string("/home/") + sudo_user;
}

void SoftwareInstaller::clear_screen() const { system("clear"); }

std::string SoftwareInstaller::detect_arch() const {
  char buf[128];
  FILE *pipe = popen("uname -m", "r");
  if (!pipe)
    return "unknown";
  fgets(buf, sizeof(buf), pipe);
  pclose(pipe);

  std::string arch(buf);
  arch.erase(arch.find_last_not_of(" \n\r\t") + 1); // 去掉换行

  if (arch == "x86_64" || arch == "amd64")
    return "amd64";
  else if (arch == "aarch64" || arch == "arm64")
    return "arm64";
  return "unknown";
}

bool SoftwareInstaller::uninstall() {
  if (!is_installed()) {
    print_info(display_name + " not installed.", RUL_YELLOW);
    return false;
  }

  print_info("Uninstalling: " + display_name, BLUE);
  std::string uninstall_cmd = "apt purge -y " + pkg_id;
  if (exec_system(uninstall_cmd) == 0) {
    print_info("Successfully removed.", GREEN);
    return true;
  } else {
    print_info("Uninstall failed.", RED);
    return false;
  }
}
