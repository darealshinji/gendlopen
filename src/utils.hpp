/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2024-2026 Carsten Janssen

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
#include <errno.h>  /* program_invocation_short_name */
#include <stdio.h>
#include <stdlib.h>  /* _get_pgmptr */
#include <string.h>
#include <algorithm>
#include <filesystem>
#include <locale>
#include <string>
#include <vector>
#include "types.hpp"


/* common functions */

namespace utils
{

/* convert a string to uppercase or lowercase
 *
 * underscores=true will convert any character not matching [A-Za-z0-9] to underscore `_'
 * underscores=false will preserve any character not matching [A-Za-z0-9] */
std::string to_upper(const std::string &s, bool underscores=true);
std::string to_lower(const std::string &s, bool underscores=true);


#ifdef __MINGW32__
/* bug: need to explicitly set locale, otherwise the program
 * may crash if the name contains non-ANSI characters */
template<typename T> fs::path mkpath(T name) {
    return fs::path(name, std::locale());
}

template<typename T1, typename T2>
fs::path mkpath(T1 name1, T2 name2) {
    return mkpath(name1) / mkpath(name2);
}
#else //!__MINGW32__
template<typename T> fs::path mkpath(T name) {
    return fs::path{name};
}

template<typename T1, typename T2>
fs::path mkpath(T1 name1, T2 name2) {
    return fs::path{name1} / name2;
}
#endif //!__MINGW32__


#ifdef _MSC_VER
inline int strcasecmp(const char *a, const char *b)            { return ::_stricmp(a, b); }
inline int strncasecmp(const char *a, const char *b, size_t n) { return ::_strnicmp(a, b, n); }
#else
inline int strcasecmp(const char *a, const char *b)            { return ::strcasecmp(a, b); }
inline int strncasecmp(const char *a, const char *b, size_t n) { return ::strncasecmp(a, b, n); }
#endif


/* syntactic sugar */
template<typename T>
bool find(const std::string &haystack, const T &needle, size_t &pos) {
    return ((pos = haystack.find(needle)) != std::string::npos);
}

template<typename T>
bool find(const std::string &haystack, const T &needle) {
    return (haystack.find(needle) != std::string::npos);
}


/* case-insensitive comparison if string begins with prefix (and is longer than prefix) */
template<size_t N>
bool prefixed_and_longer_case(const std::string &str, char const (&pfx)[N]) {
    return (str.size() > N-1 && strncasecmp(str.c_str(), pfx, N-1) == 0);
}


/* returns true if s begins with a prefix found in list */
inline bool is_prefixed(const std::string &s, const vstring_t &list)
{
    if (!s.empty() && !list.empty()) {
        for (const auto &e : list) {
            if (s.starts_with(e)) {
                return true;
            }
        }
    }

    return false;
}


/* strip ANSI white-space characters from front and back */
inline void strip_spaces(std::string &s) {
    while (!s.empty() && ::isspace(s.back()))  { s.pop_back(); }
    while (!s.empty() && ::isspace(s.front())) { s.erase(0, 1); }
}


/* replace string "from" with string "to" in string "s" */
inline void replace(const std::string &from, const std::string &to, std::string &s) {
    for (size_t pos = 0; utils::find(s, from, pos); pos += to.size()) {
        s.replace(pos, from.size(), to);
    }
}


/* count '\n' characters */
inline size_t count_linefeed(const std::string &str)
{
    size_t n = 0;

    for (auto &c : str) {
        if (c == '\n') {
            n++;
        }
    }

    return n;
}


/* get program name without full path (used in messages) */
#ifdef HAVE_PROGRAM_INVOCATION_SHORT_NAME
inline const char *progname(char *) { return program_invocation_short_name; }
#elif defined(HAVE_GETPROGNAME)
inline const char *progname(char *) { return getprogname(); }
#elif defined(_WIN32)
inline std::string progname(char *argv0) {
    char *ptr = (_get_pgmptr(&ptr) == 0) ? ptr : argv0;
    return mkpath(ptr).stem().string();
}
#else
inline std::string progname(char *argv0) {
    return mkpath(argv0).filename().string();
}
#endif


/* std::string's .at() and .front() methods but NUL
 * is returned instead of causing undefined behavior */
inline char str_at(const std::string &str, size_t pos) {
    return (str.size() > pos) ? str.at(pos) : 0;
}

inline char str_front(const std::string &str) {
    return str.empty() ? 0 : str.front();
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


/* get correct filter mode */
template <class T>
filter::mode get_filter_mode(const T &vec_prefixes, const T &vec_symbols)
{
    if (vec_prefixes.size() > 0 && vec_symbols.size() > 0) {
        return filter::pfx_list;
    } else if (vec_prefixes.size() > 0) {
        return filter::prefix;
    } else if (vec_symbols.size() > 0) {
        return filter::list;
    }

    return filter::none;
}


/* sort list and remove duplicates */
template <class T>
void sort_dedup(T &list)
{
    if (list.size() > 1) {
        std::sort(list.begin(), list.end());
        auto last = std::unique(list.begin(), list.end());
        list.erase(last, list.end());
    }
}

} /* namespace utils end */

