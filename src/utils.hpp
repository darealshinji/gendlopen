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

#ifndef GDO_UTILS_HPP
#define GDO_UTILS_HPP


/* common functions */

namespace utils
{

/* print filename to STDERR */
void print_filename(const std::filesystem::path &path, bool newline);
void print_filename(const char *name, bool newline);

inline void print_filename(const std::string &name, bool newline)
{
    print_filename(name.c_str(), newline);
}

/* case-insensitive string comparison */
bool eq_str_case(const std::string &str1, const std::string &str2);

/* case-insensitive comparison if string begins with prefix (and is longer than prefix) */
inline bool prefixed_case(const std::string &str, const std::string &pfx)
{
    return (str.size() > pfx.size() && eq_str_case(str.substr(0, pfx.size()), pfx));
}

/* convert a string to uppercase or lowercase
 *
 * underscores=true will convert any character not matching [A-Za-z0-9] to underscore `_'
 * underscores=false will preserve any character not matching [A-Za-z0-9] */
std::string convert_to_upper(const std::string &s, bool underscores=true);
std::string convert_to_lower(const std::string &s, bool underscores=true);

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

/* delete suffix from string */
inline void delete_suffix(std::string &str, const std::string &suffix)
{
    if (str.ends_with(suffix)) {
        str.erase(str.size() - suffix.size());
    }
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

/* strip ANSI colors from line */
inline void strip_ansi_colors(std::string &line)
{
    const std::regex reg(R"(\x1B\[[0-9;]*m)");
    std::string out = std::regex_replace(line, reg, "");
    line = out;
}

/* whether "c" is within the range of "beg" and "end" */
template<typename T=char>
inline bool range(T c, T beg, T end)
{
    assert(beg < end);
    return (c >= beg && c <= end);
}

} /* namespace utils end */

#endif /* GDO_UTILS_HPP */

