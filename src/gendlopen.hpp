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
#include <vector>

#include "common.hpp"
#include "cin_ifstream.hpp"


class gendlopen
{
private:

    /* right now only C output supports this */
    inline bool separate_is_supported(const output::format fmt) const {
        return (fmt == output::c);
    }

    vstring_t m_definitions, m_includes, m_symbols;
    vproto_t m_prototypes, m_objects;
    std::string m_name_upper, m_name_lower, m_default_lib, m_prefix;
    output::format m_format = output::c;

    bool m_force = false;
    bool m_separate = false;
    bool m_skip_parameter_names = false;
    int *m_argc = NULL;
    char ***m_argv = NULL;

    /* clang-ast.cpp */
    bool clang_ast_line(cin_ifstream &ifs, std::string &line, int mode);
    bool clang_ast(cin_ifstream &ifs, const std::string &ifile);

    /* tokenize.cpp */
    bool tokenize(cin_ifstream &ifs, const std::string &ifile);
    void filter_and_copy_symbols(vproto_t &tmp_proto, vproto_t &tmp_objs);

    /* generate.cpp */
    bool tokenize_input(const std::string &ifile);

    /* parse.cpp */
    std::string parse(const std::string &data);

public:

    /* c'tor */
    gendlopen(int *argc, char ***argv) : m_argc(argc), m_argv(argv)
    {}

    /* d'tor */
    virtual ~gendlopen()
    {}

    /* set options */
    void format(output::format val) { m_format = val; }
    void default_lib(const std::string &s) { m_default_lib = s; }
    void prefix(const std::string &s) { m_prefix = s; }
    void force(bool b) { m_force = b; }
    void separate(bool b) { m_separate = b; }
    void skip_parameter_names(bool b) { m_skip_parameter_names = b; }

    /* add code */
    void add_def(const std::string &s) { m_definitions.push_back(s); }
    void add_inc(const std::string &s) { m_includes.push_back(s); }
    void add_sym(const std::string &s) { m_symbols.push_back(s); }

    /* generate output */
    int generate(const std::string &ifile, const std::string &ofile, const std::string &name);

};

#endif //_GENDLOPEN_HPP_

