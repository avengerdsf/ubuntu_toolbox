cmake_minimum_required(VERSION 3.10)
project(SystemToolsToolbox)

# 1. 指定 C++ 标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# 2. 头文件路径
include_directories(${PROJECT_SOURCE_DIR}/include)

# 3. 定义源代码
add_executable(main_toolbox
    src/main_toolbox.cpp
    src/usb_manager.cpp
    src/external_software.cpp
    src/template_generator.cpp
    src/conda_setup.cpp
    src/common_utils.cpp
    src/soft_manager/soft_manager.cpp
    # src/soft_manager/clash_verge_installer.cpp
    src/soft_manager/sogou.cpp
    src/soft_manager/tmux.cpp
)

# 4. 关键配置：全静态链接设置
# -static: 强制链接所有库的静态版本 (libc, libstdc++, libgcc 等)
# -pthread: 即使是单线程程序，C++标准库某些部分可能也依赖线程库，建议加上
set(CMAKE_EXE_LINKER_FLAGS "-static -pthread")

# 如果你的代码中有用到 dlopen 等动态加载功能（通常 system() 不需要，但为了保险），
# 有时需要显式链接 dl 库，但在全静态模式下通常不适用，这里仅作提示。