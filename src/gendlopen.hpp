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
    const char *filename_macros();
    const char *license();
    void concat_templates(cstrList_t&, cstrList_t&, output::format, bool);
}

#endif //_GENDLOPEN_HPP_
