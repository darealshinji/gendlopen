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

#include <cctype>
#include "utils.hpp"


/* convert string to uppercase */
std::string utils::to_upper(const std::string &str, bool underscores)
{
    std::string out;

    if (underscores) {
        for (const char &c : str) {
            if (::isalnum(c)) {
                out += static_cast<char>(::toupper(c));
            } else {
                out += '_';
            }
        }
    } else {
        for (const char &c : str) {
            out += static_cast<char>(::toupper(c));
        }
    }

    return out;
}

/* convert string to lowercase */
std::string utils::to_lower(const std::string &str, bool underscores)
{
    std::string out;

    if (underscores) {
        for (const char &c : str) {
            if (::isalnum(c)) {
                out += static_cast<char>(::tolower(c));
            } else {
                out += '_';
            }
        }
    } else {
        for (const char &c : str) {
            out += static_cast<char>(::tolower(c));
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
        if (s.starts_with(e)) {
            return true;
        }
    }
    return false;
}

/* strip ANSI white-space characters from front and back */
void utils::strip_spaces(std::string &s)
{
    /* remove from back */
    while (!s.empty() && ::isspace(s.back())) {
        s.pop_back();
    }

    /* remove from front */
    while (!s.empty() && ::isspace(s.front())) {
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

/* read input lines */
bool utils::get_lines(FILE *fp, std::string &line, template_t &entry)
{
    bool loop = true;
    int c = EOF;

    line.clear();
    entry.maybe_keyword = false;
    entry.line_count = 1;

    /* just in case */
    if (!fp) {
        loop = false;
    }

    while (loop)
    {
        c = fgetc(fp);

        switch (c)
        {
        case '\n':
            /* concatenate lines ending on '@' */
            if (line.ends_with('@')) {
                line.back() = '\n';
                entry.line_count++;
                continue;
            } else if (line.ends_with("@\r")) {
                /* Windows line ending */
                line.replace(line.size() - 2, 2, "\r\n");
                entry.line_count++;
                continue;
            }
            loop = false;
            break;

        case EOF:
            /* remove trailing '@' */
            delete_suffix(line, '@');
            loop = false;
            break;

        case '%':
            entry.maybe_keyword = true;
            [[fallthrough]];

        default:
            line.push_back(static_cast<char>(c));
            continue;
        }
    }

    entry.data = string_to_data(line);

    return (c == EOF);
}


/* append missing path separator */
void utils::append_missing_separator(std::string &path)
{
    switch (utils::str_back(path))
    {
#ifdef _WIN32
    case '\\':
#endif
    case '/':
        break;
    default:
        path.push_back('/');
        break;
    }
}


#ifdef UTILS_PROGNAME_FALLBACK

/* get program name without full path */
const char *utils::progname(const char *argv0)
{
#ifdef _WIN32
    int path_separator = '\\';
#else
    int path_separator = '/';
#endif

    const char *p = strrchr(argv0, path_separator);

    if (p && *(p+1) != 0) {
        return p + 1;
    }

    return argv0;
}

#endif /* UTILS_PROGNAME_FALLBACK */

