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
#include <string.h>
#include <string>
#include <vector>
#include "types.hpp"


/* common functions */

namespace utils
{
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
std::string convert_to_upper(const std::string &s, bool underscores=true);
std::string convert_to_lower(const std::string &s, bool underscores=true);

/* returns true if s begins with a prefix found in list */
bool is_prefixed(const std::string &s, const vstring_t &list);

/* strip ANSI white-space characters from front and back */
void strip_spaces(std::string &s);

/* replace string "from" with string "to" in string "s" */
void replace(const std::string &from, const std::string &to, std::string &s);

/* count '\n' characters */
size_t count_linefeed(const std::string &str);

/* return char at position pos or NUL */
inline char str_at(const std::string &str, size_t pos) {
    return (str.size() > pos) ? str.at(pos) : 0;
}

/* return char at position 0 or NUL */
inline char str_front(const std::string &str) {
    return str.empty() ? 0 : str.front();
}

/* erase all occurences of string "str" from vector "v" */
size_t find_and_erase(vstring_t &v, const std::string &str);


#ifdef __cpp_lib_starts_ends_with

template <typename T>
bool starts_with(const std::string &str, const T &prefix) {
    return str.starts_with(prefix);
}

template <typename T>
bool ends_with(const std::string &str, const T &suffix) {
    return str.ends_with(suffix);
}

#else

/* starts_with() */
template<size_t N>
bool starts_with(const std::string &str, char const (&prefix)[N]) {
    constexpr size_t prefix_size = N-1;
    return (str.size() >= prefix_size &&
            ::strncmp(str.c_str(), prefix, prefix_size) == 0);
}

static inline bool starts_with(const std::string &str, const std::string &prefix) {
    return (str.size() >= prefix.size() &&
            ::strncmp(str.c_str(), prefix.c_str(), prefix.size()) == 0);
}

static inline bool starts_with(const std::string &str, const char &prefix) {
    return (!str.empty() && str.front() == prefix);
}

/* ends_with() */
template<size_t N>
bool ends_with(const std::string &str, char const (&suffix)[N]) {
    constexpr size_t suffix_size = N-1;
    return (str.size() >= suffix_size &&
            ::strcmp(str.c_str() + (str.size() - suffix_size), suffix) == 0);
}

static inline bool ends_with(const std::string &str, const std::string &suffix) {
    return (str.size() >= suffix.size() &&
            ::strcmp(str.c_str() + (str.size() - suffix.size()), suffix.c_str()) == 0);
}

static inline bool ends_with(const std::string &str, const char &prefix) {
    return (!str.empty() && str.back() == prefix);
}

#endif // __cpp_lib_starts_ends_with


template <typename T1, typename T2>
bool starts_ends_with(const std::string &str, const T1 &prefix, const T2 &suffix) {
    return (starts_with(str, prefix) && ends_with(str, suffix));
}


/* delete suffix from string */

inline void delete_suffix(std::string &str, const std::string &suffix) {
    if (ends_with(str, suffix)) {
        str.erase(str.size() - suffix.size());
    }
}

inline void delete_suffix(std::string &str, const char suffix) {
    if (ends_with(str, suffix)) {
        str.pop_back();
    }
}

} /* namespace utils end */

