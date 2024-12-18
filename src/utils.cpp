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
#include <ctype.h>
#include <stdio.h>

#include "global.hpp"


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
}


/* convert string to uppercase */
std::string utils::convert_to_upper(const std::string &str, bool underscores)
{
    std::string out;

    if (underscores) {
        for (const char &c : str) {
            if (isalnum(c)) {
                out += static_cast<char>(toupper(c));
            } else {
                out += '_';
            }
        }
    } else {
        for (const char &c : str) {
            out += static_cast<char>(toupper(c));
        }
    }

    return out;
}

/* convert string to lowercase */
std::string utils::convert_to_lower(const std::string &str, bool underscores)
{
    std::string out;

    if (underscores) {
        for (const char &c : str) {
            if (isalnum(c)) {
                out += static_cast<char>(tolower(c));
            } else {
                out += '_';
            }
        }
    } else {
        for (const char &c : str) {
            out += static_cast<char>(tolower(c));
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

/* returns true if s begins with a prefix found in list */
bool utils::is_prefixed(const std::string &s, const vstring_t &list)
{
    for (const auto &e : list) {
        if (s.starts_with(e)) {
            return true;
        }
    }
    return false;
}

/* delete suffix from string */
void utils::delete_suffix(std::string &str, const std::string &suffix)
{
    if (str.ends_with(suffix)) {
        str.erase(str.size() - suffix.size());
    }
}

void utils::delete_suffix(std::string &str, const char suffix)
{
    if (str.back() == suffix) {
        str.pop_back();
    }
}

/* strip ANSI white-space characters from front and back */
void utils::strip_spaces(std::string &s)
{
    /* remove from back */
    while (!s.empty() && isspace(s.back())) {
        s.pop_back();
    }

    /* remove from front */
    while (!s.empty() && isspace(s.front())) {
        s.erase(0, 1);
    }
}

/* replace string "from" with string "to" in string "s" */
void utils::replace(const std::string &from, const std::string &to, std::string &s)
{
    size_t pos = 0;
    const size_t len = from.size();

    for (; (pos = s.find(from, pos)) != std::string::npos; pos += to.size()) {
        s.replace(pos, len, to);
    }
}

/* count '\n' characters */
int utils::count_linefeed(const std::string &str)
{
    int n = 0;

    for (auto &c : str) {
        if (c == '\n') {
            n++;
        }
    }

    return n;
}
