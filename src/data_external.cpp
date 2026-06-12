/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2023-2026 Carsten Janssen

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
#define TEMPLATE(FILE, VAR) \
    std::vector<template_t> data_##VAR; \
    const template_t *ptr_##VAR = nullptr;

#include "list.h"

#undef TEMPLATE
}


namespace /* anonymous */
{

/* find template and load it into memory */
void load_from_file(const template_t *&ptr, std::vector<template_t> &data, const std::string &dir, const char *file, bool line_directive)
{
    std::string path, line;
    template_t entry;
    bool rv = true;

    if (ptr) {
        /* pointer is already assigned */
        return;
    }

    /* don't add line directive to license part */
    if (::strcmp(file, "license.h") == 0) {
        line_directive = false;
    }

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
            line = "#line 1 \"<STDIN>\"";
        } else {
            line = "#line 1 \"" + std::string(file) + "\"\n";
        }

        entry = { line, false, 1 };
        data.push_back(entry);
    }

    /* read lines */
    while (rv) {
        rv = utils::get_lines(fp, line, entry);
        data.push_back(entry);
    }

    entry = { "", 0, 0 };
    data.push_back(entry);

    /* assign pointer */
    ptr = data.data();
}

} /* end anonymous namespace */


/* load template into memory */
void gendlopen::load_template(templates::name file)
{
#define TEMPLATE(FILE, VAR) \
    case templates::file_##VAR: \
        load_from_file(templates::ptr_##VAR, templates::data_##VAR, m_templates_path, #FILE, m_line_directive); \
        break;

    switch (file)
    {
#include "list.h"
    }
}


/* dummy */
void gendlopen::dump_templates(const char *)
{
}

#endif /* USE_EXTERNAL_RESOURCES */
