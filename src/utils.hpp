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
#include "misc.hpp"


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
#elif defined(__has_builtin) && __has_builtin(__builtin_unreachable)
    __builtin_unreachable();
#elif defined(_MSC_VER)
    __assume(false);
#else
    std::abort();
#endif
}

} /* namespace common */

#endif //_UTILS_HPP_
