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
#include <string>
#include <vector>
#include "gendlopen.hpp"
#include "types.hpp"


namespace fs = std::filesystem;


namespace templates
{
#define TEMPLATE(x) \
    extern const template_t *ptr_##x;

#include "list.h"
}


/* create template data */
void data::create_template_lists(vtemplate_t &header, vtemplate_t &body, output::format format, bool separate)
{
    auto concat_sources = [&] (const template_t *t_header, const template_t *t_body)
    {
        load_template(templates::file_common_header);
        header.push_back(templates::ptr_common_header);
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
        load_template(templates::file_c_header);
        load_template(templates::file_c_body);
        concat_sources(templates::ptr_c_header, templates::ptr_c_body);
        break;

    case output::cxx:
        load_template(templates::file_cxx_header);
        load_template(templates::file_cxx_body);
        concat_sources(templates::ptr_cxx_header, templates::ptr_cxx_body);
        break;

    case output::plugin:
        load_template(templates::file_plugin_header);
        load_template(templates::file_plugin_body);
        concat_sources(templates::ptr_plugin_header, templates::ptr_plugin_body);
        break;

    case output::minimal:
        load_template(templates::file_min_c_header);
        header.push_back(templates::ptr_min_c_header);
        break;

    case output::minimal_cxx:
        load_template(templates::file_min_cxx_header);
        header.push_back(templates::ptr_min_cxx_header);
        break;

    [[unlikely]] case output::error:
        throw gendlopen::error(std::string(__func__) + ": format == output::error");
    }
}

