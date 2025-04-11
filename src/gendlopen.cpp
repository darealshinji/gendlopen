/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2024-2025 Carsten Janssen

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

#ifdef _MSC_VER
# include "strcasecmp.hpp"
#else
# include <strings.h>
#endif
#include <algorithm>
#include <initializer_list>
#include <regex>
#include "gendlopen.hpp"
#include "cio_ofstream.hpp"
#include "open_file.hpp"
#include "utils.hpp"


namespace /* anonymous */
{
    /* case-insensitive comparison if string begins with prefix (and is longer than prefix) */
    bool prefixed_and_longer_case_len(const std::string &str, const char *pfx, const size_t &pfxlen)
    {
        if (str.size() > pfxlen) {
            auto tmp = str.substr(0, pfxlen);
            return (strcasecmp(tmp.c_str(), pfx) == 0);
        }
        return false;
    }

    template<size_t N>
    constexpr bool prefixed_and_longer_case(const std::string &str, char const (&pfx)[N])
    {
        return prefixed_and_longer_case_len(str, pfx, N-1);
    }

    /* quote header name if needed */
    std::string format_inc(const std::string &inc)
    {
        if (prefixed_and_longer_case(inc, "nq:")) {
            /* no quotes */
            return inc.substr(3);
        }

        if (inc.size() >= 2 &&
            (utils::starts_ends_with(inc, '<', '>') ||
             utils::starts_ends_with(inc, '"', '"')))
        {
            /* already quoted */
            return inc;
        }

        /* add quotes */
        return '"' + inc + '"';
    }

    /* quote library name */
    std::string quote_lib(const std::string &lib, bool wide)
    {
        if (wide) {
            if (lib.size() >= 3 && utils::starts_ends_with(lib, "L\"", '"')) {
                /* already quoted */
                return lib;
            } else if (lib.size() >= 2 && utils::starts_ends_with(lib, '"', '"')) {
                /* prepend 'L' */
                return 'L' + lib;
            }

            return "L\"" + lib + '"';
        }

        if (lib.size() >= 2 && utils::starts_ends_with(lib, '"', '"')) {
            /* already quoted */
            return lib;
        }

        return '"' + lib + '"';
    }

    /**
     * format library name
     * foo        ==>  "foo"
     * nq:foo     ==>  foo
     * ext:foo    ==>  "foo" LIBEXTA
     * api:2:foo  ==>  LIBNAMEA(foo,2)
     */
    void format_libname(const std::string &str, std::string &lib_a, std::string &lib_w)
    {
        if (str.empty()) {
            return;
        }

        switch(str.front())
        {
        case 'N':
        case 'n':
            /* no quotes */
            if (prefixed_and_longer_case(str, "nq:")) {
                lib_a = lib_w = str.substr(3);
                return;
            }
            break;

        case 'E':
        case 'e':
            /* quotes + file extension macro */
            if (prefixed_and_longer_case(str, "ext:")) {
                auto sub = str.substr(4);
                lib_a = quote_lib(sub, false) + " LIBEXTA";
                lib_w = quote_lib(sub, true) + " LIBEXTW";
                return;
            }
            break;

        case 'A':
        case 'a':
            /* no quotes, API libname macro */
            if (prefixed_and_longer_case(str, "api:")) {
                const std::regex reg("(.*?):(.*)");
                std::smatch m;
                auto sub = str.substr(4);

                if (std::regex_match(sub, m, reg) && m.size() == 3) {
                    /* LIBNAMEA(xxx,0) */
                    lib_w = lib_a = "LIBNAMEA(" + m[2].str() + ',' + m[1].str() + ')';
                    lib_w[7] = 'W';
                    return;
                }
            }
            break;

        default:
            break;
        }

        /* quote string */
        lib_a = quote_lib(str, false);
        lib_w = quote_lib(str, true);
    }

    /* read input lines */
    bool get_lines(FILE *fp, std::string &line, template_t &entry)
    {
        bool loop = true;
        int c = EOF;

        line.clear();
        entry.maybe_keyword = 0;
        entry.line_count = 1;

        while (loop)
        {
            c = fgetc(fp);

            switch (c)
            {
            case '\n':
                /* concatenate lines ending on '@' */
                if (utils::ends_with(line, '@')) {
                    line.back() = '\n';
                    entry.line_count++;
                    continue;
                }
                loop = false;
                break;

            case EOF:
                if (utils::ends_with(line, '@')) {
                    line.pop_back();
                }
                loop = false;
                break;

            case '%':
                entry.maybe_keyword = 1;
                [[fallthrough]];

            default:
                line.push_back(static_cast<char>(c));
                continue;
            }
        }

        entry.data = line.c_str();

        return (c == EOF);
    }

} /* end anonymous namespace */


