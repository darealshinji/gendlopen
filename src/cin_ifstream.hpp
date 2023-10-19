/****
  The MIT License (MIT)

  Copyright (C) 2023 djcj@gmx.de

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE
****/

#ifndef _CIN_IFSTREAM_HPP_
#define _CIN_IFSTREAM_HPP_

#include <iostream>
#include <fstream>
#include <string>


/* wrapper class to enable reading input from
 * a file or std::cin using the same object */
class cin_ifstream
{
private:

    bool m_stdin = false;
    std::ifstream m_ifs;

    inline bool get_stdin(char &c) {
        return std::cin.get(c) ? true : false;
    }

    inline bool get_fstream(char &c) {
        return m_ifs.get(c) ? true : false;
    }

public:

    /* read from std::cin if input is "-" */
    cin_ifstream(const std::string &file)
    {
        if (file == "-") {
            m_stdin = true;
        } else {
            m_ifs.open(file.c_str());
        }
    }

    ~cin_ifstream() {
        close();
    }

    bool is_open() const {
        return m_stdin ? true : m_ifs.is_open();
    }

    void close() {
        if (m_ifs.is_open()) m_ifs.close();
    }

    bool get(char &c) {
        return m_stdin ? get_stdin(c) : get_fstream(c);
    }

    int peek() {
        return m_stdin ? std::cin.peek() : m_ifs.peek();
    }

    bool good() const {
        return m_stdin ? std::cin.good() : m_ifs.good();
    }

    void ignore()
    {
        if (m_stdin) {
            std::cin.ignore();
        } else {
            m_ifs.ignore();
        }
    }
};

#endif //_CIN_IFSTREAM_HPP_
