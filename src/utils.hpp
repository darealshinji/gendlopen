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
/* convert a string to uppercase or lowercase
 *
 * underscores=true will convert any character not matching [A-Za-z0-9] to underscore `_'
 * underscores=false will preserve any character not matching [A-Za-z0-9] */
std::string convert_to_upper(const std::string &s, bool underscores=true);
std::string convert_to_lower(const std::string &s, bool underscores=true);

/* returns true if s begins with a prefix found in list */
bool is_prefixed(const std::string &s, const vstring_t &list);

/* delete suffix from string */
void delete_suffix(std::string &str, const std::string &suffix);
void delete_suffix(std::string &str, const char suffix);

/* strip ANSI white-space characters from front and back */
void strip_spaces(std::string &s);

/* replace string "from" with string "to" in string "s" */
void replace(const std::string &from, const std::string &to, std::string &s);

/* count '\n' characters */
int count_linefeed(const std::string &str);

} /* namespace utils end */

#endif /* GDO_UTILS_HPP */

