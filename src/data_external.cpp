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


/* example: "/usr/local/share/gendlopen/templates/" */
#ifndef DEFAULT_TEMPLATES_PATH
#define DEFAULT_TEMPLATES_PATH  "templates/"
#endif



namespace templates
{
#define TEMPLATE(x) \
    std::vector<template_t> data_##x; \
    const template_t *ptr_##x = nullptr;

#include "list.h"
}


namespace /* anonymous */
{
#ifdef _MSC_VER
void get_templates_path_msvc(std::string &path)
{
    char *buf;
    size_t len;

    if (_dupenv_s(&buf, &len, TEMPLATES_ENV) == 0) {
        if (buf && *buf) {
            path = buf;
        }

        free(buf);
    }
}
#endif


/* return path to template files */
std::string get_templates_path()
{
    std::string path = DEFAULT_TEMPLATES_PATH;

#ifdef _MSC_VER
    get_templates_path_msvc(path);
#else
    char *env = getenv(TEMPLATES_ENV);

    if (env && *env) {
        path = env;
    }
#endif

    /* append missing path separator */
    switch (utils::str_back(path))
    {
#ifdef _WIN32
    case '\\':
#endif
    case '/':
        break;
    default:
        path.push_back('/');
        break;
    }

    return path;
}


/* find template and load it into memory */
template_t *load_from_file(std::vector<template_t> &data, const char *filename)
{
    std::string path, buf;
    template_t entry;
    bool eof = false;

    /* lookup path */
    path = get_templates_path() + filename;

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
        eof = gendlopen::get_lines(fp, buf, entry);
        data.push_back(entry);
    }

    entry = { "", 0, 0 };
    data.push_back(entry);

    return data.data();
}

} /* end anonymous namespace */


/* load template into memory */
void data::load_template(templates::name file)
{
#define CASE_X(x, NAME) \
    case templates::file_##x: \
        if (!templates::ptr_##x) { \
            templates::ptr_##x = load_from_file(templates::data_##x, NAME); \
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
void data::dump_templates()
{
    std::cerr << "gendlopen was build without embedded resources!" << std::endl;
}
