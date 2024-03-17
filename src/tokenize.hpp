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
#include "cin_ifstream.hpp"


class tokenize
{
private:

    vproto_t m_prototypes, m_objects;
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
    vproto_t &objects() { return m_objects; };
};

#endif //_TOKENIZE_HPP_
