/**
 * The MIT License (MIT)
 *
 * Copyright (C) 2024 djcj@gmx.de
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

#ifndef _COMMON_HPP_
#define _COMMON_HPP_

#include <string>
#include <utility>
#include <vector>
#include <ctype.h>
#include <string.h>
#ifndef _MSC_VER
#include <strings.h>
#endif

/* not supported by all compilers yet */
//#define ASSUME(x)  [[assume(x)]]        // generic C++
//#define ASSUME(x)  [[gnu::assume(x)]]   // GNU C++
//#define ASSUME(x)  __assume(x)          // MSVC
//#define ASSUME(x)  __builtin_assume(x)  // Clang
#ifndef ASSUME
#define ASSUME(x)  /**/
#endif


/* typedefs */

typedef struct {
    std::string type;
    std::string symbol;
    std::string args;
    std::string notype_args;
} proto_t;

typedef struct {
    std::string type;
    std::string symbol;
} obj_t;

typedef std::vector<proto_t> vproto_t;
typedef std::vector<obj_t> vobj_t;
typedef std::vector<std::string> vstring_t;

namespace output
{
    typedef enum {
        c,
        cxx,
        minimal
    } format;
}


/* common inline functions */

namespace common
{

/* case-insensitive string comparison */
inline bool same_string_case(const std::string &str1, const char *str2)
{
#ifdef _MSC_VER
    return (_stricmp(str1.c_str(), str2) == 0);
#else
    return (strcasecmp(str1.c_str(), str2) == 0);
#endif
}

/* strip white-spaces from front and back of a string */
inline void strip_spaces(std::string &in)
{
    while (isspace(in.back())) in.pop_back();
    while (isspace(in.front())) in.erase(0, 1);
}

/* replace string "from" with string "to" in string "s" */
inline void replace_string(const std::string &from, const std::string &to, std::string &s)
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
#elif defined(_MSC_VER) && !defined(__clang__)
    __assume(false); // MSVC
#else
    __builtin_unreachable(); // GCC, Clang
#endif
}

} /* namespace common */

#endif //_COMMON_HPP_
