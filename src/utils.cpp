/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Carsten Janssen
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

#include <filesystem>
#include <string>
#include <stdio.h>

#include "gendlopen.hpp"


namespace /* anonymous */
{
    /* quote library name */
    std::string quote_lib(const std::string &lib, bool wide)
    {
        if (wide) {
            if (lib.starts_with("L\"") && lib.back() == '"') {
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
}


/* case-insensitive string comparison (ignoring current locale) */
bool utils::eq_str_case(const std::string &str1, const std::string &str2)
{
    auto to_C_lower = [] (char c) -> char {
        if (range(c, 'A', 'Z')) {
            c += 32;  /* A-Z -> a-z */
        }
        return c;
    };

    /* different size means not equal */
    if (str1.size() != str2.size()) {
        return false;
    }

    auto it1 = str1.cbegin();
    auto it2 = str2.cbegin();

    for ( ; it1 != str1.cend(); it1++, it2++) {
        if (to_C_lower(*it1) != to_C_lower(*it2)) {
            return false;
        }
    }

    return true;
}

/* convert string to uppercase */
std::string utils::convert_to_upper(const std::string &str, bool underscores)
{
    std::string out;

    for (const char &c : str) {
        if (range(c, 'a','z')) {
            out += c - 32;
        } else if (!underscores || range(c, 'A','Z') || range(c, '0','9')) {
            out += c;
        } else {
            out += '_';
        }
    }

    return out;
}

/* convert string to lowercase */
std::string utils::convert_to_lower(const std::string &str, bool underscores)
{
    std::string out;

    for (const char &c : str) {
        if (range(c, 'A','Z')) {
            out += c + 32;
        } else if (!underscores || range(c, 'a','z') || range(c, '0','9')) {
            out += c;
        } else {
            out += '_';
        }
    }

    return out;
}

/* create "#define" lines */
std::string utils::format_def(std::string def)
{
    std::string name, value, out;
    const size_t pos = def.find('=');

    if (pos == std::string::npos) {
        name = def;
    } else {
        name = def.substr(0, pos);
        value = ' ' + def.substr(pos + 1);
    }

    strip_spaces(name);

    if (name.empty()) {
        /* empty string will be "appended" to code */
        return "";
    }

    out  = "#ifndef "  + name + '\n';
    out += "# define " + name + value + '\n';
    out += "#endif\n";

    return out;
}

/**
    * format library name
    * foo        ==>  "foo"
    * nq:foo     ==>  foo
    * ext:foo    ==>  "foo" LIBEXTA
    * api:2:foo  ==>  LIBNAMEA(foo,2)
    */
void utils::format_libname(const std::string &str, std::string &lib_a, std::string &lib_w)
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

/* quote header name if needed */
std::string utils::format_inc(const std::string &inc)
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

/* format */
output::format utils::format_enum(const char *in)
{
    std::string s = convert_to_lower(in, false);
    output::format out = output::error;

    if (s.front() == 'c') {
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
    }

    return out;
}
