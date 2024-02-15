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

    std::string m_name_upper, m_name_lower, m_guard, m_default_lib;
    output::format m_out = output::c;
    bool m_force = false;
    bool m_separate = false;
    int *m_argc = NULL;
    char ***m_argv = NULL;

    bool open_fstream(std::ofstream &ofs, const std::string &ofile);
    std::string parse(const char *data, vproto_t &prototypes, vobj_t &objects);

    /* helper to put header guards around the data and save
     * them to the provided stream */
    template<typename T=std::ofstream>
    void put_header_guards(T &str, const char *header_data, const char *body_data,
        vproto_t &prototypes, vobj_t &objects)
    {
        str << license_data;
        str << "#ifndef " << m_guard << '\n';
        str << "#define " << m_guard << "\n\n";

        if (!m_default_lib.empty()) {
            str << "#ifndef " << m_name_upper << "DEFAULT_LIB\n";
            str << "#define " << m_name_upper << "DEFAULT_LIB " << m_default_lib << '\n';
            str << "#endif\n\n";
        }

        str << parse(header_data, prototypes, objects);
        str << parse(body_data, prototypes, objects);
        str << "#endif //" << m_guard << '\n';
    }

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
    void separate(bool b) { m_separate = b; }
    void force(bool b) { m_force = b; }

    /* generate output */
    void generate(const std::string &ifile, const std::string &ofile, const std::string &name);

};

#endif //_GENDLOPEN_HPP_

