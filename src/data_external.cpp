/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2023-2025 Carsten Janssen

 Permission is hereby  granted, free of charge, to any  person obtaining a copy
 of this software and associated  documentation files (the "Software"), to deal
 in the Software  without restriction, including without  limitation the rights
 to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
 copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
 IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
 FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
 AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
 LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
**/

#include <filesystem>
#include <iostream>
#include <ostream>
#include <string>
#include <vector>
#include <cstdlib>
#include "cio_ofstream.hpp"
#include "open_file.hpp"
#include "gendlopen.hpp"
#include "types.hpp"
#include "utils.hpp"


namespace templates
{
#define TEMPLATE(x) \
    std::vector<template_t> data_##x; \
    const template_t *ptr_##x = nullptr;

#include "list.h"
}


namespace /* anonymous */
{

/* find template and load it into memory */
template_t *load_from_file(std::vector<template_t> &data, const std::string &templates_path, const char *filename)
{
    std::string path, buf;
    template_t entry;
    bool eof = false;

    /* lookup path */
    path = templates_path + filename;

    /* open file for reading */
    open_file file(path);

    if (!file.is_open()) {
        throw gendlopen::error("failed to open file for reading: " + path);
    }

    FILE *fp = file.file_pointer();

    /* add initial #line directive */
    if (fp == stdin) {
        buf = "#line 1 \"<STDIN>\"";
    } else {
        buf = "#line 1 \"" + std::string(filename) + "\"\n";
    }

    entry = { buf, false, 1 };
    data.push_back(entry);

    /* read lines */
    while (!eof) {
        eof = utils::get_lines(fp, buf, entry);
        data.push_back(entry);
    }

    entry = { "", 0, 0 };
    data.push_back(entry);

    return data.data();
}

} /* end anonymous namespace */



void gendlopen::get_templates_path_env()
{
#ifdef _MSC_VER
    char *buf;
    size_t len;

    if (_dupenv_s(&buf, &len, TEMPLATES_ENV) == 0) {
        if (buf && *buf) {
            m_templates_path = buf;
        }

        free(buf);
    }
#else
    char *env = getenv(TEMPLATES_ENV);

    if (env && *env) {
        m_templates_path = env;
    }
#endif /* !_MSC_VER */

    utils::append_missing_separator(m_templates_path);
}


/* load template into memory */
void gendlopen::load_template(templates::name file)
{
#define CASE_X(x, NAME) \
    case templates::file_##x: \
        if (!templates::ptr_##x) { \
            templates::ptr_##x = load_from_file(templates::data_##x, m_templates_path, NAME); \
        } \
        break

    switch (file)
    {
    CASE_X(license, "license.h");
    CASE_X(filename_macros, "filename_macros.h");
    CASE_X(common_header, "common.h");
    CASE_X(c_header, "c.h");
    CASE_X(c_body, "c.c");
    CASE_X(cxx_header, "cxx.hpp");
    CASE_X(cxx_body, "cxx.cpp");
    CASE_X(min_c_header, "minimal.h");
    CASE_X(min_cxx_header, "minimal_cxxeh.hpp");
    CASE_X(plugin_header, "plugin.h");
    CASE_X(plugin_body, "plugin.c");
    }
}


/* stub */
void gendlopen::dump_templates()
{
    std::cerr << "gendlopen was build without embedded resources!" << std::endl;
}
