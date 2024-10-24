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

#ifndef GDO_CIO_IFSTREAM_HPP
#define GDO_CIO_IFSTREAM_HPP

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>


namespace cio
{

/* wrapper class to enable reading input from
 * a file or std::cin using the same object */
class ifstream
{
private:

    std::string m_buf;
    std::ifstream m_ifs;
    bool m_is_stdin = false;
    size_t m_fsize = 0;

public:

    ifstream() {}
    virtual ~ifstream() {}

    bool open(const std::filesystem::path &path);
    bool open(const std::string &file);

    bool is_open() const;
    bool is_stdin() const;
    void close();

    bool get(char &c);
    int  peek();
    bool good() const;

    size_t file_size() const;

    void ignore(size_t n = 1);
    void ignore_line();

    bool getline(std::string &out);

    /* get a preview of the next line */
    bool peek_line(std::string &out);
};

} /* namespace cio */

#endif /* GDO_CIO_IFSTREAM_HPP */
