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
 * Substitute placeholders in embedded template data.
 */

#include <iostream>
#include <list>
#include <random>
#include <regex>
#include <vector>
#include <cstdlib>

#include "global.hpp"


namespace /* anonymous */
{
    enum {
        NO_PARAM_SKIP_FOUND,
        PARAM_SKIP_REMOVE_BEGIN,
        PARAM_SKIP_USE_BEGIN,
        PARAM_SKIP_END
    };

    namespace fmt {
        std::string upper;
        std::string lower;
        std::string cxx_namespace;
    }

    /* check for a "%PARAM_SKIP_*%" line */
    int check_skip_keyword(const char *ptr)
    {
        const char str[] = "%PARAM_SKIP_";
        const size_t len = sizeof(str) - 1;

        if (strncmp(ptr, str, len) != 0) {
            return NO_PARAM_SKIP_FOUND;
        }

        ptr += len;

        if (strcmp(ptr, "REMOVE_BEGIN%") == 0) {
            return PARAM_SKIP_REMOVE_BEGIN;
        } else if (strcmp(ptr, "USE_BEGIN%") == 0) {
            return PARAM_SKIP_USE_BEGIN;
        } else if (strcmp(ptr, "END%") == 0) {
            return PARAM_SKIP_END;
        }

        return NO_PARAM_SKIP_FOUND;
    }

    /* return only the leading newlines from string */
    std::string get_leading_newlines(const std::string &in)
    {
        size_t i = 0;

        while (i < in.size()) {
            if (in.at(i) == '\n') {
                /* Unix newline */
                i++;
            } else if (in.compare(i, 2, "\r\n") == 0) {
                /* Windows newline */
                i += 2;
            } else {
                break;
            }
        }

        return in.substr(0, i);
    }

    /* return only the trailing newlines from string */
    const char *get_trailing_newlines(const std::string &in)
    {
        auto it = in.rbegin();

        while (it != in.rend()) {
            if (*it != '\n') {
                break;
            }

            auto next = it + 1;

            if (next != in.rend() && *next == '\r') {
                /* Windows newline */
                it += 2;
            } else {
                /* Unix newline */
                it++;
            }
        }

        return in.c_str() + (in.size() - std::distance(in.rbegin(), it));
    }

    /* loop and replace function prototypes, save to output stream */
    void replace_function_prototypes(const vproto_t &vec, const std::string &line, cio::ofstream &ofs)
    {
        auto erase_token = [] (const std::string &token, std::string &s)
        {
            size_t pos = 0;
            const size_t len = token.size();

            while ((pos = s.find(token, pos)) != std::string::npos) {
                s.erase(pos, len);
            }
        };

        std::string nl_beg;        /* copy of leading newlines */
        const char *nl_end = NULL; /* pointer to begin of trailing newlines */

        for (auto &e : vec) {
            /* we can't handle variable argument lists in wrapper functions */
            if (e.args.ends_with("...") && line.find("%%return%%") != std::string::npos) {
                if (!nl_end) {
                    /* do this only once */
                    nl_beg = get_leading_newlines(line);
                    nl_end = get_trailing_newlines(line);
                }

                /* print trailing and leading newlines from the template line */
                ofs << nl_beg;
                ofs << "/* can't handle variable argument lists in wrapper functions */\n";
                ofs << "// " << e.type << ' ' << e.symbol << '(' << e.args << ");\n";
                ofs << nl_end;
                continue;
            }

            std::string copy = line;

            /* don't "return" on "void" functions */
            if (utils::eq_str_case(e.type, "void")) {
                /* keep the indentation pretty */
                erase_token("%%return%% ", copy);
                erase_token("%%return%%", copy);
            } else {
                utils::replace("%%return%%", "return", copy);
            }

            if (e.type.back() == '*') {
                /* »char * x«  -->  »char *x« */
                utils::replace("%%type%% ", e.type, copy);
            }
            utils::replace("%%type%%", e.type, copy);
            utils::replace("%%func_symbol%%", e.symbol, copy);
            utils::replace("%%args%%", e.args, copy);
            utils::replace("%%notype_args%%", e.notype_args, copy);

            ofs << copy << '\n';
        }
    }

    /* loop and replace object prototypes */
    void replace_object_prototypes(const vproto_t &vec, const std::string &line, cio::ofstream &ofs)
    {
        for (auto &e : vec) {
            std::string copy = line;

            if (e.type.back() == '*') {
                /* »char * x«  -->  »char *x« */
                utils::replace("%%obj_type%% ", e.type, copy);
            }
            utils::replace("%%obj_type%%", e.type, copy);
            utils::replace("%%obj_symbol%%", e.symbol, copy);

            ofs << copy << '\n';
        }
    }

