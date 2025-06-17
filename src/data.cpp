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

/* use a separate compilation unit for the template data */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <ostream>
#include <string>
#include <cstdlib>
#include "cio_ofstream.hpp"
#include "filesystem_compat.hpp"
#include "gendlopen.hpp"
#include "types.hpp"

namespace templates
{
#include "template.h"
}


static void save_to_file(const std::string &filename, const template_t *list)
{
    cio::ofstream ofs;

    if (!ofs.open(filename)) {
        throw gendlopen::error("failed to open file for writing: " + filename);
    }

    std::cout << "saving " << filename << std::endl;

    for ( ; list->data != NULL; list++) {
        ofs << list->data << '\n';
    }
}

static size_t save_data(bool line_directive, const template_t *list, size_t line_count)
{
    if (!line_directive && strncmp(list->data, "#line", 5) == 0) {
        /* skip initial line directive */
        list++;
        line_count--;
    }

    for ( ; list->data != NULL; list++) {
        save::ofs << list->data << '\n';
    }

    return line_count;
}

/* license */
size_t data::license(bool line_directive)
{
    return save_data(line_directive, templates::license, templates::license_lines);
}

/* filename macros */
size_t data::filename_macros(bool line_directive)
{
    return save_data(line_directive, templates::filename_macros, templates::filename_macros_lines);
}

/* create template data */
void data::create_template_lists(vtemplate_t &header, vtemplate_t &body, output::format format, bool separate)
{
    auto concat_sources = [&] (const template_t *t_header, const template_t *t_body)
    {
        header.push_back(templates::common_header);
        header.push_back(t_header);

        if (separate) {
            body.push_back(t_body);
        } else {
            header.push_back(t_body);
        }
    };

    switch (format)
    {
    case output::c:
        concat_sources(templates::c_header, templates::c_body);
        break;
    case output::cxx:
        concat_sources(templates::cxx_header, templates::cxx_body);
        break;
    case output::plugin:
        concat_sources(templates::plugin_header, templates::plugin_body);
        break;
    case output::minimal:
        header.push_back(templates::min_c_header);
        break;
    case output::minimal_cxx:
        header.push_back(templates::min_cxx_header);
        break;
    [[unlikely]] case output::error:
        throw gendlopen::error(std::string(__func__) + ": format == output::error");
    }
}


#define OUTDIR "templates"

/* dump templates */
void data::dump_templates()
{
    if (!fs::create_directory(OUTDIR)) {
        int errsav = errno;
        std::string msg = "failed to create directory `" OUTDIR "': ";
        throw gendlopen::error(msg + strerror(errsav));
    }

#define SAVE(x) save_to_file(OUTDIR "/" FILENAME_##x, templates::x)

    SAVE(license);
    SAVE(filename_macros);
    SAVE(common_header);
    SAVE(c_header);
    SAVE(c_body);
    SAVE(cxx_header);
    SAVE(cxx_body);
    SAVE(plugin_header);
    SAVE(plugin_body);
    SAVE(min_c_header);
    SAVE(min_cxx_header);
}
