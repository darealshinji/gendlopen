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


/* silence `unused reference' compiler warnings */
template<typename T>
void UNUSED_REF(T x) {
    static_cast<void>(x);
}


namespace templates
{
#ifdef USE_EXTERNAL_RESOURCES

# define TEMPLATE(FILE, VAR) \
    std::vector<template_t> data_##VAR; \
    const template_t *ptr_##VAR = nullptr;
# include "list.h"
# undef TEMPLATE

#else

# include "template.h"

#endif
}

namespace fs = std::filesystem;
namespace t = templates;


namespace /* anonymous */
{
#ifdef USE_EXTERNAL_RESOURCES

    /* find template and load it into memory */
    void load_from_file(std::vector<template_t> &data, const std::string &dir, const char *file, bool line_directive)
    {
        std::string path, line;
        template_t entry;
        bool rv = true;

        /* don't add line directive to license part */
        if (strcmp(file, "license.h") == 0) {
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
    }

#else //!USE_EXTERNAL_RESOURCES

    /* save template data to file */
    void save_to_file(std::string path, const char *filename, const template_t *list)
    {
        cio::ofstream ofs;

        utils::append_missing_separator(path);
        path += filename;

        if (!ofs.open(path)) {
            throw gendlopen::error(std::string("failed to open file for writing: ") + path);
        }

        std::cout << "saving " << path << std::endl;

        for ( ; list->line_count != 0; list++) {
            ofs << list->data << '\n';
        }
    }

#endif //!USE_EXTERNAL_RESOURCES
}


/* create template data */
void gendlopen::create_template_lists(vtemplate_t &header, vtemplate_t &body)
{
    auto concat_sources = [&] (const template_t *t_header, const template_t *t_body)
    {
        load_template(t::file_common_header);
        load_template(t::file_common_body);

        if (m_separate) {
            /* common header + header, common body + body */
            header.push_back(t::ptr_common_header);
            header.push_back(t_header);
            body.push_back(t::ptr_common_body);
            body.push_back(t_body);
        } else {
            /* common data + header + body */
            header.push_back(t::ptr_common_header);
            header.push_back(t::ptr_common_body);
            header.push_back(t_header);
            header.push_back(t_body);
        }
    };

    switch (m_format)
    {
    case output::c:
        load_template(t::file_c_header);
        load_template(t::file_c_body);
        concat_sources(t::ptr_c_header, t::ptr_c_body);
        break;

    case output::cxx:
        load_template(t::file_cxx_header);
        load_template(t::file_cxx_body);
        concat_sources(t::ptr_cxx_header, t::ptr_cxx_body);
        break;

    case output::plugin:
        load_template(t::file_plugin_header);
        load_template(t::file_plugin_body);
        concat_sources(t::ptr_plugin_header, t::ptr_plugin_body);
        break;

    case output::minimal:
        load_template(t::file_min_c_header);
        header.push_back(t::ptr_min_c_header);
        break;

    case output::minimal_cxx:
        load_template(t::file_min_cxx_header);
        header.push_back(t::ptr_min_cxx_header);
        break;

    [[unlikely]] case output::error:
        throw error(std::string(__func__) + ": m_format == output::error");
    }
}


/* load template into memory */
void gendlopen::load_template(templates::name file)
{
#ifdef USE_EXTERNAL_RESOURCES
    switch (file)
    {
# define TEMPLATE(FILE, VAR) \
    case templates::file_##VAR: \
        load_from_file(templates::data_##VAR, m_templates_path, #FILE, m_line_directive); \
        templates::ptr_##VAR = templates::data_##VAR.data(); \
        break;

# include "list.h"
# undef TEMPLATE

    default:
        break;
    }
#endif //USE_EXTERNAL_RESOURCES

    UNUSED_REF(file);
}


/* dump templates */
void gendlopen::dump_templates(const char *dir)
{
#if !defined(USE_EXTERNAL_RESOURCES)
    auto st = fs::symlink_status(dir);

    if (!fs::exists(st) && !fs::create_directories(dir)) {
        throw error(std::string("failed to create directory: ") + dir);
    }

# define TEMPLATE(FILE, VAR) \
    save_to_file(dir, #FILE, templates::ptr_##VAR);

# include "list.h"
# undef TEMPLATE

#endif //!USE_EXTERNAL_RESOURCES

    UNUSED_REF(dir);
}

