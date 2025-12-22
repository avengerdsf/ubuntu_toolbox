#include "external_software.h"
#include "unit_color.h" 

namespace fs = std::filesystem;

void SogouInstaller::post_install() {
            print_info("Configuring Fcitx environment variables...", BLUE);
    
            // 1. 设置 fcitx 自启动
            if (fs::exists("/usr/share/applications/fcitx.desktop")) {
                system("cp /usr/share/applications/fcitx.desktop /etc/xdg/autostart/ 2>/dev/null || true");
            }
    
            // 2. 配置 /etc/environment
            std::string env_file = "/etc/environment";
            // 简单检查是否已配置
            int check = system("grep -q 'XMODIFIERS' /etc/environment");
            
            if (check != 0) {
                system("echo 'GTK_IM_MODULE=fcitx' | tee -a /etc/environment > /dev/null");
                system("echo 'QT_IM_MODULE=fcitx' | tee -a /etc/environment > /dev/null");
                system("echo 'XMODIFIERS=@im=fcitx' | tee -a /etc/environment > /dev/null");
                print_info("Environment variables set. Please relogin.", GREEN);
            } else {
                print_info("Environment variables already look configured.", RUL_YELLOW);
            }
        }