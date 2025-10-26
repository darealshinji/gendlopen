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

#ifdef USE_EXTERNAL_RESOURCES

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
void load_from_file(std::vector<template_t> &data, const std::string &dir, const char *file, bool line_directive)
{
    std::string path, buf;
    template_t entry;
    bool eof = false;

    /* lookup path */
    path = dir + file;

    /* open file for reading */
    open_file ofs(path);

    if (!ofs.is_open()) {
        throw gendlopen::error("failed to open file for reading: " + path);
    }

    FILE *fp = ofs.file_pointer();

    /* add initial #line directive */
    if (line_directive) {
        if (fp == stdin) {
            buf = "#line 1 \"<STDIN>\"";
        } else {
            buf = "#line 1 \"" + std::string(file) + "\"\n";
        }

        entry = { buf, false, 1 };
        data.push_back(entry);
    }

    /* read lines */
    while (!eof) {
        eof = utils::get_lines(fp, buf, entry);
        data.push_back(entry);
    }

    entry = { "", 0, 0 };
    data.push_back(entry);
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
#define CASE_X(x, NAME, LINE_DIRECTIVE) \
    case templates::file_##x: \
        if (!templates::ptr_##x) { \
            load_from_file(templates::data_##x, m_templates_path, NAME, LINE_DIRECTIVE); \
            templates::ptr_##x = templates::data_##x.data(); \
        } \
        break

    const bool b = m_line_directive;

    switch (file)
    {
        /* never add line directive to license part */
        CASE_X(license,         "license.h",         false);
        CASE_X(filename_macros, "filename_macros.h", b);
        CASE_X(common_header,   "common.h",          b);
        CASE_X(c_header,        "c.h",               b);
        CASE_X(c_body,          "c.c",               b);
        CASE_X(cxx_header,      "cxx.hpp",           b);
        CASE_X(cxx_body,        "cxx.cpp",           b);
        CASE_X(min_c_header,    "minimal.h",         b);
        CASE_X(min_cxx_header,  "minimal_cxxeh.hpp", b);
        CASE_X(plugin_header,   "plugin.h",          b);
        CASE_X(plugin_body,     "plugin.c",          b);
    }
}


/* dummy */
void gendlopen::dump_templates()
{
}

#endif /* USE_EXTERNAL_RESOURCES */
