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


namespace templates
{
#define TEMPLATE(FILE, VAR) \
    std::vector<template_t> data_##VAR; \
    extern const template_t *ptr_##VAR;

#include "list.h"
#undef TEMPLATE
}

namespace fs = std::filesystem;
namespace t = templates;


namespace /* anonymous */
{
    /* find template and load it into memory */
    void load_from_file(std::vector<template_t> &data, const std::string &dir, const char *filename, bool line_directive)
    {
        bool rv = true;

        /* don't add line directive to license part */
        if (strcmp(filename, "license.h") == 0) {
            line_directive = false;
        }

        /* lookup path */
        auto path = utils::make_fs_path(dir, filename);

        /* open file for reading */
        open_file ofs(path);

        if (!ofs.is_open()) {
            throw gendlopen::error("failed to open file for reading: " + path.string());
        }

        FILE *fp = ofs.file_pointer();

        template_t entry;
        entry.maybe_keyword = false;
        entry.line_count = 1;

        /* add initial #line directive */
        if (line_directive) {
            if (fp == stdin) {
                entry.data = "#line 1 \"<STDIN>\"";
            } else {
                entry.data = "#line 1 \"" + std::string(filename) + "\"\n";
            }

            data.push_back(entry);
        }

        /* read lines */
        while (rv) {
            rv = utils::get_lines(fp, entry);
            data.push_back(entry);
        }

        entry = { {}, 0, 0 };
        data.push_back(entry);
    }

    /* save template data to file */
    void save_to_file(const std::string &dir, const char *filename, const template_t *list)
    {
        cio::ofstream ofs;

        auto path = utils::make_fs_path(dir, filename);

        if (!ofs.open(path)) {
            throw gendlopen::error("failed to open file for writing: " + path.string());
        }

        std::cout << "saving " << path << std::endl;

        for ( ; list->line_count != 0; list++) {
            if (list->line_count == 1) {
                ofs << list->data << '\n';
            } else {
                /* we need to append `@' symbols to multiline data */
                for (const char &c : list->data) {
                    if (c == '\n') {
                        ofs << "@\n";
                    } else {
                        ofs << c;
                    }
                }

                ofs << '\n';
            }
        }
    }
}


/* load template into memory */
void gendlopen::load_template(templates::name file)
{
    if (m_templates_path.empty()) {
        /* nothing to load */
        return;
    }

    switch (file)
    {
#define TEMPLATE(FILE, VAR) \
    case templates::file_##VAR: \
        load_from_file(templates::data_##VAR, m_templates_path, #FILE, m_line_directive); \
        templates::ptr_##VAR = templates::data_##VAR.data(); \
        break;

#include "list.h"
#undef TEMPLATE

    default:
        break;
    }
}


/* create template data */
void gendlopen::create_template_lists(vtemplate_t &header, vtemplate_t &body)
{
    if (m_format == output::c)
    {
        load_template(t::file_common_header);
        load_template(t::file_c_header_1);
        load_template(t::file_c_header_wrap);
        load_template(t::file_c_header_end);
        load_template(t::file_c_body_1);
        load_template(t::file_c_body_2);
        load_template(t::file_c_body_3);
        load_template(t::file_c_body_4);
        load_template(t::file_c_body_5);
        load_template(t::file_c_body_6);
        load_template(t::file_c_body_7);
        load_template(t::file_c_body_8);
        load_template(t::file_c_body_9);
        load_template(t::file_c_body_wrap);
        load_template(t::file_c_body_end);

        /* header */
        header.push_back(t::ptr_common_header);
        header.push_back(t::ptr_c_header_1);
        header.push_back(t::ptr_c_header_wrap);
        header.push_back(t::ptr_c_header_end);

        /* body */
        vtemplate_t *vec = m_separate ? &body : &header;
        vec->push_back(t::ptr_c_body_1);
        vec->push_back(t::ptr_c_body_2);
        vec->push_back(t::ptr_c_body_3);
        vec->push_back(t::ptr_c_body_4);
        vec->push_back(t::ptr_c_body_5);
        vec->push_back(t::ptr_c_body_6);
        vec->push_back(t::ptr_c_body_7);
        vec->push_back(t::ptr_c_body_8);
        vec->push_back(t::ptr_c_body_9);
        vec->push_back(t::ptr_c_body_wrap);
        vec->push_back(t::ptr_c_body_end);
    }
    else if (m_format == output::plugin) {
        load_template(t::file_plugin_header);
        load_template(t::file_plugin_body);
        load_template(t::file_common_header);

        header.push_back(t::ptr_common_header);
        header.push_back(t::ptr_plugin_header);

        if (m_separate) {
            body.push_back(t::ptr_plugin_body);
        } else {
            header.push_back(t::ptr_plugin_body);
        }
    }
    else if (m_format == output::minimal) {
        load_template(t::file_min_c_header);
        header.push_back(t::ptr_min_c_header);
    }
    else if (m_format == output::minimal_cxx) {
        load_template(t::file_min_cxx_header);
        header.push_back(t::ptr_min_cxx_header);
    }
}


/* dump templates */
void gendlopen::dump_templates()
{
    if (!fs::exists(fs::status(m_dump_path)) && !fs::create_directories(m_dump_path)) {
        throw error("failed to create directory: " + m_dump_path);
    }

#define TEMPLATE(FILE, VAR) save_to_file(m_dump_path, #FILE, templates::ptr_##VAR);
#include "list.h"
#undef TEMPLATE
}

