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

#ifndef _CIO_HPP_
#define _CIO_HPP_

#include <iostream>
#include <fstream>
#include <string>


namespace cio
{

/* wrapper class to enable reading input from
 * a file or std::cin using the same object */
class ifstream;

/* wrapper class to enable writing output to
 * a file or std::cout using the same object */
class ofstream;


class ifstream
{
private:

    bool m_stdin = false;
    std::string m_buf;
    std::ifstream m_ifs;

public:

    ifstream() {}
    ~ifstream() {}

    bool open(const std::string &file, std::ios::openmode mode = std::ios::in | std::ios::binary)
    {
        close();

        if (file == "-") {
            /* STDIN */
            m_stdin = true;
        } else {
            /* file */
            m_ifs.open(file.c_str(), mode);
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

    bool getline(std::string &out)
    {
        if (!m_buf.empty()) {
            /* buffer */
            out = m_buf;
            m_buf.clear();

            if (out.back() == '\n') {
                out.pop_back();
            }
            return true;
        } else if (m_stdin) {
            /* STDIN */
            return std::getline(std::cin, out) ? true : false;
        }

        /* file */
        return std::getline(m_ifs, out) ? true : false;
    }

    /* get a preview of the next line */
    bool peek_line(std::string &out)
    {
        if (m_buf.empty() && !getline(m_buf)) {
            return false;
        }

        /* always add a newline to buffer so we can
         * extract it as a whole line again */
        if (m_buf.back() != '\n') {
            m_buf.push_back('\n');
        }

        out = m_buf;

        /* remove newline */
        out.pop_back();

        return true;
    }
};


class ofstream
{
private:

    bool m_stdout = false;
    std::ofstream m_ofs;

public:

    ofstream() {}
    ~ofstream() {}

    bool open(const std::string &file, std::ios::openmode mode = std::ios::out | std::ios::binary)
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

} /* namespace cio */

#endif //_CIN_IFSTREAM_HPP_
