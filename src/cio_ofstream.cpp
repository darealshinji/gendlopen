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
#include "gendlopen.hpp"


namespace cio
{

bool ofstream::open(const std::filesystem::path &path)
{
    close();
    m_ofs.open(path, std::ios_base::out | std::ios_base::binary);

    return m_ofs.is_open();
}

bool ofstream::open(const std::string &file)
{
    if (file != "-")
    {
#ifdef _MSC_VER
        /* std::filesystem does the filename conversion for us;
         * this is broken on MinGW32 */
        return open(std::filesystem::path(file));
#else
        close();
        m_ofs.open(file, std::ios_base::out | std::ios_base::binary);
        return m_ofs.is_open();
#endif
    }

    /* STDOUT */
    close();

    return true;
}

void ofstream::close()
{
    std::cout << std::flush;

    if (m_ofs.is_open()) {
        m_ofs.close();
    }
}

} /* namespace cio */
