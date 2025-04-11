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
int gendlopen::replace_function_prototypes(const int &templ_lineno, const std::string &entry)
{
    auto erase_string = [] (const std::string &token, std::string &s)
    {
        size_t pos = 0;
        const size_t len = token.size();

        while ((pos = s.find(token, pos)) != std::string::npos) {
            s.erase(pos, len);
        }
    };

    size_t longest = 0;
    int line_count = 0;
    const int entry_lines = utils::count_linefeed(entry);

    /* print #line directive to make sure the line count is on par */
    if (m_prototypes.empty()) {
        if (m_line_directive) {
            save::ofs << "#line " << (templ_lineno + entry_lines + 1) << '\n';
        }
        return entry_lines;
    }

    if (entry.find("%%func_symbol_pad%%") != std::string::npos) {
        longest = get_longest_symbol_size(m_prototypes);
    }

    for (auto &e : m_prototypes) {
        if (utils::ends_with(e.args, "...") && /* VA_ARGS */
            entry.find("%%return%%") != std::string::npos) /* wrapper function */
        {
            /* we can't handle variable argument lists in wrapper functions */
            continue;
        }

        std::string copy = entry;

        /* don't "return" on "void" functions */
        if (strcasecmp(e.type.c_str(), "void") == 0) {
            /* keep the indentation pretty */
            erase_string("%%return%% ", copy);
            erase_string("%%return%%", copy);
        } else {
            utils::replace("%%return%%", "return", copy);
        }

        if (utils::ends_with(e.type, '*')) {
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
            save::ofs << "#line " << templ_lineno << '\n';
            line_count++;
        }

        save::ofs << copy << '\n';
        line_count += entry_lines + 1;
    }

    return line_count;
}

/* loop and replace object prototypes */
int gendlopen::replace_object_prototypes(const int &templ_lineno, const std::string &entry)
{
    int line_count = 0;
    const int entry_lines = utils::count_linefeed(entry);
    size_t longest = 0;

    /* print #line directive to make sure the line count is on par */
    if (m_objects.empty()) {
        if (m_line_directive) {
            save::ofs << "#line " << (templ_lineno + entry_lines + 1) << '\n';
        }
        return entry_lines;
    }

    if (entry.find("%%obj_symbol_pad%%") != std::string::npos) {
        longest = get_longest_symbol_size(m_objects);
    }

    for (auto &e : m_objects) {
        std::string copy = entry;

        if (utils::ends_with(e.type, '*')) {
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
            save::ofs << "#line " << templ_lineno << '\n';
            line_count++;
        }

        save::ofs << copy << '\n';
        line_count += entry_lines + 1;
    }

    return line_count;
}

/* loop and replace any symbol names */
int gendlopen::replace_symbol_names(const int &templ_lineno, const std::string &entry)
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
            save::ofs << "#line " << templ_lineno << '\n';
            line_count++;
        }

        save::ofs << copy << '\n';
        line_count += entry_lines + 1;
    };

    /* print #line directive to make sure the line count is on par */
    if (m_prototypes.empty() && m_objects.empty()) {
        if (m_line_directive) {
            save::ofs << "#line " << (templ_lineno + entry_lines + 1) << '\n';
        }
        return entry_lines;
    }

    /* function pointer */
    for (auto &e : m_prototypes) {
        /* %%sym_type%% --> »%%type%% (*)(%%args%%)« */
        if (utils::ends_with(e.type, '*')) {
            type = e.type + "(*)(" + e.args + ")";
        } else {
            type = e.type + " (*)(" + e.args + ")";
        }

        replace_and_print(e.symbol);
    }

    /* object pointer */
    for (auto &e : m_objects) {
        /* %%sym_type%% --> »%%obj_type%% *« */
        if (utils::ends_with(e.type, '*')) {
            type = e.type + '*';
        } else {
            type = e.type + " *";
        }

        replace_and_print(e.symbol);
    }

    return line_count;
}

/* substitute placeholders in a single line/entry */
int gendlopen::substitute_line(const template_t &line, int &templ_lineno, bool &param_skip_code)
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
            save::ofs << "#line " << (templ_lineno + entry_lines) << '\n';
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
            save::ofs << '\n';
            return 1; /* 1 line */
        }
        return 0; /* no line */
    }

    /* check if we have to comment out lines between
     * "%PARAM_SKIP_*_BEGIN%" and "%PARAM_SKIP_END%" */
    if (line.maybe_keyword && check_skip_keyword(line.data, param_skip_code, m_parameter_names)) {
        if (!param_skip_code && m_line_directive) {
            /* +1 to compensate for the removed %PARAM_SKIP_* line */
            save::ofs << "#line " << (templ_lineno + 1) << '\n';
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
        const std::regex reg_pfxupper("(" NOTALNUM "?[_]?)(GDO_)");
        const std::regex reg_pfxlower("(" NOTALNUM "?[_]?)(gdo_)");
        const std::regex reg_standalone("(" NOTALNUM "?)(gdo)(" NOTALNUM "?)");

        buf = std::regex_replace(buf, reg_pfxupper, m_fmt_upper);
        buf = std::regex_replace(buf, reg_pfxlower, m_fmt_lower);
        buf = std::regex_replace(buf, reg_standalone, m_fmt_standalone);
    }

    /* nothing to loop, just append */
    if (!line.maybe_keyword) {
        save::ofs << buf << '\n';
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
            return replace_function_prototypes(templ_lineno, buf);
        }
    } else if (has_obj == 1) {
        /* object prototypes */
        if (m_objects.empty()) {
            return print_lineno();
        } else {
            return replace_object_prototypes(templ_lineno, buf);
        }
    } else if (has_sym == 1) {
        /* any symbol */
        if (m_prototypes.empty() && m_objects.empty()) {
            return print_lineno();
        } else {
            return replace_symbol_names(templ_lineno, buf);
        }
    }

    /* nothing to loop, just append and return */
    save::ofs << buf << '\n';

    return utils::count_linefeed(buf) + 1;
}

/* substitute placeholders */
int gendlopen::substitute(const vtemplate_t &data)
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
        int templ_lineno = 0; /* input template line count */
        int i = 0;

        /* skip initial #line directive */
        if (!m_line_directive && strncmp(list[0].data, "#line", 5) == 0) {
            i++;
        }

        for ( ; list[i].data != NULL; i++) {
            total_lines += substitute_line(list[i], templ_lineno, param_skip_code);
            templ_lineno += list[i].line_count;
        }
    }

    return total_lines;
}
