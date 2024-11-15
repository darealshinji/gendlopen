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

#include <iostream>
#include <string>
#include <stdio.h>

#include "open_file.hpp"


open_file::open_file(const std::string &path)
{
    if (path.empty() || path == "-") {
        m_fp = stdin;
    } else {
#ifdef _MSC_VER
        /* "secure" variant for MSVC */
        if (fopen_s(&m_fp, path.c_str(), "rb") != 0) {
            m_fp = NULL;
        }
#else
        m_fp = fopen(path.c_str(), "rb");
#endif
    }
}

open_file::~open_file()
{
    close();
}

bool open_file::is_open() const
{
    return (m_fp != NULL);
}

void open_file::close()
{
    if (m_fp && m_fp != stdin) {
        fclose(m_fp);
    }

    m_fp = stdin;
}

FILE *open_file::file_pointer() const
{
    return m_fp;
}
