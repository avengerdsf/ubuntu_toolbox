#include "external_software.h"
#include "sub_menu.h"
#include "unit_color.h" 



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

void SoftwareInstaller::print_info(const std::string &msg, const std::string &color) {
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
    FILE* pipe = popen("dpkg --print-architecture", "r");
    if (!pipe) return "amd64"; // fallback
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
    const char* sudo_user = std::getenv("SUDO_USER");
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
    if (config.dependencies.empty()) return;
    
    print_info("Installing dependencies...", RUL_YELLOW);
    std::string deps = "";
    for (const auto& dep : config.dependencies) {
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
            if (exec_sys(wget_cmd) != 0) return false;
            
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

std::string SoftwareInstaller::find_local_deb(const std::string& arch) {
        // 防御性检查：目录不存在直接返回
        if (!fs::exists(TEMP_DIR)) return "";

        for (const auto& entry : fs::directory_iterator(TEMP_DIR)) {
            // 必须是普通文件，忽略子目录
            if (!entry.is_regular_file()) continue;

            if (entry.path().extension() == ".deb") {
                std::string filename = entry.path().filename().string();

                // 1. 核心匹配：文件名必须包含 config.id
                // 例如 config.id="sogou"，匹配 "sogoupinyin_4.2.1_amd64.deb"
                if (filename.find(config.id) == std::string::npos) {
                    continue;
                }

                // 2. 架构匹配：文件名必须包含当前架构 (如 amd64) 或者 通用架构 (all)
                // 这一步至关重要，防止在 amd64 机器上强行安装 arm64 包
                if (filename.find(arch) != std::string::npos || 
                    filename.find("_all") != std::string::npos || // 常见命名规则 name_version_all.deb
                    filename.find("-all") != std::string::npos) {  // 备用命名规则
                    
                    return entry.path().string();
                }
            }
        }
    return "";
}
