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

#include "cio_ifstream.hpp"


namespace fs = std::filesystem;

namespace cio
{

ifstream::ifstream()
{}

ifstream::~ifstream()
{
    close();
}

bool ifstream::open(const fs::path &path)
{
    close();
    m_ifs.open(path, std::ios_base::in | std::ios_base::binary);

    if (m_ifs.is_open()) {
        m_fsize = fs::file_size(path);
        m_iptr = &m_ifs;
        return true;
    }

    return false;
}

bool ifstream::open(const std::string &file)
{
    /* STDIN */
    if (file == "-") {
        close();
        m_iptr = &std::cin;
        return true;
    }

    return open(fs::path(file));
}

bool ifstream::is_stdin() const
{
    return (m_iptr && m_iptr == &std::cin);
}

bool ifstream::is_open() const
{
    return is_stdin() ? true : m_ifs.is_open();
}

void ifstream::close()
{
    m_fsize = 0;
    m_buf.clear();
    m_iptr = NULL;

    if (m_ifs.is_open()) {
        m_ifs.close();
    }
}

bool ifstream::get(char &c)
{
    if (!m_buf.empty()) {
        /* buffer */
        c = m_buf.front();
        m_buf.erase(0, 1);
        return true;
    } else if (m_iptr && m_iptr->get(c)) {
        return true;
    }

    return false;
}

int ifstream::peek()
{
    if (!m_buf.empty()) {
        return m_buf.front();
    }

    if (m_iptr) {
        return m_iptr->peek();
    }

    return std::char_traits<char>::eof();
}

bool ifstream::good() const
{
    if (!m_buf.empty()) {
        return true;
    }

    if (m_iptr) {
        return m_iptr->good();
    }

    return false;
}

size_t ifstream::file_size() const
{
    return m_fsize;
}

void ifstream::ignore(size_t n)
{
    if (!m_buf.empty()) {
        /* buffer */
        if (m_buf.size() >= n) {
            m_buf.clear();
        } else {
            m_buf.erase(0, n);
        }
    } else if (m_iptr) {
        m_iptr->ignore(n);
    }
}

void ifstream::ignore_line()
{
    constexpr auto max_size = std::numeric_limits<std::streamsize>::max();

    if (!m_buf.empty()) {
        /* buffer */
        m_buf.clear();
    } else if (m_iptr) {
        m_iptr->ignore(max_size, '\n');
    }
}

bool ifstream::getline(std::string &out)
{
    if (!m_buf.empty()) {
        /* buffer */
        out = m_buf;
        m_buf.clear();

        if (out.back() == '\n') {
            out.pop_back();
        }
        return true;
    } else if (m_iptr && std::getline(*m_iptr, out)) {
        return true;
    }

    return false;
}

/* get a preview of the next line */
bool ifstream::peek_line(std::string &out)
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

} /* namespace cio */

