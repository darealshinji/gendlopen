/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2024-2025 Carsten Janssen

 Permission is hereby  granted, free of charge, to any  person obtaining a copy
 of this software and associated  documentation files (the "Software"), to deal
 in the Software  without restriction, including without  limitation the rights
 to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
 copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
 IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
 FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
 AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
 LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
**/

#pragma once

#include <stdio.h>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <vector>
#include "types.hpp"

namespace cio { class ofstream; }


#define SET(TYPE, NAME, DEFVAL) \
    private: \
        TYPE m_##NAME = DEFVAL; \
    public: \
        void NAME(const TYPE &var) { m_##NAME = var; }


class gendlopen
{
public:

    class error : public std::runtime_error
    {
        public:
            error(const std::string &message) : std::runtime_error(message) {}
            virtual ~error() {}
    };

private:

    vstring_t m_includes, m_symbol_list, m_prefix_list, m_typedefs;
    vproto_t m_prototypes, m_objects;

    std::string m_pfx = "gdo"; /* can be mixed case, used to create header name on STDOUT */
    std::string m_pfx_upper = "GDO";
    std::string m_pfx_lower = "gdo";

    /* used by std::regex_replace() */
    std::string m_fmt_upper, m_fmt_lower, m_fmt_namespace;

    std::string m_defines;
    std::string m_deflib_a, m_deflib_w;

    /* clang-ast.cpp */
    bool get_declarations(FILE *fp, int mode);
    void clang_ast(FILE *fp);

    /* tokenize.cpp */
    void tokenize();

    /* parse.cpp */
    void parse(std::vector<vstring_t> &vec_tokens, vstring_t &options, vproto_t &vproto, std::string &input_name);
    void filter_and_copy_symbols(vproto_t &proto);

    /* parse_options.cpp */
    void parse_options(const vstring_t &options);

    /* data.cpp */
    int save_filename_macros_data(cio::ofstream &ofs);
    int save_license_data(cio::ofstream &ofs);
    void create_template_data_lists(vtemplate_t &header, vtemplate_t &body);

    /* gendlopen.cpp */
    void create_typedefs();
    void get_common_prefix();

    /* generate.cpp */
    void open_ofstream(const std::filesystem::path &opath, cio::ofstream &ofs);
    void read_custom_template();

    /* substitute.cpp */
    int replace_function_prototypes(const int &templ_lineno, const std::string &entry, cio::ofstream &ofs);
    int replace_object_prototypes(const int &templ_lineno, const std::string &entry, cio::ofstream &ofs);
    int replace_symbol_names(const int &templ_lineno, const std::string &entry, cio::ofstream &ofs);
    int substitute_line(const template_t &line, int &templ_lineno, bool &skip_code, cio::ofstream &ofs);
    int substitute(const vtemplate_t &data, cio::ofstream &ofs);

public:

    /* c'tor */
    gendlopen();

    /* d'tor */
    ~gendlopen();

    /* set options */
    SET(std::string, input, {})
    SET(std::string, output, "-")
    SET(output::format, format, output::c)
    SET(param::names, parameter_names, param::param_default)
    SET(std::string, custom_template, {})
    SET(bool, force, false)
    SET(bool, separate, false)
    SET(bool, ast_all_symbols, false)
    SET(bool, print_symbols, false)
    SET(bool, read_options, true)
    SET(bool, print_date, true)
    SET(bool, line_directive, false)
    SET(bool, pragma_once, true)

    /* gendlopen.cpp */
    void prefix(const std::string &s);
    void default_lib(const std::string &s);
    void format(const std::string &s);
    void add_inc(const std::string &s);
    void add_def(const std::string &s);

    /* add code */
    void add_pfx(const std::string &s) { m_prefix_list.push_back(s); }
    void add_sym(const std::string &s) { m_symbol_list.push_back(s); }

    /* generate output */
    void generate();

};

#undef SET

