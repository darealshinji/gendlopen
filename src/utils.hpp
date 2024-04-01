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

#ifndef _UTILS_HPP_
#define _UTILS_HPP_

#include <string>
#include <utility>
#include <vector>
#include <ctype.h>
#include <string.h>
#ifndef _MSC_VER
#include <strings.h>
#endif


/* ANSI color codes used in the Clang AST output */
#ifndef CLANG_ANSI_COLORS
#define CLANG_ANSI_COLORS
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
#endif //CLANG_ANSI_COLORS


/* assume */
#if defined(__has_cpp_attribute) && __has_cpp_attribute(assume) >= 202207L
    #define ASSUME(x)    [[assume(x)]]
#elif defined(__clang__)
    #define ASSUME(x)    __builtin_assume(x)
#elif defined(_MSC_VER)
    #define ASSUME(x)    __assume(x)
#elif defined(__GNUC__) && __GNUC__ >= 13
    #define ASSUME(x)    [[gnu::assume(x)]]
#else
    #define ASSUME(x)    /**/
#endif


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


/* common inline functions */

namespace utils
{

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

inline int xstrcasecmp(const char *str1, const char *str2)
{
    ASSUME(str1 != NULL);
    ASSUME(str2 != NULL);

#ifdef _MSC_VER
    return _stricmp(str1, str2);
#else
    return strcasecmp(str1, str2);
#endif
}

/* case-insensitive string comparison */
inline bool eq_str_case(const std::string &str1, const char *str2)
{
    return (xstrcasecmp(str1.c_str(), str2) == 0);
}

/* case-insensitive check if str begins with and is longer than pfx */
inline bool beglt_case(const std::string &str, const std::string &pfx)
{
    if (str.size() <= pfx.size()) {
        return false;
    }

    return (xstrcasecmp(str.substr(0, pfx.size()).c_str(), pfx.c_str()) == 0);
}

/* strip white-spaces from front and back of a string */
inline void strip_spaces(std::string &in)
{
    while (isspace(in.back())) in.pop_back();
    while (isspace(in.front())) in.erase(0, 1);
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
    ASSUME(beg < end);
    return (c >= beg && c <= end);
}

/* just in case std::unreachable is not implemented */
[[noreturn]] inline void unreachable()
{
#ifdef __cpp_lib_unreachable
    std::unreachable();
#elif defined(__GNUC__) || defined(__clang__)
    __builtin_unreachable();
#elif defined(_MSC_VER)
    __assume(false);
#else
    std::abort();
#endif
}

} /* namespace common */

#endif //_UTILS_HPP_
