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

#ifndef GDO_GENDLOPEN_HPP
#define GDO_GENDLOPEN_HPP

#define SET(NAME, TYPE)  void NAME(TYPE var) { m_##NAME = var; }


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

    std::string m_input;
    std::string m_output = "-";

    std::string m_pfx = "gdo"; /* can be mixed case, used to create header name on STDOUT */
    std::string m_pfx_upper = "GDO";
    std::string m_pfx_lower = "gdo";

    /* used by std::regex_replace() */
    std::string m_fmt_upper, m_fmt_lower, m_fmt_namespace;

    std::string m_defines, m_custom_template, m_common_prefix;
    std::string m_deflib_a, m_deflib_w;

    output::format m_format = output::c;
    param::names m_parameter_names = param::param_default;

    bool m_force = false;
    bool m_separate = false;
    bool m_ast_all_symbols = false;
    bool m_print_symbols = false;
    bool m_read_options = true;
    bool m_print_date = true;
    bool m_line_directive = true;

    /* clang-ast.cpp */
    bool get_declarations(FILE *fp, int mode);
    void clang_ast(FILE *fp);

    /* tokenize.cpp */
    void tokenize();
    void filter_and_copy_symbols(vproto_t &proto);
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
    int replace_function_prototypes(const int &line_number, const std::string &line, cio::ofstream &ofs);
    int replace_object_prototypes(const int &line_number, const std::string &line, cio::ofstream &ofs);
    int replace_symbol_names(const int &line_number, const std::string &line, cio::ofstream &ofs);
    int substitute_line(const template_t &line, int &line_number, bool &skip_code, cio::ofstream &ofs);
    int substitute(const vtemplate_t &data, cio::ofstream &ofs);

public:

    /* c'tor */
    gendlopen();

    /* d'tor */
    ~gendlopen();

    /* set options */
    SET(input, const std::string &)
    SET(output, const std::string &)
    SET(format, output::format)
    SET(parameter_names, param::names)
    SET(custom_template, const std::string&)
    SET(force, bool)
    SET(separate, bool)
    SET(ast_all_symbols, bool)
    SET(print_symbols, bool)
    SET(read_options, bool)
    SET(print_date, bool)
    SET(line_directive, bool)

    /* gendlopen.cpp */
    void prefix(const std::string &s);
    void default_lib(const std::string &lib_a, const std::string &lib_w);

    /* add code */
    void add_def(const std::string &s) { m_defines += s; }
    void add_inc(const std::string &s) { m_includes.push_back(s); }
    void add_pfx(const std::string &s) { m_prefix_list.push_back(s); }
    void add_sym(const std::string &s) { m_symbol_list.push_back(s); }

    /* generate output */
    void generate();

};

#undef SET

#endif /* GDO_GENDLOPEN_HPP */
