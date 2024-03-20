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
    std::string m_buf;
    std::ifstream m_ifs;

public:

    cin_ifstream() {}
    ~cin_ifstream() {}

    bool open(const std::string &file)
    {
        close();

        if (file == "-") {
            /* STDIN */
            m_stdin = true;
        } else {
            /* file */
            m_ifs.open(file.c_str());
        }

        /* clear buffer */
        m_buf.clear();

        return is_open();
    }

    bool is_open() const
    {
        if (!m_buf.empty()) {
            return true;
        }
        return m_stdin ? true : m_ifs.is_open();
    }

    void close() {
        if (m_ifs.is_open()) m_ifs.close();
    }

    bool get(char &c)
    {
        if (!m_buf.empty()) {
            /* buffer */
            c = m_buf.front();
            m_buf.erase(0, 1);
            return true;
        } else if (m_stdin) {
            /* STDIN */
            return std::cin.get(c) ? true : false;
        }

        /* file */
        return m_ifs.get(c) ? true : false;
    }

    int peek()
    {
        if (!m_buf.empty()) {
            return m_buf.front();
        }
        return m_stdin ? std::cin.peek() : m_ifs.peek();
    }

    bool good() const
    {
        if (!m_buf.empty()) {
            return true;
        }
        return m_stdin ? std::cin.good() : m_ifs.good();
    }

    void ignore()
    {
        if (!m_buf.empty()) {
            /* buffer */
            m_buf.erase(0, 1);
        } else if (m_stdin) {
            /* STDIN */
            std::cin.ignore();
        } else {
            /* file */
            m_ifs.ignore();
        }
    }

    bool getline(std::string &line)
    {
        if (!m_buf.empty()) {
            /* non-empty buffer is always at least 1 line */
            auto pos = m_buf.find('\n');
            line = m_buf.substr(0, pos);
            m_buf.erase(0, pos);
            return true;
        } else if (m_stdin) {
            /* STDIN */
            return std::getline(std::cin, line) ? true : false;
        }

        /* file */
        return std::getline(m_ifs, line) ? true : false;
    };

    void ungetline(const std::string &line)
    {
        /* always add a newline so we can extract
         * it as a whole line again */
        m_buf.insert(0, 1, '\n');
        m_buf.insert(0, line);
    }
};

#endif //_CIN_IFSTREAM_HPP_
