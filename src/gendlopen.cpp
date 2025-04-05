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
#include "utils.hpp"


namespace /* anonymous */
{
    /* case-insensitive comparison if string begins with prefix (and is longer than prefix) */
    bool prefixed_case_len(const std::string &str, const char *pfx, const size_t &pfxlen)
    {
        if (str.size() > pfxlen) {
            auto tmp = str.substr(0, pfxlen);
            return (strcasecmp(tmp.c_str(), pfx) == 0);
        }
        return false;
    }

    template<size_t N>
    constexpr bool prefixed_case(const std::string &str, char const (&pfx)[N])
    {
        return prefixed_case_len(str, pfx, N-1);
    }

    /* quote header name if needed */
    std::string format_inc(const std::string &inc)
    {
        if (prefixed_case(inc, "nq:")) {
            /* no quotes */
            return inc.substr(3);
        }

        if ((inc.front() == '<' && inc.back() == '>') ||
            (inc.front() == '"' && inc.back() == '"'))
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
            if (utils::starts_with(lib, "L\"") && lib.back() == '"') {
                /* already quoted */
                return lib;
            } else if (lib.front() == '"' && lib.back() == '"') {
                /* prepend 'L' */
                return 'L' + lib;
            }

            return "L\"" + lib + '"';
        }

        if (lib.front() == '"' && lib.back() == '"') {
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
        switch(str.at(0))
        {
        case 'N':
        case 'n':
            /* no quotes */
            if (prefixed_case(str, "nq:")) {
                lib_a = lib_w = str.substr(3);
                return;
            }
            break;

        case 'E':
        case 'e':
            /* quotes + file extension macro */
            if (prefixed_case(str, "ext:")) {
                auto sub = str.substr(4);
                lib_a = quote_lib(sub, false) + " LIBEXTA";
                lib_w = quote_lib(sub, true) + " LIBEXTW";
                return;
            }
            break;

        case 'A':
        case 'a':
            /* no quotes, API libname macro */
            if (prefixed_case(str, "api:")) {
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

    /* 'void (*)()'  ==>  'void (*name)()' */
    std::string fptr_typedef(const std::string &type, const std::string &name)
    {
        auto pos = type.find("(*)");

        if (pos == std::string::npos || type.find('(') != pos) {
            return {};
        }

        std::string s = type;
        pos += 2; /* insert after '*' */

        return s.insert(pos, name);
    }

    /* 'char[32]'  ==>  'char name[32]' */
    std::string array_typedef(std::string &type, const std::string &name)
    {
        auto pos = type.find('[');

        if (pos == std::string::npos) {
            return {};
        }

        std::string s = type;

        /* insert space if needed */
        if (pos > 0 && type.at(pos-1) != ' ') {
            s.insert(pos, 1, ' ');
            pos++;
        }

        return s.insert(pos, name);
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

    if (s.front() == 'c') {
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


/* create typedefs for function pointers and arrays */
void gendlopen::create_typedefs()
{
    /* create typename */
    auto mk_name = [this] (const std::string &symbol) {
        return m_pfx_lower + '_' + symbol + "_t";
    };

    for (auto &p : m_objects) {
        std::string def, name;

        if (p.prototype == proto::function_pointer) {
            /* function pointer */
            name = mk_name(p.symbol);
            def = fptr_typedef(p.type, name);
        } else if (p.prototype == proto::object_array) {
            /* array type */
            name = mk_name(p.symbol);
            def = array_typedef(p.type, name);
        } else {
            continue;
        }

        if (!def.empty()) {
            m_typedefs.push_back(def); /* add to typedefs */
            p.type = name; /* replace old type */
        }
    }
}
