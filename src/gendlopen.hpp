#ifndef _GENDLOPEN_HPP_
#define _GENDLOPEN_HPP_

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
#include "cio_ifstream.hpp"
#include "cio_ofstream.hpp"
#include "gendlopen_class.hpp"

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

#endif //_GENDLOPEN_HPP_
