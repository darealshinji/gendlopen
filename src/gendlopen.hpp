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

#ifndef _GENDLOPEN_HPP_
#define _GENDLOPEN_HPP_

#include <fstream>
#include <string>
#include <utility>
#include <vector>
#include <cassert>
#include <cstring>
#include "cio.hpp"

/* check for __builtin_unreachable() */
#if defined(__has_builtin) && !defined(HAS_BUILTIN_UNREACHABLE)
    #if __has_builtin(__builtin_unreachable)
        #define HAS_BUILTIN_UNREACHABLE
    #endif
#endif

/* ANSI color codes used in the Clang AST output */

/* escaped variants for std::regex */
#define COL(x)    "\x1B\\[" #x "m"
#define C0        COL(0)          /* default */
#define CGREEN    COL(0;32)       /* green */
#define CFGREEN   COL(0;1;32)     /* fat green */
#define CFBLUE    COL(0;1;36)     /* fat blue */

/* unescaped variants for std::string */
#define sCOL(x)   "\x1B[" #x "m"
#define sC0       sCOL(0)         /* default */
#define sCORANGE  sCOL(0;33)      /* orange */
#define sCFGREEN  sCOL(0;1;32)    /* fat green */


/* typedefs */

typedef struct {
    std::string type;
    std::string symbol;
    std::string args;
    std::string notype_args;
} proto_t;

using vproto_t = std::vector<proto_t>;
using vstring_t = std::vector<std::string>;


/* enum for outout format */

namespace output
{
    typedef enum {
        c,
        cxx,
        minimal,
        minimal_cxx
    } format;
}


/* common functions */

namespace utils
{

/* case-insensitive string comparison */
bool eq_str_case(const std::string &str1, const std::string &str2);

/* returns true if s begins with a prefix found in list */
inline bool is_prefixed(const std::string &s, const vstring_t &list)
{
    for (const auto &e : list) {
        if (s.starts_with(e)) {
            return true;
        }
    }
    return false;
}

/* strip ANSI white-space characters from front and back */
inline void strip_spaces(std::string &s)
{
    const char *list = " \t\n\r\v\f";

    /* remove from back */
    while (!s.empty() && std::strchr(list, s.back())) {
        s.pop_back();
    }

    /* remove from front */
    while (!s.empty() && std::strchr(list, s.front())) {
        s.erase(0, 1);
    }
}

/* replace string "from" with string "to" in string "s" */
inline void replace(const std::string &from, const std::string &to, std::string &s)
{
    for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
    {
        s.replace(pos, from.size(), to);
    }
}

/* whether "c" is within the range of "beg" and "end" */
template<typename T=char>
inline bool range(T c, T beg, T end)
{
    assert(beg < end);
    return (c >= beg && c <= end);
}

/* in case std::unreachable is not implemented */
[[noreturn]] inline void unreachable()
{
#ifdef __cpp_lib_unreachable
    std::unreachable();
#elif defined(HAS_BUILTIN_UNREACHABLE)
    /* GCC, Clang */
    __builtin_unreachable();
#elif defined(_MSC_VER)
    /* MSVC */
    __assume(false);
#endif
}

} /* namespace utils end */


class gendlopen
{
private:

    inline bool separate_is_supported(const output::format fmt) const {
        return (fmt == output::c || fmt == output::cxx);
    }

    vstring_t m_definitions, m_includes, m_symbols, m_prefix;
    vproto_t m_prototypes, m_objects;

    std::string m_name_upper, m_name_lower;
    std::string m_default_liba, m_default_libw;
    std::string m_ifile;

    output::format m_format = output::c;

    bool m_force = false;
    bool m_separate = false;
    bool m_skip_parameter_names = false;
    bool m_ast_all_symbols = false;
    int *m_argc = NULL;
    char ***m_argv = NULL;

    /* clang-ast.cpp */
    bool clang_ast_line(cio::ifstream &ifs, std::string &line, int mode);
    bool clang_ast(cio::ifstream &ifs);

    /* tokenize.cpp */
    bool tokenize(cio::ifstream &ifs);
    void filter_and_copy_symbols(vproto_t &tmp_proto, vproto_t &tmp_objs);

    /* generate.cpp */
    bool tokenize_input();

    /* parse.cpp */
    std::string parse(std::string &data);

public:

    /* c'tor */
    gendlopen(int *argc, char ***argv) : m_argc(argc), m_argv(argv)
    {}

    /* d'tor */
    ~gendlopen()
    {}

    /* set options */
    void input(const std::string &s) { m_ifile = s; }
    void format(output::format val) { m_format = val; }
    void default_liba(const std::string &s) { m_default_liba = s; }
    void default_libw(const std::string &s) { m_default_libw = s; }
    void force(bool b) { m_force = b; }
    void separate(bool b) { m_separate = b; }
    void skip_parameter_names(bool b) { m_skip_parameter_names = b; }
    void ast_all_symbols(bool b) { m_ast_all_symbols = b; }

    /* add code */
    void add_def(const std::string &s) { m_definitions.push_back(s); }
    void add_inc(const std::string &s) { m_includes.push_back(s); }
    void add_pfx(const std::string &s) { m_prefix.push_back(s); }
    void add_sym(const std::string &s) { m_symbols.push_back(s); }

    /* generate output */
    int generate(const std::string &ofile, const std::string &name);

};

#endif //_GENDLOPEN_HPP_

