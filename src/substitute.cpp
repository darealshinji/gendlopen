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

} /* end anonymous namespace */


/* loop and replace function prototypes, save to output stream */
int gendlopen::replace_function_prototypes(const int &line_number, const std::string &line, cio::ofstream &ofs)
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

    int line_count = 0;
    const int intputline_count = utils::count_linefeed(line);
    int nl_beg_count = 0;
    int nl_end_count = 0;

    for (auto &e : m_prototypes) {
        if (e.args.ends_with("...") && line.find("%%return%%") != std::string::npos) {
            /* we can't handle variable argument lists in wrapper functions */
            if (!nl_end) {
                /* do this only once */
                nl_beg = get_leading_newlines(line);
                nl_end = get_trailing_newlines(line);
                nl_beg_count = utils::count_linefeed(nl_beg);
                nl_end_count = utils::count_linefeed(nl_end);
            }

            /* print trailing and leading newlines from the template line */
            ofs << nl_beg;
            ofs << "/* can't handle variable argument lists in wrapper functions */\n";
            ofs << "// " << e.type << ' ' << e.symbol << '(' << e.args << ");\n";
            ofs << nl_end;
            line_count += nl_beg_count + 2 + nl_end_count;
        } else {
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

            if (m_line_directive) {
                ofs << "#line " << line_number << '\n';
                line_count++;
            }

            ofs << copy << '\n';
            line_count += intputline_count + 1;
        }
    }

    return line_count;
}

/* loop and replace object prototypes */
int gendlopen::replace_object_prototypes(const int &line_number, const std::string &line, cio::ofstream &ofs)
{
    int line_count = 0;
    const int intputline_count = utils::count_linefeed(line);

    for (auto &e : m_objects) {
        std::string copy = line;

        if (e.type.back() == '*') {
            /* »char * x«  -->  »char *x« */
            utils::replace("%%obj_type%% ", e.type, copy);
        }
        utils::replace("%%obj_type%%", e.type, copy);
        utils::replace("%%obj_symbol%%", e.symbol, copy);

        if (m_line_directive) {
            ofs << "#line " << line_number << '\n';
            line_count++;
        }

        ofs << copy << '\n';
        line_count += intputline_count + 1;
    }

    return line_count;
}

/* loop and replace any symbol names */
int gendlopen::replace_symbol_names(const int &line_number, const std::string &line, cio::ofstream &ofs)
{
    std::string type;
    int line_count = 0;
    const int intputline_count = utils::count_linefeed(line);

    auto replace_and_print = [&] (const std::string &symbol, bool line_directive)
    {
        std::string copy = line;
        utils::replace("%%sym_type%%", type, copy);
        utils::replace("%%symbol%%", symbol, copy);

        if (line_directive) {
            ofs << "#line " << line_number << '\n';
            line_count++;
        }

        ofs << copy << '\n';
        line_count += intputline_count + 1;
    };

    /* function pointer */
    for (auto &e : m_prototypes) {
        /* %%sym_type%% --> »%%type%% (*)(%%args%%)« */
        if (e.type.back() == '*') {
            type = e.type + "(*)(" + e.args + ")";
        } else {
            type = e.type + " (*)(" + e.args + ")";
        }

        replace_and_print(e.symbol, m_line_directive);
    }

    /* object pointer */
    for (auto &e : m_objects) {
        /* %%sym_type%% --> »%%obj_type%% *« */
        if (e.type.back() == '*') {
            type = e.type + '*';
        } else {
            type = e.type + " *";
        }

        replace_and_print(e.symbol, m_line_directive);
    }

    return line_count;
}

