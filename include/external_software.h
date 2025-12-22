#ifndef EXTERNAL_SOFTWARE_H
#define EXTERNAL_SOFTWARE_H

#include <string>
#include <vector>
#include <map>
#include <memory>
#include <functional>

#include <iostream>
#include <fstream>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <unistd.h>
#include <array>

enum class InstallType {
    APT, DEB_URL, DEB_MANUAL, SCRIPT
};

struct SoftwareConfig {
    std::string id;
    std::string display_name;
    InstallType type;
    std::string pkg_name;
    std::map<std::string, std::string> arch_urls; 
    std::vector<std::string> dependencies;
    std::string website;
};

class SoftwareInstaller {
public:
    explicit SoftwareInstaller(SoftwareConfig config);
    virtual ~SoftwareInstaller() = default;

    void install();
    void uninstall();

    std::string get_id() const { return config.id; }
    std::string get_name() const { return config.display_name; }
    
    // [修改点 1] 将 is_installed 移到 public 区域
    bool is_installed(); 

protected:
    SoftwareConfig config;
    virtual void post_install() {} 
    virtual void post_uninstall() {}

    // 工具函数
    std::string detect_arch();
    std::string get_user_home();
    void print_info(const std::string &msg, const std::string &color);
    int exec_sys(const std::string &cmd);
    std::string create_temp_dir();

private:
    void install_dependencies();
    bool perform_core_installation(); 
    std::string find_local_deb(const std::string& arch); 
};

struct SoftwareItem {
    std::string id;
    std::string category;
    std::unique_ptr<SoftwareInstaller> installer;
};


// Sogou Installer (自定义环境配置)
class SogouInstaller : public SoftwareInstaller {
    public:
        explicit SogouInstaller(SoftwareConfig config) : SoftwareInstaller(config) {}    
    protected:
        void post_install() override ;
};

class TmuxInstaller : public SoftwareInstaller {
    public:
    
        explicit TmuxInstaller(SoftwareConfig config) : SoftwareInstaller(config) {}

        
    protected:
        void post_install() override ;
};

extern std::vector<SoftwareItem> SOFTWARE_LIST;
void init_software_list();

#endif