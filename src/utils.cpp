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

#include <ctype.h>
#include <vector>
#include "utils.hpp"


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

/* returns true if s begins with a prefix found in list */
bool utils::is_prefixed(const std::string &s, const vstring_t &list)
{
    if (s.empty() || list.empty()) {
        return false;
    }

    for (const auto &e : list) {
        if (starts_with(s, e)) {
            return true;
        }
    }
    return false;
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
size_t utils::count_linefeed(const std::string &str)
{
    size_t n = 0;

    for (auto &c : str) {
        if (c == '\n') {
            n++;
        }
    }

    return n;
}

/* erase all occurences of string "str" from vector "v" */
size_t utils::find_and_erase(vstring_t &v, const std::string &str)
{
    if (v.empty()) {
        return 0;
    }

#ifdef __cpp_lib_erase_if
    return std::erase(v, str);
#else
    const auto it = std::remove(v.begin(), v.end(), str);
    const auto rv = v.end() - it;
    v.erase(it, v.end());
    return rv;
#endif
}

