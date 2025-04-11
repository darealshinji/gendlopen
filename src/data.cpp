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

#include <string.h>
#include <ostream>
#include "cio_ofstream.hpp"
#include "gendlopen.hpp"
#include "types.hpp"

namespace templates {
#include "template.h"
}


namespace /* anonymous */
{
    constexpr int save_data(bool line_directive, const template_t *list)
    {
        int total_lines = 0;
        int i = 0;

        if (!line_directive && strncmp(list[0].data, "#line", 5) == 0) {
            /* skip initial line directive */
            i++;
        }

        for ( ; list[i].data != NULL; i++) {
            save::ofs << list[i].data << '\n';
            total_lines += list[i].line_count;
        }

        return total_lines;
    }
}

/* filename macros */
int data::filename_macros(bool line_directive) {
    return save_data(line_directive, templates::filename_macros);
}

/* license */
int data::license(bool line_directive) {
    return save_data(line_directive, templates::license);
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
