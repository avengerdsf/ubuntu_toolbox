#include "external_software.h"
#include "unit_color.h" 

namespace fs = std::filesystem;
    
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
                    if (line == config_line) return; // 已存在，无需写入
                }
            }
    
            // 写入配置
            std::ofstream outfile(tmux_conf, std::ios::app);
            if (outfile.is_open()) {
                outfile << "\n" << config_line << "\n";
                outfile.close();
                
                // 修复权限 (因为当前是以 sudo 运行的)
                const char* sudo_user = std::getenv("SUDO_USER");
                if (sudo_user) {
                    std::string chown_cmd = "chown " + std::string(sudo_user) + ":" + std::string(sudo_user) + " " + tmux_conf;
                    system(chown_cmd.c_str());
                }
                print_info("Added mouse support to " + tmux_conf, GREEN);
            } else {
                print_info("Failed to write to .tmux.conf", RED);
            }
        }