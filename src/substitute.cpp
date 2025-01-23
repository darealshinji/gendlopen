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

/**
 * Substitute placeholders in embedded template data.
 */

#ifdef _MSC_VER
# include "strcasecmp.hpp"
#else
# include <strings.h>
#endif
#include <string.h>
#include <iterator>
#include <list>
#include <ostream>
#include <regex>
#include <string>
#include <vector>
#include "cio_ofstream.hpp"
#include "gendlopen.hpp"
#include "types.hpp"
#include "utils.hpp"



namespace /* anonymous */
{
    namespace fmt {
        std::string upper;
        std::string lower;
        std::string cxx_namespace;
    }

    /* check for a "%PARAM_SKIP_*%" line */
    bool check_skip_keyword(const char *ptr, bool &param_skip_code, param::names parameter_names)
    {
        const char str[] = "%PARAM_SKIP_";
        const size_t len = sizeof(str) - 1;

        if (strncmp(ptr, str, len) != 0) {
            return false;
        }

        ptr += len;

        if (strcmp(ptr, "REMOVE_BEGIN%") == 0) {
            /* PARAM_SKIP_REMOVE_BEGIN */
            param_skip_code = (parameter_names == param::skip);
            return true;
        } else if (strcmp(ptr, "USE_BEGIN%") == 0) {
            /* PARAM_SKIP_USE_BEGIN */
            param_skip_code = (parameter_names != param::skip);
            return true;
        } else if (strcmp(ptr, "END%") == 0) {
            /* PARAM_SKIP_END */
            param_skip_code = false;
            return true;
        }

        return false;
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

    /* get the length of the longest symbol */
    size_t get_longest_symbol_size(const vproto_t &v)
    {
        auto comp = [] (const proto_t &a, const proto_t &b) {
            return (a.symbol.size() < b.symbol.size());
        };

        auto it = std::max_element(v.begin(), v.end(), comp);

        return (it == v.end()) ? 0 : (*it).symbol.size();
    }

} /* end anonymous namespace */


/* loop and replace function prototypes, save to output stream */
int gendlopen::replace_function_prototypes(const int &templ_lineno, const std::string &entry, cio::ofstream &ofs)
{
    auto erase_string = [] (const std::string &token, std::string &s)
    {
        size_t pos = 0;
        const size_t len = token.size();

        while ((pos = s.find(token, pos)) != std::string::npos) {
            s.erase(pos, len);
        }
    };

    std::string nl_beg;        /* copy of leading newlines */
    const char *nl_end = NULL; /* pointer to begin of trailing newlines */

    size_t longest = 0;

    int line_count = 0;
    int nl_beg_count = 0;
    int nl_end_count = 0;
    const int entry_lines = utils::count_linefeed(entry);

    /* print #line directive to make sure the line count is on par */
    if (m_prototypes.empty()) {
        if (m_line_directive) {
            ofs << "#line " << (templ_lineno + entry_lines + 1) << '\n';
        }
        return entry_lines;
    }

    if (entry.find("%%func_symbol_pad%%") != std::string::npos) {
        longest = get_longest_symbol_size(m_prototypes);
    }

    for (auto &e : m_prototypes) {
        if (e.args.ends_with("...") && /* VA_ARGS */
            entry.find("%%return%%") != std::string::npos) /* wrapper function */
        {
            /* we can't handle variable argument lists in wrapper functions */
            if (!nl_end) {
                /* do this only once */
                nl_beg = get_leading_newlines(entry);
                nl_end = get_trailing_newlines(entry);
                nl_beg_count = utils::count_linefeed(nl_beg);
                nl_end_count = utils::count_linefeed(nl_end);
            }

            const char *space = (e.type.back() == '*') ? "" : " ";

            /* print trailing and leading newlines from the template line */
            ofs << nl_beg;
            ofs << "/* can't handle variable argument lists in wrapper functions */\n";
            ofs << "// " << e.type << space << e.symbol << '(' << e.args << ");\n";
            ofs << nl_end;

            line_count += nl_beg_count + 2 + nl_end_count;
        } else {
            /* regular entry */
            std::string copy = entry;

            /* don't "return" on "void" functions */
            if (strcasecmp(e.type.c_str(), "void") == 0) {
                /* keep the indentation pretty */
                erase_string("%%return%% ", copy);
                erase_string("%%return%%", copy);
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

            /* symbol name with padding */
            if (longest > 0) {
                std::string s = e.symbol;
                s.append(longest - e.symbol.size(), ' ');
                utils::replace("%%func_symbol_pad%%", s, copy);
            }

            if (m_line_directive) {
                ofs << "#line " << templ_lineno << '\n';
                line_count++;
            }

            ofs << copy << '\n';
            line_count += entry_lines + 1;
        }
    }

    return line_count;
}

/* loop and replace object prototypes */
int gendlopen::replace_object_prototypes(const int &templ_lineno, const std::string &entry, cio::ofstream &ofs)
{
    int line_count = 0;
    const int entry_lines = utils::count_linefeed(entry);
    size_t longest = 0;

    /* print #line directive to make sure the line count is on par */
    if (m_objects.empty()) {
        if (m_line_directive) {
            ofs << "#line " << (templ_lineno + entry_lines + 1) << '\n';
        }
        return entry_lines;
    }

    if (entry.find("%%obj_symbol_pad%%") != std::string::npos) {
        longest = get_longest_symbol_size(m_objects);
    }

    for (auto &e : m_objects) {
        std::string copy = entry;

        if (e.type.back() == '*') {
            /* »char * x«  -->  »char *x« */
            utils::replace("%%obj_type%% ", e.type, copy);
        }
        utils::replace("%%obj_type%%", e.type, copy);
        utils::replace("%%obj_symbol%%", e.symbol, copy);

        /* symbol name with padding */
        if (longest > 0) {
            std::string s = e.symbol;
            s.append(longest - e.symbol.size(), ' ');
            utils::replace("%%obj_symbol_pad%%", s, copy);
        }

        if (m_line_directive) {
            ofs << "#line " << templ_lineno << '\n';
            line_count++;
        }

        ofs << copy << '\n';
        line_count += entry_lines + 1;
    }

    return line_count;
}

/* loop and replace any symbol names */
int gendlopen::replace_symbol_names(const int &templ_lineno, const std::string &entry, cio::ofstream &ofs)
{
    std::string type;
    int line_count = 0;
    const int entry_lines = utils::count_linefeed(entry);

    auto replace_and_print = [&, this] (const std::string &symbol)
    {
        std::string copy = entry;
        utils::replace("%%sym_type%%", type, copy);
        utils::replace("%%symbol%%", symbol, copy);

        if (m_line_directive) {
            ofs << "#line " << templ_lineno << '\n';
            line_count++;
        }

        ofs << copy << '\n';
        line_count += entry_lines + 1;
    };

    /* print #line directive to make sure the line count is on par */
    if (m_prototypes.empty() && m_objects.empty()) {
        if (m_line_directive) {
            ofs << "#line " << (templ_lineno + entry_lines + 1) << '\n';
        }
        return entry_lines;
    }

    /* function pointer */
    for (auto &e : m_prototypes) {
        /* %%sym_type%% --> »%%type%% (*)(%%args%%)« */
        if (e.type.back() == '*') {
            type = e.type + "(*)(" + e.args + ")";
        } else {
            type = e.type + " (*)(" + e.args + ")";
        }

        replace_and_print(e.symbol);
    }

    /* object pointer */
    for (auto &e : m_objects) {
        /* %%sym_type%% --> »%%obj_type%% *« */
        if (e.type.back() == '*') {
            type = e.type + '*';
        } else {
            type = e.type + " *";
        }

        replace_and_print(e.symbol);
    }

    return line_count;
}

/* substitute placeholders in a single line/entry */
int gendlopen::substitute_line(const template_t &line, int &templ_lineno, bool &param_skip_code, cio::ofstream &ofs)
{
    const list_t function_keywords = {
        "%%return%%",
        "%%type%%",
        "%%func_symbol%%",
        "%%func_symbol_pad%%",
        "%%args%%",
        "%%notype_args%%"
    };

    const list_t object_keywords = {
        "%%obj_type%%",
        "%%obj_symbol%%",
        "%%obj_symbol_pad%%"
    };

    const list_t symbol_keywords = {
        "%%sym_type%%",
        "%%symbol%%"
    };

    std::string buf;
    int has_func = 0;
    int has_obj = 0;
    int has_sym = 0;

    /* print #line directive to make sure the line count is on par */
    auto print_lineno = [&, this] () -> int
    {
        const int entry_lines = utils::count_linefeed(buf);

        if (m_line_directive) {
            ofs << "#line " << (templ_lineno + entry_lines + 1) << '\n';
        }

        return entry_lines;
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
        return 0; /* no line */
    }

    /* check if we have to comment out lines between
     * "%PARAM_SKIP_*_BEGIN%" and "%PARAM_SKIP_END%" */
    if (line.maybe_keyword && check_skip_keyword(line.data, param_skip_code, m_parameter_names)) {
        if (!param_skip_code && m_line_directive) {
            /* +1 to compensate for the removed %PARAM_SKIP_* line */
            ofs << "#line " << (templ_lineno + 1) << '\n';
            return 1; /* 1 line */
        }
        return 0; /* no line */
    }

    /* skip line */
    if (param_skip_code) {
        return 0;
    }

    buf = line.data;

#define NOTALNUM "[^a-zA-Z0-9_]"

    /* replace prefixes */
    if (m_pfx_upper != "GDO") {
        /* regex for prefixes: [_]GDO_ / [_]gdo_ / gdo:: */
        const std::regex reg_upper("(" NOTALNUM "?[_]?)(GDO_)");
        const std::regex reg_lower("(" NOTALNUM "?[_]?)(gdo_)");
        const std::regex reg_namespace("(" NOTALNUM "?)(gdo::)");

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

    if ((has_func + has_obj + has_sym) > 1) {
        /* error */
        throw error("cannot mix function, object and regular symbol"
                    " placeholders:\n" + std::string{line.data});
    } else if (has_func == 1) {
        /* function prototypes */
        if (m_prototypes.empty()) {
            return print_lineno();
        } else {
            return replace_function_prototypes(templ_lineno, buf, ofs);
        }
    } else if (has_obj == 1) {
        /* object prototypes */
        if (m_objects.empty()) {
            return print_lineno();
        } else {
            return replace_object_prototypes(templ_lineno, buf, ofs);
        }
    } else if (has_sym == 1) {
        /* any symbol */
        if (m_prototypes.empty() && m_objects.empty()) {
            return print_lineno();
        } else {
            return replace_symbol_names(templ_lineno, buf, ofs);
        }
    }

    /* nothing to loop, just append and return */
    ofs << buf << '\n';

    return utils::count_linefeed(buf) + 1;
}

/* substitute placeholders */
int gendlopen::substitute(const vtemplate_t &data, cio::ofstream &ofs)
{
    int total_lines = 0;
    bool param_skip_code = false;

    if (data.empty()) {
        return 0;
    }

    if (m_prototypes.empty() && m_objects.empty()) {
        throw error("no function or object prototypes");
    }

    for (const auto &list : data) {
        int templ_lineno = 1; /* input template line count */
        int i = 0;

        /* skip initial #line directive */
        if (!m_line_directive && strncmp(list[0].data, "#line", 5) == 0) {
            i++;
        }

        for ( ; list[i].data != NULL; i++) {
            total_lines += substitute_line(list[i], templ_lineno, param_skip_code, ofs);
            templ_lineno += list[i].line_count;
        }
    }

    return total_lines;
}
