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

ofstream::ofstream() : m_stdout(false)
{}

ofstream::~ofstream()
{}

bool ofstream::open(const std::string &file, std::ios::openmode mode)
{
    if (file == "-") {
        /* STDOUT */
        m_stdout = true;
    } else {
        /* file */
        m_ofs.open(file.c_str(), mode);
    }

    return is_open();
}

bool ofstream::is_open() const {
    return m_stdout ? true : m_ofs.is_open();
}

void ofstream::close()
{
    if (m_stdout) {
        /* flush content */
        std::cout << std::flush;
    } else if (m_ofs.is_open()) {
        m_ofs.close();
    }
}

} /* namespace cio */
