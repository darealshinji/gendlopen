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
#include <vector>
#include <ctype.h>
#include <string.h>
#ifndef _MSC_VER
#include <strings.h>
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

inline bool same_string_case(const std::string &str1, const char *str2)
{
#ifdef _MSC_VER
    return (_stricmp(str1.c_str(), str2) == 0);
#else
    return (strcasecmp(str1.c_str(), str2) == 0);
#endif
}

inline void strip_spaces(std::string &in)
{
    while (isspace(in.back())) in.pop_back();
    while (isspace(in.front())) in.erase(0, 1);
}

/* replace_string(a,b,s) will substitute a with b in s */
inline void replace_string(const std::string &from, const std::string &to, std::string &s)
{
    for (size_t pos = 0; (pos = s.find(from, pos)) != std::string::npos; pos += to.size())
    {
        s.replace(pos, from.size(), to);
    }
}

#endif //_COMMON_HPP_
