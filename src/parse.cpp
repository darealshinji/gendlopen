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
#include <random>
#include <regex>
#include <cstdlib>
#include "gendlopen.hpp"

#define SKIP_BEGIN 0
#define SKIP_END 1


namespace /* anonymous */
{
    using list_t = std::list<const char *>;

    /* check for keywords from a list */
    int find_keyword(const std::string &line, const list_t &list)
    {
        if (line.find("%%") == std::string::npos) {
            return 0;
        }

        for (const auto &e : list) {
            if (line.find(e) != std::string::npos) {
                return 1;
            }
        }

        return 0;
    }

    /* check for a "%SKIP_BEGIN%" or "%SKIP_END%" line and
     * return either SKIP_BEGIN, SKIP_END or -1 if nothing was found */
    int check_skip_keyword(const std::string &line)
    {
        const std::regex reg(R"(^\s*?%SKIP_(BEGIN|END)%\s*?$)");
        std::smatch m;

        if (!std::regex_match(line, m, reg) || m.size() != 2) {
            return -1;
        }

        if (m[1] == "BEGIN") {
            return SKIP_BEGIN;
        } else if (m[1] == "END") {
            return SKIP_END;
        }
        return -1;
    }

    /* loop and replace function prototypes, append to buffer */
    void replace_function_prototypes(vproto_t &vec, const std::string &line, std::string &buffer)
    {
        std::string copy;

        for (const auto &e : vec) {
            copy = line;

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
            utils::replace("%%func_symbol%%", e.symbol, copy);
            utils::replace("%%args%%", e.args, copy);
            utils::replace("%%notype_args%%", e.notype_args, copy);

            buffer += copy;
        }
    }

    void replace_symbol_names(vproto_t &proto, vproto_t &obj, const std::string &line, std::string &buffer)
    {
        std::string copy, type;

        for (const auto &e : proto) {
            copy = line;

            /* (%%sym_type%%) == (%%type%% (*)(%%args%%)) */
            type = e.type + " (*)(" + e.args + ")";
            utils::replace("%%sym_type%%", type, copy);
            utils::replace("%%symbol%%", e.symbol, copy);
            buffer += copy;
        }

        for (const auto &e : obj) {
            copy = line;

            /* (%%sym_type%%) == (%%obj_type%% *) */
            type = e.type + " *";
            utils::replace("%%sym_type%%", type, copy);
            utils::replace("%%symbol%%", e.symbol, copy);
            buffer += copy;
        }
    }
}

/* parse the template data */
std::string gendlopen::parse(std::string &data)
{
    std::string buf, line;
    std::string fmt_upper, fmt_lower, fmt_namespace;
    bool comment_out = false;

    const list_t function_keywords = {
        "%%return%%",
        "%%type%%",
        "%%func_symbol%%",
        "%%args%%",
        "%%notype_args%%"
    };

    const list_t object_keywords = {
        "%%obj_type%%",
        "%%obj_symbol%%"
    };

    const list_t symbol_keywords = {
        "%%sym_type%%",
        "%%symbol%%"
    };

    if (data.empty()) {
        return {};
    }

    if (m_prototypes.empty() && m_objects.empty()) {
        std::cerr << "error: no function or object prototypes" << std::endl;
        std::exit(1);
    }

    /* regex for prefixes: [_]GDO_* / [_]gdo_* / gdo:: */
    const std::regex reg_upper("([^A-Za-z0-9_]?[_]?)(GDO_)([A-Za-z0-9_])");
    const std::regex reg_lower("([^A-Za-z0-9_]?[_]?)(gdo_)([A-Za-z0-9_])");
    const std::regex reg_nmspc("([^A-Za-z0-9_]?)(gdo::)");

    const bool custom_prefix = (m_name_upper != "GDO");

    if (custom_prefix) {
        fmt_upper = "$1" + (m_name_upper + "_$3");
        fmt_lower = "$1" + (m_name_lower + "_$3");
        fmt_namespace = "$1" + (m_name_lower + "::");
    }

    /* lambda function to replace prefixes in lines */
    auto replace_prefixes = [&] (const std::string &in) -> std::string
    {
        if (custom_prefix) {
            std::string s;
            s = std::regex_replace(in, reg_upper, fmt_upper);
            s = std::regex_replace(s, reg_lower, fmt_lower);
            s = std::regex_replace(s, reg_nmspc, fmt_namespace);
            return s;
        }
        return in;
    };

    /* Change all line endings from \r\n to \n.
     * This is a "radical" solution but it makes parsing the code
     * a lot easier. */
    utils::replace("\r\n", "\n", data);

    /* read data character by character */
    for (const char *p = data.c_str(); *p != 0; p++)
    {
        /* will be NUL if EOL is reached */
        const char next = *(p+1);

        /* treat lines ending on '@' like single lines
         * but keep the '\n' character */
        if (*p == '@' && next == '\n') {
            p++;

            if (comment_out) {
                /* keep trailing @ symbol in commented out section */
                line += '@';
            } else {
                line += '\n';
                continue;
            }
        }

        line += *p;

        /* end of line or data not yet reached */
        if (*p != '\n' && next != 0) {
            continue;
        }

        /* skip the whole line if it has the %DNL% (Do Not Lex) keyword */
        if (line.find("%DNL%") != std::string::npos) {
            line.clear();
            continue;
        }

        /* check if we have to comment out lines
         * between "%SKIP_BEGIN%" and "%SKIP_END%" */

        int skip = check_skip_keyword(line);

        if (skip == SKIP_BEGIN) {
            /* set as "true" if "--skip-parameter-names" was given,
             * otherwise set as false and nothing is commented out */
            comment_out = m_skip_parameter_names;
            line.clear();
            continue;
        } else if (skip == SKIP_END) {
            comment_out = false;
            line.clear();
            continue;
        }

        if (comment_out) {
            buf += "//" + replace_prefixes(line);
            line.clear();
            continue;
        }

        /* check if the line needs to be processed in a loop */
        int has_func = find_keyword(line, function_keywords);
        int has_obj = find_keyword(line, object_keywords);
        int has_sym = find_keyword(line, symbol_keywords);

        if ((has_func + has_obj + has_sym) > 1) {
            /* error */
            std::cerr << "error: cannot mix function, object and regular symbol placeholders:\n";
            std::cerr << line << std::endl;
            std::exit(1);
        } else if (has_func == 1) {
            /* function prototypes */

            if (m_prototypes.empty()) {
                line.clear();
                continue;
            }

            line = replace_prefixes(line);
            replace_function_prototypes(m_prototypes, line, buf);
        } else if (has_obj == 1) {
            /* object prototypes */

            if (m_objects.empty()) {
                line.clear();
                continue;
            }

            line = replace_prefixes(line);

            for (const auto &e : m_objects) {
                std::string copy = line;
                utils::replace("%%obj_type%%", e.type, copy);
                utils::replace("%%obj_symbol%%", e.symbol, copy);
                buf += copy;
            }
        } else if (has_sym == 1) {
            /* any symbol */
            line = replace_prefixes(line);
            replace_symbol_names(m_prototypes, m_objects, line, buf);
        } else {
            /* nothing to loop, just append */
            buf += replace_prefixes(line);
        }

        line.clear();
    }

    return buf;
}
