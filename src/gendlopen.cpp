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

#include <stdio.h>
#include <iostream>
#include <regex>
#include "gendlopen.hpp"
#include "cio_ofstream.hpp"
#include "open_file.hpp"
#include "utils.hpp"


namespace /* anonymous */
{
    /* quote header name if needed */
    std::string format_inc(const std::string &inc)
    {
        if (utils::prefixed_and_longer_case(inc, "nq:")) {
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


    /* read input lines */
    bool get_lines(FILE *fp, std::string &line, template_t &entry)
    {
        bool loop = true;
        int c = EOF;

        line.clear();
        entry.maybe_keyword = false;
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
                entry.maybe_keyword = true;
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
        std::cout << "/* typedefs */\n";

        for (const auto &e : m_typedefs) {
            std::cout << "typedef " << e << ";\n";
        }
        std::cout << '\n';
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


/* replace prefixes in string */
std::string gendlopen::replace_prefixes(const char *data)
{
    if (!data) {
        return "";
    }

#define NOTALNUM "[^a-zA-Z0-9_]"

    const std::regex reg_pfxupper("(" NOTALNUM "?[_]?)(GDO_)");
    const std::regex reg_pfxlower("(" NOTALNUM "?[_]?)(gdo_)");
    const std::regex reg_standalone("(" NOTALNUM "?)(gdo)(" NOTALNUM "?)");

#undef NOTALNUM

    std::string buf = data;
    buf = std::regex_replace(buf, reg_pfxupper, m_fmt_upper);
    buf = std::regex_replace(buf, reg_pfxlower, m_fmt_lower);
    buf = std::regex_replace(buf, reg_standalone, m_fmt_standalone);

    return buf;
}


/* read and process custom template */
void gendlopen::process_custom_template()
{
    std::string buf;
    template_t entry;
    bool param_skip_code = false;
    bool eof = false;
    size_t templ_lineno = 1; /* input template line count */

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


/* parse input and generate output */
void gendlopen::process()
{
    /* parse input */
    tokenize();

    if (print_symbols()) {
        /* print symbols */
        print_symbols_to_stdout();
    } else if (print_lookup()) {
        /* print symbol lookup macro */
        save::symbol_name_lookup(m_pfx_upper, m_prototypes, m_objects);
    } else if (!custom_template().empty()) {
        /* use a custom template */
        process_custom_template();
    } else {
        /* default */
        generate();
    }
}
