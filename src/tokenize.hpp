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

#ifndef _TOKENIZE_HPP_
#define _TOKENIZE_HPP_

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include "common.hpp"


/* wrapper class to enable reading input from
 * a file or std::cin using the same object */
class cin_ifstream
{
private:

    bool m_stdin = false;
    std::ifstream m_ifs;

public:

    cin_ifstream() {}

    ~cin_ifstream() {
        close();
    }

    /* read from std::cin if input is "-" */
    void open(const std::string &file)
    {
        if (file == "-") {
            m_stdin = true;
        } else {
            m_ifs.open(file.c_str());
        }
    }

    bool is_open() const {
        return m_stdin ? true : m_ifs.is_open();
    }

    void close() {
        if (m_ifs.is_open()) m_ifs.close();
    }

    std::istream& get(char &c) {
        return m_stdin ? std::cin.get(c) : m_ifs.get(c);
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


class tokenize
{
private:

    vproto_t m_prototypes;
    vobj_t m_objects;
    cin_ifstream m_ifs;
    bool m_skip_parameter_names = false;

    vstring_t read_input();
    bool tokenize_function(const std::string &s);
    bool tokenize_object(const std::string &s);

public:

    /* c'tor */
    tokenize()
    {}

    /* d'tor */
    virtual ~tokenize()
    {}

    /* tokenize input */
    bool tokenize_file(const std::string &ifile, bool skip_parameter_names);

    /* return prototype vectors */
    vproto_t &prototypes() { return m_prototypes; };
    vobj_t &objects() { return m_objects; };
};

#endif //_TOKENIZE_HPP_
