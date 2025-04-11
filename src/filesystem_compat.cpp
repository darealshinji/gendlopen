/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2025 Carsten Janssen

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

#ifdef MINGW32_NEED_CONVERT_FILENAME
# include <wchar.h>
# include <cstdlib>
#endif
#ifdef __cpp_lib_filesystem
# include <filesystem>
#else
# include <sys/types.h>
# include <sys/stat.h>
# ifndef _WIN32
#  include <unistd.h>
# endif
# include "utils.hpp"
#endif
#include <iostream>
#include <string>

#include "gendlopen.hpp"
#include "filesystem_compat.hpp"
#include "types.hpp"


#ifdef MINGW32_NEED_CONVERT_FILENAME

/**
 * convert from string to wstring;
 * this is required because on MinGW std::filesystem will throw an exception
 * if an input string contains non-ASCII characters (this doesn't happend with MSVC)
 */

static wchar_t *char_to_wchar(const char *str)
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

std::wstring fs::convert_filename(const std::string &str)
{
    wchar_t *buf = char_to_wchar(str.c_str());

    if (!buf) {
        std::string msg = __FILE__;
        msg += ": char_to_wchar() failed to convert string to wide characters: ";
        msg += str;

        throw gendlopen::error(msg);
    }

    std::wstring ws = buf;
    delete[] buf;

    return ws;
}

#endif /* MINGW32_NEED_CONVERT_FILENAME */


#if !defined(__cpp_lib_filesystem)

std::string fs::filename(const std::string &path)
{
#ifdef _WIN32
    if (utils::ends_with(path, '\\') || utils::ends_with(path, '/')) {
        return "";
    }

    auto pos = path.find_last_of("\\/");
#else
    if (utils::ends_with(path, '/')) {
        return "";
    }

    auto pos = path.rfind('/');
#endif

    return (pos == std::string::npos) ? path : path.substr(pos+1);
}


/* not an exact replica of std::filesystem::path::replace_extension()
 * but it does exactly what we want */
void fs::replace_extension(std::string &path, const std::string &ext)
{
    std::string dirname, basename, dot;

    /* first split path into dirname and basename */
#ifdef _WIN32
    auto pos = path.find_last_of("\\/");
#else
    auto pos = path.rfind('/');
#endif

    if (pos == std::string::npos) {
        basename = path;
    } else {
        basename = path.substr(pos+1);
        dirname = path.substr(0, pos+1);
    }

    /* remove leading dot temporarily from basename */
    if (utils::starts_with(basename, '.')) {
        dot = ".";
        basename.erase(0, 1);
    }

    /* erase old extension */
    pos = basename.rfind('.');

    if (pos != std::string::npos) {
        basename.erase(pos);
    }

    path = dirname + dot + basename + ext;
}

#endif // !__cpp_lib_filesystem

