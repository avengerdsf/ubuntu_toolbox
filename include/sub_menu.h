#pragma once
#include <string>
#include "common_utils.h"
#include "unit_color.h"
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
#include <map>
#include <memory>
#include <limits> // 用于 numeric_limits
#include "external_software.h"
#include <regex>
#include <sstream>

extern const std::string TEMP_DIR;

void usb_serial_menu();

void external_software();

void template_generator_menu();

void conda_setup_menu();