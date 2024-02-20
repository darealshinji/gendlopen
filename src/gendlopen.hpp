/**
 * The MIT License (MIT)
 *
 * Copyright (C) 2023-2024 djcj@gmx.de
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

#ifndef _GENDLOPEN_HPP_
#define _GENDLOPEN_HPP_

#include <fstream>
#include <string>

#include "template.h"
#include "common.hpp"
#include "tokenize.hpp"


class gendlopen
{
private:

    std::string m_name_upper, m_name_lower, m_guard, m_default_lib, m_extra_code;
    output::format m_out = output::c;
    bool m_force = false;
    bool m_separate = false;
    bool m_skip_parameter_names = false;
    int *m_argc = NULL;
    char ***m_argv = NULL;

    bool open_fstream(std::ofstream &ofs, const std::string &ofile);
    std::string parse(const std::string &data, vproto_t &prototypes, vobj_t &objects);

public:

    /* c'tor */
    gendlopen(int *argc, char ***argv) : m_argc(argc), m_argv(argv)
    {}

    /* d'tor */
    virtual ~gendlopen()
    {}

    /* set options */
    void format(output::format val) { m_out = val; }
    void default_lib(const std::string &str) { m_default_lib = str; }
    void force(bool b) { m_force = b; }
    void separate(bool b) { m_separate = b; }
    void skip_parameter_names(bool b) { m_skip_parameter_names = b; }

    /* add extra code */
    void extra_code(const std::string &s) { m_extra_code += s; }

    /* generate output */
    void generate(const std::string &ifile, const std::string &ofile, const std::string &name);

};

#endif //_GENDLOPEN_HPP_

