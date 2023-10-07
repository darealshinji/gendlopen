/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2023 djcj@gmx.de

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
*/

#include <string>
#include <vector>
#include <string.h>

#ifdef _MSC_VER
#define strcasecmp _stricmp
#else
#include <strings.h>
#endif

#include "gendlopen.hpp"
#include "template.h"


static const char *function_keywords[] =
{
    "GDO_RET",
    "GDO_TYPE",
    "GDO_SYMBOL",
    "GDO_ARGS",
    "GDO_NOTYPE_ARGS",
    NULL
};

static const char *object_keywords[] =
{
    "GDO_OBJ_TYPE",
    "GDO_OBJ_SYMBOL",
    NULL
};


/* check if the line needs to be processed in a loop */
static inline bool need_loop(const std::string &line, const char **list)
{
    for (const char **p = list; *p != NULL; p++) {
        if (line.find(*p) != std::string::npos) {
            return true;
        }
    }

    return false;
}

static inline std::string get_indent(const std::string &line)
{
    auto pos = line.find_first_not_of(" \t\n\r\v\f");

    if (pos != std::string::npos) {
        return line.substr(0, pos);
    }

    return {};
}

static inline std::string get_indent_end(const std::string &line)
{
    auto pos = line.find_last_not_of(" \t\n\r\v\f");

    if (pos != std::string::npos) {
        return line.substr(pos+1);
    }

    return {};
}

/* parse the template data */
std::string gendlopen::parse(const char *data)
{
    std::string buf, line;

    if (!data) {
        return {};
    }

    if (m_prototypes.empty() && m_objects.empty()) {
        std::cerr << "error: no function or object prototypes" << std::endl;
        std::exit(1);
    }

    for (const char *p = data; *p != 0; p++)
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
        if (line.find("GDO_") == std::string::npos) {
            buf += line;
            line.clear();
            continue;
        }

        /* replace typedefs and common header data first */
        if (line == "GDO_TYPEDEFS\n" && m_typedefs.empty()) {
            /* skip entire line */
            line.clear();
            continue;
        } else {
            replace_string("GDO_COMMON", template_common_header_data, line);
            replace_string("GDO_TYPEDEFS", m_typedefs, line);
        }

        /* replace if needed */

        bool has_func = need_loop(line, function_keywords);
        bool has_obj = need_loop(line, object_keywords);

        if (has_func && has_obj) {
            /* error */
            std::cerr << "error: cannot use function and object prototypes in the same line" << std::endl;
            std::exit(1);
        } else if (has_func) {
            /* function prototypes */

            if (m_prototypes.empty()) {
                buf += get_indent(line);
                buf += "/* -- no function prototypes -- */";
                buf += get_indent_end(line);
                line.clear();
                continue;
            }

            for (const auto &p : m_prototypes) {
                auto copy = line;

                /* don't "return" on "void" functions */
                if (strcasecmp(p.type.c_str(), "void") == 0) {
                    /* keep the indentation pretty */
                    replace_string("GDO_RET ", "", copy);
                    replace_string("GDO_RET", "", copy);
                } else {
                    replace_string("GDO_RET", "return", copy);
                }

                replace_string("GDO_TYPE", p.type, copy);
                replace_string("GDO_SYMBOL", p.symbol, copy);
                replace_string("GDO_ARGS", p.args, copy);
                replace_string("GDO_NOTYPE_ARGS", p.notype_args, copy);

                buf += copy;
            }
        } else if (has_obj) {
            /* object prototypes */

            if (m_objects.empty()) {
                buf += get_indent(line);
                buf += "/* -- no object prototypes -- */";
                buf += get_indent_end(line);
                line.clear();
                continue;
            }

            for (const auto &p : m_objects) {
                auto copy = line;
                replace_string("GDO_OBJ_TYPE", p.type, copy);
                replace_string("GDO_OBJ_SYMBOL", p.symbol, copy);
                buf += copy;
            }
        } else {
            /* nothing to loop, just append */
            buf += line;
        }

        line.clear();
    }

    /* replace the rest if a different prefix was set */
    if (m_name_upper != "GDO_") {
        replace_string("GDO_", m_name_upper, buf);
        replace_string("gdo_", m_name_lower, buf);

        /* C++ namespace */
        if (m_cxx) {
            std::string s = "namespace " + m_name_lower;
            s.pop_back(); /* remove trailing underscore */
            replace_string("namespace gdo", s, buf);

            s = m_name_lower;
            s.pop_back(); /* remove trailing underscore */
            s += "::";
            replace_string("gdo::", s, buf);
        }
    }

    buf += '\n';

    return buf;
}
