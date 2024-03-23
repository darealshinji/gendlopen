/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2023-2024 Carsten Janssen
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
#include <list>
#include <string>
#include <stdlib.h>

#include "gendlopen.hpp"


namespace /* anonymous */
{
    using list_t = std::list<const char *>;

    /* check for keyword in list */
    bool find_keyword(const std::string &line, const list_t &list)
    {
        if (line.find("%%") == std::string::npos) {
            return false;
        }

        for (const auto &e : list) {
            if (line.find(e) != std::string::npos) {
                return true;
            }
        }

        return false;
    }
}

/* parse the template data */
std::string gendlopen::parse(const std::string &data)
{
    std::string buf, line;
    bool custom_prefix = false;
    bool comment_out = false;

    const list_t function_keywords = {
        "%%return%%",
        "%%type%%",
        "%%symbol%%",
        "%%args%%",
        "%%notype_args%%"
    };

    const list_t object_keywords = {
        "%%obj_type%%",
        "%%obj_symbol%%"
    };

    if (data.empty()) {
        return {};
    }

    if (m_prototypes.empty() && m_objects.empty()) {
        std::cerr << "error: no function or object prototypes" << std::endl;
        std::exit(1);
    }

    if (m_name_upper != "GDO") {
        custom_prefix = true;
    }

    /* read data character by character */
    for (const char *p = data.c_str(); *p != 0; p++)
    {
        /* will be NUL if EOL was reached */
        const char next = *(p+1);

        /* treat lines ending on '@' like single lines */
        if (*p == '@' && next == '\n') {
            p++;

            if (comment_out) {
                /* keep trailing @ symbol in the comments */
                line += '@';
            } else {
                /* append newline and continue reading */
                line += '\n';
                continue;
            }
        }

        /* add character to line */
        line += *p;

        /* end of line not yet reached */
        if (*p != '\n' && next != 0) {
            continue;
        }

        /* whether to comment out parts of code */
        if (line == "%SKIP_BEGIN%\n") {
            comment_out = m_skip_parameter_names;
            line.clear();
            continue;
        } else if (line == "%SKIP_END%\n") {
            comment_out = false;
            line.clear();
            continue;
        }

        if (comment_out) {
            buf += "//" + line;
            line.clear();
            continue;
        }

        /* check if the line needs to be processed in a loop */
        bool has_func = find_keyword(line, function_keywords);
        bool has_obj = find_keyword(line, object_keywords);

        if (has_func && has_obj) {
            /* error */
            std::cerr << "error: cannot use function and object prototypes in the same line(s):\n";
            std::cerr << line << std::endl;
            std::exit(1);
        } else if (has_func) {
            /* function prototypes */

            if (m_prototypes.empty()) {
                line.clear();
                continue;
            }

            for (const auto &e : m_prototypes) {
                std::string copy = line;

                /* don't "return" on "void" functions */
                if (utils::eq_str_case(e.type, "void")) {
                    /* keep the indentation pretty */
                    utils::replace("%%return%% ", "", copy);
                    utils::replace("%%return%%", "", copy);
                } else {
                    utils::replace("%%return%%", "return", copy);
                }

                if (e.type.ends_with("*")) {
                    /* append space */
                    utils::replace("%%type%% ", e.type, copy);
                }
                utils::replace("%%type%%", e.type, copy);
                utils::replace("%%symbol%%", e.symbol, copy);
                utils::replace("%%args%%", e.args, copy);
                utils::replace("%%notype_args%%", e.notype_args, copy);

                buf += copy;
            }
        } else if (has_obj) {
            /* object prototypes */

            if (m_objects.empty()) {
                line.clear();
                continue;
            }

            for (const auto &e : m_objects) {
                std::string copy = line;
                utils::replace("%%obj_type%%", e.type, copy);
                utils::replace("%%obj_symbol%%", e.symbol, copy);
                buf += copy;
            }
        } else {
            /* nothing to loop, just append */
            buf += line;
        }

        line.clear();
    }

    /* replace the prefixes */
    if (custom_prefix) {
        utils::replace("GDO", m_name_upper, buf);
        utils::replace("gdo", m_name_lower, buf);
    }

    return buf;
}