/* c'tor */
gendlopen::gendlopen()
{}


/* d'tor */
gendlopen::~gendlopen()
{}


/* set symbol prefix name */
void gendlopen::prefix(const std::string &s)
{
    /* set name */
    m_pfx = s;

    /* set uppercase/lowercase name */
    m_pfx_upper = utils::convert_to_upper(m_pfx);
    m_pfx_lower = utils::convert_to_lower(m_pfx);

    /* set regex format string (used in substitute.cpp) */
    m_fmt_upper = "$1" + m_pfx_upper + '_';
    m_fmt_lower = "$1" + m_pfx_lower + '_';
    m_fmt_standalone = "$1" + m_pfx_lower + "$3";
}


/* set default library to load */
void gendlopen::default_lib(const std::string &lib)
{
    std::string lib_a, lib_w;
    format_libname(lib, lib_a, lib_w);
    m_deflib_a = lib_a;
    m_deflib_w = lib_w;
}


/* add "#include" line */
void gendlopen::add_inc(const std::string &inc)
{
    m_includes.push_back(format_inc(inc));
}


/* add "#define" line */
void gendlopen::add_def(const std::string &def)
{
    std::string name, value, out;
    const size_t pos = def.find('=');

    if (pos == std::string::npos) {
        name = def;
    } else {
        name = def.substr(0, pos);
        value = ' ' + def.substr(pos + 1);
    }

    utils::strip_spaces(name);

    if (!name.empty()) {
        m_defines += "#ifndef "  + name + '\n';
        m_defines += "# define " + name + value + '\n';
        m_defines += "#endif\n";
    }
}


/* set output format */
void gendlopen::format(const std::string &in)
{
    output::format out = output::error;
    std::string s = utils::convert_to_lower(in, false);

    if (utils::starts_with(s, 'c')) {
        if (s == "c") {
            out = output::c;
        } else if (s == "cxx" || s == "c++" || s == "cpp") {
            out = output::cxx;
        }
    } else if (utils::starts_with(s, "minimal")) {
        s.erase(0, 7);

        if (s.empty() || s == "-c") {
            out = output::minimal;
        } else if (s == "-cxx" || s == "-c++" || s == "-cpp") {
            out = output::minimal_cxx;
        }
    } else if (utils::starts_with(s, "plugin")) {
        s.erase(0, 6);

        if (s.empty() || s == "-c") {
            out = output::plugin;
        }
    }

    if (out == output::error) {
        throw error("unknown output format: " + in);
    }

    format(out);
}


/* print all found symbols to stdout */
void gendlopen::print_symbols_to_stdout()
{
    cio::ofstream out; /* defaults to STDOUT */

    if (!m_typedefs.empty()) {
        std::string str = "/* typedefs */\n";

        for (const auto &e : m_typedefs) {
            str += "typedef " + e + ";\n";
        }
        str += '\n';
    }

    std::cout << "/* prototypes */\n";

    auto print_type = [] (const std::string &s)
    {
        if (utils::ends_with(s, '*')) {
            std::cout << s;
        } else {
            std::cout << s << ' ';
        }
    };

    for (const auto &e : m_objects) {
        print_type(e.type);
        std::cout << e.symbol << ";\n";
    }

    for (const auto &e : m_prototypes) {
        print_type(e.type);
        std::cout << e.symbol << '(' << e.args << ");\n";
    }

    std::cout << "\n/***  " << (m_objects.size() + m_prototypes.size()) << " matches  ***/" << std::endl;
}


/* read and process custom template */
void gendlopen::process_custom_template()
{
    std::string buf;
    template_t entry;
    bool param_skip_code = false;
    bool eof = false;
    int templ_lineno = 1; /* input template line count */

    /* open file for reading */
    open_file file(m_custom_template);

    if (!file.is_open()) {
        throw error("failed to open file for reading: " + m_custom_template);
    }

    /* create output file */
    open_ofstream(m_output);
    FILE *fp = file.file_pointer();

    /* write initial #line directive */
    if (m_line_directive) {
        if (fp == stdin) {
            save::ofs << "#line 1 \"<STDIN>\"\n";
        } else {
            save::ofs << "#line 1 \"" << m_custom_template << "\"\n";
        }
    }

    /* parse lines */
    while (!eof) {
        eof = get_lines(fp, buf, entry);
        substitute_line(entry, templ_lineno, param_skip_code);
        templ_lineno += entry.line_count;
    }
}
