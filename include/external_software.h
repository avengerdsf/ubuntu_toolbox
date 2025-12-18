#pragma once
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

extern const std::string TEMP_DIR;

class SoftwareInstaller;
class ClashInstaller;
class SogouInstaller;
class TmuxInstaller;

// SoftwareItem 结构体
struct SoftwareItem {
  std::string name;
  std::string type;
  std::unique_ptr<SoftwareInstaller> installer;
  bool has_config = false;
};

extern std::vector<SoftwareItem> SOFTWARE_LIST;
// 初始化软件列表

class SoftwareInstaller {
public:
  SoftwareInstaller(const std::string &pkg, const std::string &name)
      : pkg_id(pkg), display_name(name) {}
  virtual ~SoftwareInstaller() = default;
  bool uninstall();
  virtual void install() = 0;
  virtual void configure() {} // 默认空实现，子类可覆盖

protected:
  bool is_installed();
  int exec_system(const std::string &cmd);
  std::string create_temp_dir();
  void print_info(const std::string &msg, const std::string &color = "\033[0m");
  std::string get_user_home() const;
  void clear_screen() const;
  std::string detect_arch() const;

  const std::string pkg_id;
  const std::string display_name;
};

class ClashInstaller : public SoftwareInstaller {
public:
  ClashInstaller(const std::string &pkg_id, const std::string &display_name,
                 const std::string &url_amd64, const std::string &url_arm64);

  void install() override;

private:
  std::string url_amd64;
  std::string url_arm64;

  std::string get_target_url();
};

class SogouInstaller : public SoftwareInstaller {
public:
  SogouInstaller(const std::string &pkg_id, const std::string &display_name,
                 const std::string &website_url);

  void install() override;
  void configure() override; // 可选：配置 fcitx

private:
  std::string website_url;

  std::string get_default_download_dir();
  std::string find_deb_file(const std::string &user_home,
                            const std::string &arch);
};

class TmuxInstaller : public SoftwareInstaller {
public:
  TmuxInstaller(const std::string &pkg_id, const std::string &display_name);
  void install() override;
  void configure() override;

private:
};
