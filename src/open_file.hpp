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

/**
 * Tiny wrapper class around fopen()/fclose() used to open files for reading.
 * Main purpose is automatic stream closing through class destructor and
 * direct usage of "stdin" file pointer.
 */

#pragma once

#include <cstdio>
#include <filesystem>
#include <string>


class open_file
{
private:

    FILE *m_fp = NULL;

    static inline FILE *call_fopen(const char *path, const char *mode)
    {
#ifdef _MSC_VER
        FILE *fp;

        if (::fopen_s(&fp, path, mode) == 0) {
           return fp;
        }

        return NULL;
#else
        return ::fopen(path, mode);
#endif
    }

    bool do_open(const std::string &path)
    {
        close();

        if (path.empty() || path == "-") {
            m_fp = stdin;
        } else {
            m_fp = call_fopen(path.c_str(), "rb");
        }

        return is_open();
    }

public:

    open_file() {}
    open_file(const std::filesystem::path &path) { open(path.string()); }
    open_file(const std::string &path)           { open(path); }

    ~open_file() { close(); }

    bool open(const std::string &path)           { return do_open(path); }
    bool open(const std::filesystem::path &path) { return do_open(path.string()); }

    FILE *fp() const      { return m_fp; }
    bool is_open() const  { return (m_fp != NULL); }
    bool is_stdin() const { return (m_fp == stdin); }

    void close() {
        if (m_fp && m_fp != stdin) {
            ::fclose(m_fp);
        }
        m_fp = stdin;
    }

    bool getc(char &c)
    {
        int val;

        if (m_fp && (val = ::fgetc(m_fp)) != EOF) {
            c = static_cast<char>(val);
            return true;
        }

        return false;
    }
};