    /* loop and replace any symbol names */
    void replace_symbol_names(const vproto_t &proto, const vproto_t &obj, const std::string &line, cio::ofstream &ofs)
    {
        std::string copy, type;

        /* function pointer */
        for (auto &e : proto) {
            copy = line;

            /* %%sym_type%% --> »%%type%% (*)(%%args%%)« */
            type = e.type + " (*)(" + e.args + ")";
            utils::replace("%%sym_type%%", type, copy);
            utils::replace("%%symbol%%", e.symbol, copy);
            ofs << copy << '\n';
        }

        /* object pointer */
        for (auto &e : obj) {
            copy = line;

            /* %%sym_type%% --> »%%obj_type%% *« */
            type = e.type + " *";
            utils::replace("%%sym_type%%", type, copy);
            utils::replace("%%symbol%%", e.symbol, copy);
            ofs << copy << '\n';
        }
    }

} /* end anonymous namespace */


/* substitute placeholders in a single line */
void gendlopen::substitute_line(const char *line, bool &skip_code, cio::ofstream &ofs)
{
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

    /* regex for prefixes: [_]GDO_ / [_]gdo_ / gdo:: */
    const std::regex reg_upper("([^A-Za-z0-9_]?[_]?)(GDO_)");
    const std::regex reg_lower("([^A-Za-z0-9_]?[_]?)(gdo_)");
    const std::regex reg_namespace("([^A-Za-z0-9_]?)(gdo::)");

    /* empty line */
    if (line[0] == 0) {
        if (!skip_code) {
            ofs << '\n';
        }
        return;
    }

    /* optimize search for potential keywords */
    bool maybe_keyword = (strchr(line, '%') != NULL);

    if (maybe_keyword) {
        /* skip the whole line if it has the %DNL% (Do Not Lex) keyword */
        if (strstr(line, "%DNL%") != NULL) {
            return;
        }

        /* check if we have to comment out lines between
        * "%PARAM_SKIP_*_BEGIN%" and "%PARAM_SKIP_END%" */
        switch (check_skip_keyword(line))
        {
        case PARAM_SKIP_REMOVE_BEGIN:
            skip_code = (m_parameter_names == param::skip);
            return;
        case PARAM_SKIP_USE_BEGIN:
            skip_code = (m_parameter_names != param::skip);
            return;
        case PARAM_SKIP_END:
            skip_code = false;
            return;
        default:
            break;
        }
    }

    /* skip line */
    if (skip_code) {
        return;
    }

    std::string buf = line;

    /* replace prefixes */
    if (m_name_upper != "GDO") {
        //m_fmt_upper = "$1" + m_name_upper + '_';
        //m_fmt_lower = "$1" + m_name_lower + '_';
        //m_fmt_namespace = "$1" + m_name_lower + "::";
        buf = std::regex_replace(buf, reg_upper, m_fmt_upper);
        buf = std::regex_replace(buf, reg_lower, m_fmt_lower);
        buf = std::regex_replace(buf, reg_namespace, m_fmt_namespace);
    }

    if (maybe_keyword) {
        /* insert common symbol prefix string */
        utils::replace("%COMMON_PREFIX%", m_common_prefix, buf);

        /* update value */
        maybe_keyword = (buf.find('%') != std::string::npos);
    }

    /* nothing to loop, just append */
    if (!maybe_keyword) {
        ofs << buf << '\n';
        return;
    }

    /* check if the line needs to be processed in a loop */
    auto find_keyword = [&buf] (const list_t &list) -> int
    {
        for (const auto &e : list) {
            if (buf.find(e) != std::string::npos) {
                return 1;
            }
        }
        return 0;
    };

    int has_func, has_obj, has_sym;

    if (buf.find("%%") == std::string::npos) {
        has_func = has_obj = has_sym = 0;
    } else {
        /* check for keywords that require a loop */
        has_func = find_keyword(function_keywords);
        has_obj = find_keyword(object_keywords);
        has_sym = find_keyword(symbol_keywords);
    }

    if ((has_func + has_obj + has_sym) > 1) {
        /* error */
        throw error("cannot mix function, object and regular symbol"
                    " placeholders:\n" + std::string{line});
    } else if (has_func == 1) {
        /* function prototypes */
        replace_function_prototypes(m_prototypes, buf, ofs);
    } else if (has_obj == 1) {
        /* object prototypes */
        replace_object_prototypes(m_objects, buf, ofs);
    } else if (has_sym == 1) {
        /* any symbol */
        replace_symbol_names(m_prototypes, m_objects, buf, ofs);
    } else {
        /* nothing to loop, just append */
        ofs << buf << '\n';
    }
}

/* substitute placeholders */
void gendlopen::substitute(const cstrList_t &data, cio::ofstream &ofs)
{
    bool skip_code = false;

    if (data.empty()) {
        return;
    }

    if (m_prototypes.empty() && m_objects.empty()) {
        throw error("no function or object prototypes");
    }

    for (const auto &list : data) {
        const char *line = list[0];

        /* go through char** list */
        for (int i = 0; line != NULL; i++, line = list[i]) {
            substitute_line(line, skip_code, ofs);
        }
    }
}
