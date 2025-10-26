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

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <regex>
#include "gendlopen.hpp"
#include "cio_ofstream.hpp"
#include "open_file.hpp"
#include "utils.hpp"


namespace /* anonymous */
{

/* print all found symbols to stdout */
void print_symbols_to_stdout(vstring_t vtypedefs, vproto_t vprototypes, vproto_t vobjects)
{
    cio::ofstream out; /* defaults to STDOUT */

    if (!vtypedefs.empty()) {
        std::cout << "/* typedefs */\n";

        for (const auto &e : vtypedefs) {
            std::cout << "typedef " << e << ";\n";
        }
        std::cout << '\n';
    }

    std::cout << "/* prototypes */\n";

    auto print_type = [] (const std::string &s)
    {
        if (s.ends_with('*')) {
            std::cout << s;
        } else {
            std::cout << s << ' ';
        }
    };

    for (const auto &e : vobjects) {
        print_type(e.type);
        std::cout << e.symbol << ";\n";
    }

    for (const auto &e : vprototypes) {
        print_type(e.type);
        std::cout << e.symbol << '(' << e.args << ");\n";
    }

    std::cout << "\n/***  " << (vobjects.size() + vprototypes.size()) << " matches  ***/" << std::endl;
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
    m_pfx_upper = utils::to_upper(m_pfx);
    m_pfx_lower = utils::to_lower(m_pfx);

    /* set regex format string (used in substitute.cpp) */
    m_fmt_upper = "$1" + m_pfx_upper + '_';
    m_fmt_lower = "$1" + m_pfx_lower + '_';
    m_fmt_standalone = "$1" + m_pfx_lower + "$3";
}


/* add "#include" line */
void gendlopen::add_inc(const std::string &inc)
{
    if (utils::prefixed_and_longer_case(inc, "nq:")) {
        /* no quotes */
        m_includes.push_back(inc.substr(3));
    } else if (inc.size() >= 2 &&
        (utils::front_and_back(inc, '<', '>') || utils::front_and_back(inc, '"', '"')))
    {
        /* already quoted */
        m_includes.push_back(inc);
    } else {
        /* add quotes */
        m_includes.push_back('"' + inc + '"');
    }
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
    std::string s = utils::to_lower(in, false);

    if (s.starts_with('c')) {
        if (s == "c") {
            out = output::c;
        } else if (s == "cxx" || s == "c++" || s == "cpp") {
            out = output::cxx;
        }
    } else if (s.starts_with("minimal")) {
        s.erase(0, 7);

        if (s.empty() || s == "-c") {
            out = output::minimal;
        } else if (s == "-cxx" || s == "-c++" || s == "-cpp") {
            out = output::minimal_cxx;
        }
    } else if (s.starts_with("plugin")) {
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


/* replace prefixes in string */
std::string gendlopen::replace_prefixes(const std::string &input)
{
    const std::regex reg_pfxupper("(([^a-zA-Z0-9_]|^)[_]?)(GDO_)");
    const std::regex reg_pfxlower("(([^a-zA-Z0-9_]|^)[_]?)(gdo_)");
    const std::regex reg_standalone("([^a-zA-Z0-9_]|^)(gdo)([^a-zA-Z0-9_]|$)");

    std::string buf = input;
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

    /* open file for reading */
    open_file file(m_custom_template);

    if (!file.is_open()) {
        throw error("failed to open file for reading: " + m_custom_template);
    }

    /* create output file */
    save::open_ofstream(m_output, m_force);
    FILE *fp = file.file_pointer();

    /* write initial #line directive */
    if (m_line_directive) {
        if (fp == stdin) {
            save::ofs << "#line 1 \"<STDIN>\"\n";
        } else {
            save::ofs << "#line 1 \"" << m_custom_template << "\"\n";
        }

        /* count the "#line" directive */
        m_substitute_lineno = 1;
    }

    /* parse lines */
    while (!eof) {
        eof = utils::get_lines(fp, buf, entry);
        substitute_line(entry, param_skip_code);

        if (m_line_directive) {
            m_substitute_lineno += entry.line_count;
        }
    }
}


/* parse input and generate output */
void gendlopen::process(const int &argc, char ** const &argv)
{
    get_templates_path_env();

    parse_cmdline(argc, argv);
    tokenize();

    if (print_symbols()) {
        print_symbols_to_stdout(m_typedefs, m_prototypes, m_objects);
    } else if (!custom_template().empty()) {
        process_custom_template();
    } else {
        generate(); /* default */
    }
}
