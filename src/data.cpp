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

#include "global.hpp"
#include "template.h"


namespace /* anonymous */
{
    constexpr int save_data(cio::ofstream &ofs, bool line_directive, const template_t *list)
    {
        int total_lines = 0;
        int i = 0;

        if (!line_directive && strncmp(list[0].data, "#line", 5) == 0) {
            /* skip initial line directive */
            i++;
        }

        for ( ; list[i].data != NULL; i++) {
            ofs << list[i].data << '\n';
            total_lines += list[i].line_count;
        }

        return total_lines;
    }
}

/* filename macros */
int gendlopen::save_filename_macros_data(cio::ofstream &ofs) {
    return save_data(ofs, m_line_directive, filename_macros);
}

/* license */
int gendlopen::save_license_data(cio::ofstream &ofs) {
    return save_data(ofs, m_line_directive, license);
}

/* create template data */
void gendlopen::create_template_data_lists(vtemplate_t &header, vtemplate_t &body)
{
    switch (m_format)
    {
    [[unlikely]] default:
        [[fallthrough]];

    case output::c:
        {
            header.push_back(common_header);
            header.push_back(c_header);

            if (m_separate) {
                body.push_back(c_body);
            } else {
                header.push_back(c_body);
            }
        }
        break;

    case output::cxx:
        {
            header.push_back(common_header);
            header.push_back(cxx_header);

            if (m_separate) {
                body.push_back(cxx_body);
            } else {
                header.push_back(cxx_body);
            }
        }
        break;

    case output::minimal:
        header.push_back(min_c_header);
        break;

    case output::minimal_cxx:
        header.push_back(min_cxx_header);
        break;
    }
}
