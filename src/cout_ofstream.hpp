/**
 * The MIT License (MIT)
 *
 * Copyright (C) 2024 djcj@gmx.de
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

#ifndef _COUT_OFSTREAM_HPP_
#define _COUT_OFSTREAM_HPP_

#include <iostream>
#include <fstream>
#include <string>


class cout_ofstream
{
private:

    bool m_stdout = false;
    std::ofstream m_ofs;

public:

    cout_ofstream() {}
    ~cout_ofstream() {}

    bool open(const std::string &file, std::ios::openmode mode = std::ios::out)
    {
        if (file == "-") {
            m_stdout = true;
        } else {
            m_ofs.open(file.c_str(), mode);
        }

        return is_open();
    }

    bool is_open() const {
        return m_stdout ? true : m_ofs.is_open();
    }

    void close()
    {
        if (m_stdout) {
            std::cout << std::flush;
        } else if (m_ofs.is_open()) {
            m_ofs.close();
        }
    }

    template<class T>
    std::ostream& operator<<(const T &obj)
    {
        if (m_stdout) {
            return std::cout << obj;
        }
        return m_ofs << obj;
    }
};


#endif //_COUT_OFSTREAM_HPP_
