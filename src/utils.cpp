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

#include <filesystem>
#include <string>
#include <stdio.h>

#include "gendlopen.hpp"


/**
 * convert from string to wstring;
 * this is required because on MinGW std::filesystem will throw an exception
 * if the string contains non-ASCII characters (this doesn't happend with MSVC)
 */
#ifdef __MINGW32__
wchar_t *utils::char_to_wchar(const char *str)
{
    size_t len, n;
    wchar_t *buf;

    if (!str || ::mbstowcs_s(&len, NULL, 0, str, 0) != 0 || len == 0) {
        return nullptr;
    }

    buf = new wchar_t[(len + 1) * sizeof(wchar_t)];
    if (!buf) return nullptr;

    if (::mbstowcs_s(&n, buf, len+1, str, len) != 0 || n == 0) {
        delete[] buf;
        return nullptr;
    }

    buf[len] = L'\0';
    return buf;
}

std::wstring utils::str_to_wstr(const std::string &str)
{
    wchar_t *buf = utils::char_to_wchar(str.c_str());

    if (!buf) {
        std::cerr << "error: failed to convert string to wide characters: "
            << str << std::endl;
        std::abort();
    }

    std::wstring ws = buf;
    delete[] buf;

    return ws;
}
#endif // __MINGW32__

/* case-insensitive string comparison (ignoring current locale) */
bool utils::eq_str_case(const std::string &str1, const std::string &str2)
{
    auto to_C_lower = [] (char c) -> char {
        if (range(c, 'A', 'Z')) {
            c += 32;  /* A-Z -> a-z */
        }
        return c;
    };

    /* different size means not equal */
    if (str1.size() != str2.size()) {
        return false;
    }

    auto it1 = str1.cbegin();
    auto it2 = str2.cbegin();

    for ( ; it1 != str1.cend(); it1++, it2++) {
        if (to_C_lower(*it1) != to_C_lower(*it2)) {
            return false;
        }
    }

    return true;
}

/* convert string to uppercase */
std::string utils::convert_to_upper(const std::string &str, bool underscores)
{
    std::string out;

    for (const char &c : str) {
        if (range(c, 'a','z')) {
            out += c - 32;
        } else if (!underscores || range(c, 'A','Z') || range(c, '0','9')) {
            out += c;
        } else {
            out += '_';
        }
    }

    return out;
}

/* convert string to lowercase */
std::string utils::convert_to_lower(const std::string &str, bool underscores)
{
    std::string out;

    for (const char &c : str) {
        if (range(c, 'A','Z')) {
            out += c + 32;
        } else if (!underscores || range(c, 'a','z') || range(c, '0','9')) {
            out += c;
        } else {
            out += '_';
        }
    }

    return out;
}
