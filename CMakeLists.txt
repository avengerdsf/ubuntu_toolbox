cmake_minimum_required(VERSION 3.10)
project(SystemToolsToolbox)

# 指定 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 头文件
include_directories(${PROJECT_SOURCE_DIR}/include)
# 可执行文件
add_executable(main_toolbox
    src/main_toolbox.cpp
    src/usb_manager.cpp
    src/external_software.cpp
    src/template_generator.cpp
    src/soft_manager/soft_manager.cpp
    src/soft_manager/clash_verge_installer.cpp
    src/soft_manager/sogou_installer.cpp
    src/soft_manager/tmux_installer.cpp
)

# 如果需要静态编译 (可移植到其他 Linux)
set_target_properties(main_toolbox PROPERTIES LINK_FLAGS "-static")
