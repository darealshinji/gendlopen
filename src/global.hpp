#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <cassert>
#include <cstring>

#include "types.hpp"
#include "utils.hpp"
#include "cio_ofstream.hpp"
#include "gendlopen.hpp"

/* help.cpp */
namespace help {
    void print(const char *prog);
    void print_full(const char *prog);
}

/* data.cpp */
namespace data {
    void save_filename_macros_data(cio::ofstream &ofs);
    void save_license_data(cio::ofstream &ofs);
    void concat_templates(cstrList_t &header, cstrList_t &body, output::format format, bool separate);
}

#endif /* GLOBAL_HPP */
