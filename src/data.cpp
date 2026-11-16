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
    extern const char *VAR; \
    [[maybe_unused]] const char *filename_##VAR = #FILE;

#include "list.h"
#undef TEMPLATE
}

namespace fs = std::filesystem;
namespace t = templates;


namespace /* anonymous */
{
    /* find template and load it into memory */
    void load_from_file(std::string &data, const std::string &dir, const char *filename, bool line_directive)
    {
        /* lookup path */
        auto path = utils::make_fs_path(dir, filename);

        /* open file for reading */
        open_file ofs(path);

        if (!ofs.is_open()) {
            throw gendlopen::error("failed to open file for reading: " + path.string());
        }

        /* add initial #line directive */
        if (line_directive) {
            if (ofs.file_pointer() == stdin) {
                data += "#line 1 \"<STDIN>\"";
            } else {
                data += "#line 1 \"" + std::string(filename) + "\"\n";
            }
        }

        /* read lines */
        char c;

        while (ofs.getc(c)) {
            data += c;
        }
    }

    /* save template data to file */
    void save_to_file(const std::string &dir, const char *filename, const char *data)
    {
        cio::ofstream ofs;

        auto path = utils::make_fs_path(dir, filename);

        if (!ofs.open(path)) {
            throw gendlopen::error("failed to open file for writing: " + path.string());
        }

        ofs << data;

        std::cout << "file saved: " << path << std::endl;
    }
}


/* load template data into memory */
void gendlopen::load_template(const char *ptr, std::string &buffer)
{
    if (m_templates_path.empty()) {
        buffer += ptr;
        return;
    }

#define TEMPLATE(FILE, VAR) \
    if (ptr == t::VAR) {\
        load_from_file(buffer, m_templates_path, #FILE, m_line_directive); \
        return; \
    }
#include "list.h"
#undef TEMPLATE

    throw error("gendlopen::load_template() failed to load data");
}


/* create template data */
void gendlopen::create_template_lists(std::string &header, std::string &body)
{
    auto add_msg = [] (std::string &buf, const char *filename) {
        buf += "/* template file \"<built-in>/";
        buf += filename;
        buf += "\" was skipped */\n\n";
    };

    switch (m_format)
    {
    case output::c:
        load_template(t::common_header, header);
        load_template(t::c_header_1, header);

        load_template(t::c_body_1, body);
        load_template(t::c_body_2, body);
        load_template(t::c_body_3, body);
        load_template(t::c_body_4, body);
        load_template(t::c_body_5, body);
        load_template(t::c_body_6, body);
        load_template(t::c_body_7, body);
        load_template(t::c_body_8, body);
        load_template(t::c_body_9, body);

        if (m_parameter_names == param::skip) {
            add_msg(header, t::filename_c_header_wrap);
            add_msg(body, t::filename_c_body_wrap);
        } else {
            load_template(t::c_header_wrap, header);
            load_template(t::c_body_wrap, body);
        }

        load_template(t::c_header_end, header);
        load_template(t::c_body_end, body);
        break;

    case output::plugin:
        load_template(t::common_header, header);
        load_template(t::plugin_header, header);
        load_template(t::plugin_body, body);
        break;

    case output::minimal:
        load_template(t::min_c_header, header);
        break;

    case output::minimal_cxx:
        load_template(t::min_cxx_header, header);
        break;
    }
}


/* dump templates */
void gendlopen::dump_templates()
{
    if (!fs::exists(fs::status(m_dump_path)) && !fs::create_directories(m_dump_path)) {
        throw error("failed to create directory: " + m_dump_path);
    }

#define TEMPLATE(FILE, VAR) save_to_file(m_dump_path, #FILE, templates::VAR);
#include "list.h"
#undef TEMPLATE
}

