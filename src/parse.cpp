/**
 * The MIT License (MIT)
 *
 * Copyright (C) 2023-2024 djcj@gmx.de
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

/**
 * Parse the embedded template data and substitute the placeholders.
 */

#include <iostream>
#include <string>
#include <stdlib.h>

#include "template.h"
#include "common.hpp"
#include "gendlopen.hpp"

using common::replace_string;
using common::same_string_case;


/*
static inline
std::string get_indent(const std::string &line)
{
    auto pos = line.find_first_not_of(" \t\n\r\v\f");

    if (pos != std::string::npos) {
        return line.substr(0, pos);
    }

    return {};
}

static inline
std::string get_indent_end(const std::string &line)
{
    auto pos = line.find_last_not_of(" \t\n\r\v\f");

    if (pos != std::string::npos) {
        return line.substr(pos + 1);
    }

    return {};
}
*/

/* check for keyword in list */
static inline
bool find_keyword(const std::string &line, const char* const *list)
{
    for (auto p = list; *p != NULL; p++) {
        if (line.find(*p) != std::string::npos) {
            return true;
        }
    }

    return false;
}

/* parse the template data */
std::string gendlopen::parse(const std::string &data, vproto_t &prototypes, vobj_t &objects)
{
    std::string buf, line;
    bool custom_prefix = false;

    const char* const function_keywords[] = {
        "GDO_RET",
        "GDO_TYPE",
        "GDO_SYMBOL",
        "GDO_ARGS",
        "GDO_NOTYPE_ARGS",
        NULL
    };

    const char* const object_keywords[] = {
        "GDO_OBJ_TYPE",
        "GDO_OBJ_SYMBOL",
        NULL
    };

    if (data.empty()) {
        return {};
    }

    if (prototypes.empty() && objects.empty()) {
        std::cerr << "error: no function or object prototypes" << std::endl;
        std::exit(1);
    }

    if (m_name_upper != "GDO") {
        custom_prefix = true;
    }

    for (const char *p = data.c_str(); *p != 0; p++)
    {
        const char next = *(p+1);

        /* treat lines ending on '@' like single lines */
        if (*p == '@' && next == '\n') {
            line += '\n';
            p++;
            continue;
        }

        /* add character to line */
        line += *p;

        /* end of line not yet reached */
        if (*p != '\n' && next != 0) {
            continue;
        }

        /* nothing to replace */
        if (line.find("GDO") == std::string::npos &&
            (custom_prefix && line.find("gdo") == std::string::npos))
        {
            buf += line;
            line.clear();
            continue;
        }

        /* check if the line needs to be processed in a loop */
        bool has_func = find_keyword(line, function_keywords);
        bool has_obj = find_keyword(line, object_keywords);

        if (has_func && has_obj) {
            /* error */
            std::cerr << "error: cannot use function and object prototypes in the same line" << std::endl;
            std::exit(1);
        } else if (has_func) {
            /* function prototypes */

            if (prototypes.empty()) {
                //buf += get_indent(line);
                //buf += "/* -- no function prototypes -- */";
                //buf += get_indent_end(line);
                line.clear();
                continue;
            }

            for (const auto &e : prototypes) {
                auto copy = line;

                /* don't "return" on "void" functions */
                if (same_string_case(e.type, "void")) {
                    /* keep the indentation pretty */
                    replace_string("GDO_RET ", "", copy);
                    replace_string("GDO_RET", "", copy);
                } else {
                    replace_string("GDO_RET", "return", copy);
                }

                if (e.type.ends_with("*")) {
                    replace_string("GDO_TYPE ", e.type, copy);
                }
                replace_string("GDO_TYPE", e.type, copy);
                replace_string("GDO_SYMBOL", e.symbol, copy);
                replace_string("GDO_ARGS", e.args, copy);
                replace_string("GDO_NOTYPE_ARGS", e.notype_args, copy);

                buf += copy;
            }
        } else if (has_obj) {
            /* object prototypes */

            if (objects.empty()) {
                //buf += get_indent(line);
                //buf += "/* -- no object prototypes -- */";
                //buf += get_indent_end(line);
                line.clear();
                continue;
            }

            for (const auto &e : objects) {
                auto copy = line;
                replace_string("GDO_OBJ_TYPE", e.type, copy);
                replace_string("GDO_OBJ_SYMBOL", e.symbol, copy);
                buf += copy;
            }
        } else {
            /* nothing to loop, just append */
            buf += line;
        }

        line.clear();
    }

    /* replace the rest */
    if (custom_prefix) {
        replace_string("GDO", m_name_upper, buf);
        replace_string("gdo", m_name_lower, buf);
    }

    return buf;
}
