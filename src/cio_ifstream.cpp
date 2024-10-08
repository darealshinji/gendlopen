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


static const std::ios_base::openmode def_mode = std::ios_base::in | std::ios_base::binary;

namespace fs = std::filesystem;


namespace cio
{

bool ifstream::open(const fs::path &path)
{
    close();
    m_ifs.open(path, def_mode);

    if (m_ifs.is_open()) {
        m_fsize = fs::file_size(path);
        return true;
    }

    return false;
}

bool ifstream::open(const std::string &file)
{
    close();

    /* STDIN */
    if (file == "-") {
        m_is_stdin = true;
        return true;
    }

    m_ifs.open(file, def_mode);

    if (m_ifs.is_open()) {
        fs::path path = file;
        m_fsize = fs::file_size(path);
        return true;
    }

    return false;
}

bool ifstream::is_open() const
{
    return m_is_stdin ? true : m_ifs.is_open();
}

void ifstream::close()
{
    m_is_stdin = false;
    m_fsize = 0;
    m_buf.clear();

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
    } else if (m_is_stdin) {
        /* STDIN */
        return std::cin.get(c) ? true : false;
    } else if (m_ifs.is_open()) {
        /* file */
        return m_ifs.get(c) ? true : false;
    }

    return false;
}

int ifstream::peek()
{
    if (!m_buf.empty()) {
        return m_buf.front();
    }

    return m_is_stdin ? std::cin.peek() : m_ifs.peek();
}

bool ifstream::good() const
{
    if (!m_buf.empty()) {
        return true;
    }

    return m_is_stdin ? std::cin.good() : m_ifs.good();
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
    } else if (m_is_stdin) {
        /* STDIN */
        std::cin.ignore(n);
    } else if (m_ifs.is_open()) {
        /* file */
        m_ifs.ignore(n);
    }
}

void ifstream::ignore_line()
{
    constexpr auto max_size = std::numeric_limits<std::streamsize>::max();

    if (!m_buf.empty()) {
        /* buffer */
        m_buf.clear();
    } else if (m_is_stdin) {
        /* STDIN */
        std::cin.ignore(max_size, '\n');
    } else if (m_ifs.is_open()) {
        /* file */
        m_ifs.ignore(max_size, '\n');
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
    } else if (m_is_stdin) {
        /* STDIN */
        return std::getline(std::cin, out) ? true : false;
    } else if (m_ifs.is_open()) {
        /* file */
        return std::getline(m_ifs, out) ? true : false;
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