/* substitute placeholders in a single line */
int gendlopen::substitute_line(const template_t &line, int &line_number, bool &param_skip_code, cio::ofstream &ofs)
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

    std::string buf;
    int has_func = 0;
    int has_obj = 0;
    int has_sym = 0;

    auto print_lineno = [&] (bool line_directive) -> int {
        if (!param_skip_code && line_directive) {
            /* +1 to compensate for the removed %PARAM_SKIP_* line */
            ofs << "#line " << (line_number + 1) << '\n';
            return 1; /* 1 line */
        }
        return 0; /* 0 lines */
    };

    auto find_keyword = [&buf] (const list_t &list) -> int {
        for (const auto &e : list) {
            if (buf.find(e) != std::string::npos) {
                return 1;
            }
        }
        return 0;
    };


    /* empty line */
    if (line.data[0] == 0) {
        if (!param_skip_code) {
            ofs << '\n';
            return 1; /* 1 line */
        }
        return 0; /* 0 lines */
    }

    if (line.maybe_keyword) {
        /* check if we have to comment out lines between
        * "%PARAM_SKIP_*_BEGIN%" and "%PARAM_SKIP_END%" */
        switch (check_skip_keyword(line.data))
        {
        case PARAM_SKIP_REMOVE_BEGIN:
            param_skip_code = (m_parameter_names == param::skip);
            return print_lineno(m_line_directive);

        case PARAM_SKIP_USE_BEGIN:
            param_skip_code = (m_parameter_names != param::skip);
            return print_lineno(m_line_directive);

        case PARAM_SKIP_END:
            param_skip_code = false;
            return print_lineno(m_line_directive);

        default:
            break;
        }
    }

    /* skip line */
    if (param_skip_code) {
        return 0;
    }

    buf = line.data;

    /* replace prefixes */
    if (m_pfx_upper != "GDO") {
        /* regex for prefixes: [_]GDO_ / [_]gdo_ / gdo:: */
        const std::regex reg_upper("([^A-Za-z0-9_]?[_]?)(GDO_)");
        const std::regex reg_lower("([^A-Za-z0-9_]?[_]?)(gdo_)");
        const std::regex reg_namespace("([^A-Za-z0-9_]?)(gdo::)");

        buf = std::regex_replace(buf, reg_upper, m_fmt_upper);
        buf = std::regex_replace(buf, reg_lower, m_fmt_lower);
        buf = std::regex_replace(buf, reg_namespace, m_fmt_namespace);
    }

    /* nothing to loop, just append */
    if (!line.maybe_keyword) {
        ofs << buf << '\n';
        return utils::count_linefeed(buf) + 1;
    }

    /* check if the line needs to be processed in a loop */
    if (buf.find("%%") != std::string::npos) {
        has_func = find_keyword(function_keywords);
        has_obj = find_keyword(object_keywords);
        has_sym = find_keyword(symbol_keywords);
    }

    int has_loop = has_func + has_obj + has_sym;

    if (has_loop > 1) {
        /* error */
        throw error("cannot mix function, object and regular symbol"
                    " placeholders:\n" + std::string{line.data});
    } else if (has_loop == 0) {
        /* nothing to loop, just append and return */
        ofs << buf << '\n';
        return utils::count_linefeed(buf) + 1;
    }

    if (has_func == 1) {
        /* function prototypes */
        return replace_function_prototypes(line_number, buf, ofs);
    } else if (has_obj == 1) {
        /* object prototypes */
        return replace_object_prototypes(line_number, buf, ofs);
    } else if (has_sym == 1) {
        /* any symbol */
        return replace_symbol_names(line_number, buf, ofs);
    }

    return 0;
}

/* substitute placeholders */
int gendlopen::substitute(const vtemplate_t &data, cio::ofstream &ofs)
{
    int lines = 0;
    bool param_skip_code = false;

    if (data.empty()) {
        return 0;
    }

    if (m_prototypes.empty() && m_objects.empty()) {
        throw error("no function or object prototypes");
    }

    for (const auto &list : data) {
        int line_number = 1;
        int i = 0;

        /* skip initial #line directive */
        if (!m_line_directive && strncmp(list[0].data, "#line", 5) == 0) {
            i++;
        }

        for ( ; list[i].data != NULL; i++) {
            lines += substitute_line(list[i], line_number, param_skip_code, ofs);
            line_number += list[i].line_count;
        }
    }

    return lines;
}
