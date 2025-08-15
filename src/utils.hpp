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

#pragma once

#if !defined(_MSC_VER)
# include <strings.h>
#endif
#include <stdio.h>
#include <string.h>
#include <string>
#include <vector>
#include "types.hpp"


/* common functions */

namespace utils
{

/* MSVC compat */

inline int strcasecmp(const char *a, const char *b)
{
#ifdef _MSC_VER
    return ::_stricmp(a, b);
#else
    return ::strcasecmp(a, b);
#endif
}

inline int strncasecmp(const char *a, const char *b, size_t n)
{
#ifdef _MSC_VER
    return ::_strnicmp(a, b, n);
#else
    return ::strncasecmp(a, b, n);
#endif
}


/* case-insensitive comparison if string begins with prefix (and is longer than prefix) */
template<size_t N>
bool prefixed_and_longer_case(const std::string &str, char const (&pfx)[N])
{
    return (str.size() > N-1 && utils::strncasecmp(str.c_str(), pfx, N-1) == 0);
}


/* convert a string to uppercase or lowercase
 *
 * underscores=true will convert any character not matching [A-Za-z0-9] to underscore `_'
 * underscores=false will preserve any character not matching [A-Za-z0-9] */
std::string to_upper(const std::string &s, bool underscores=true);
std::string to_lower(const std::string &s, bool underscores=true);


/* returns true if s begins with a prefix found in list */
bool is_prefixed(const std::string &s, const vstring_t &list);


/* strip ANSI white-space characters from front and back */
void strip_spaces(std::string &s);


/* replace string "from" with string "to" in string "s" */
void replace(const std::string &from, const std::string &to, std::string &s);


/* count '\n' characters */
size_t count_linefeed(const std::string &str);


/* std::string's .at(), .front() and .back() methods, but NUL
 * is returned instead of causing undefined behavior */
inline char str_at(const std::string &str, size_t pos) {
    return (str.size() > pos) ? str.at(pos) : 0;
}

inline char str_front(const std::string &str) {
    return str.empty() ? 0 : str.front();
}

inline char str_back(const std::string &str) {
    return str.empty() ? 0 : str.back();
}


/* check string for prefix and suffix */
template <typename T1, typename T2>
bool front_and_back(const std::string &str, const T1 &prefix, const T2 &suffix) {
    return (str.starts_with(prefix) && str.ends_with(suffix));
}


/* delete suffix from string */

inline void delete_suffix(std::string &str, const std::string &suffix) {
    if (str.ends_with(suffix)) {
        str.erase(str.size() - suffix.size());
    }
}

inline void delete_suffix(std::string &str, const char suffix) {
    if (str.ends_with(suffix)) {
        str.pop_back();
    }
}

} /* namespace utils end */

