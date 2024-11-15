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

    vstring_t m_includes, m_symbols, m_prefix, m_typedefs;
    vproto_t m_prototypes, m_objects;

    std::string m_ifile;
    std::string m_ofile = "-";

    std::string m_name = "gdo";
    std::string m_name_upper = "GDO";
    std::string m_name_lower = "gdo";
    std::string m_fmt_upper; // = "$1" + m_name_upper + '_';
    std::string m_fmt_lower; // = "$1" + m_name_lower + '_';
    std::string m_fmt_namespace; // = "$1" + m_name_lower + "::";

    std::string m_defines, m_custom_template;
    std::string m_deflib_a, m_deflib_w;
    std::string m_common_prefix;

    output::format m_format = output::c;
    param::names m_parameter_names = param::param_default;

    cio::ofstream m_ofs, m_ofs_body;

    bool m_force = false;
    bool m_separate = false;
    bool m_ast_all_symbols = false;
    bool m_print_symbols = false;
    bool m_read_options = true;
    bool m_print_date = true;

    /* clang-ast.cpp */
    bool clang_ast_line(FILE *fp, int mode);
    void clang_ast(FILE *fp);

    /* tokenize.cpp */
    void tokenize();
    void filter_and_copy_symbols(vproto_t &proto);
    void parse_options(const vstring_t &options);

    /* generate.cpp */
    void open_ofstream(const std::filesystem::path &opath, cio::ofstream &ofs);
    void read_custom_template();

    /* substitute.cpp */
    void substitute_prepare();
    void substitute_line(const char *line, bool &skip_code, cio::ofstream &ofs);
    void substitute(const cstrList_t &data, cio::ofstream &ofs);

public:

    /* c'tor */
    gendlopen();

    /* d'tor */
    ~gendlopen();

    /* set options */
    void input(const std::string &s) { m_ifile = s; }
    void output(const std::string &s) { m_ofile = s; }
    void format(output::format val) { m_format = val; }
    void parameter_names(param::names val) { m_parameter_names = val; }
    void custom_template(const std::string &s) { m_custom_template = s; }
    void force(bool b) { m_force = b; }
    void separate(bool b) { m_separate = b; }
    void ast_all_symbols(bool b) { m_ast_all_symbols = b; }
    void print_symbols(bool b) { m_print_symbols = b; }
    void read_options(bool b) { m_read_options = b; }
    void print_date(bool b) { m_print_date = b; }

    void name(const std::string &s);
    void default_lib(const std::string &lib_a, const std::string &lib_w);

    /* add code */
    void add_def(const std::string &s) { m_defines += s; }
    void add_inc(const std::string &s) { m_includes.push_back(s); }
    void add_pfx(const std::string &s) { m_prefix.push_back(s); }
    void add_sym(const std::string &s) { m_symbols.push_back(s); }

    /* generate output */
    void generate();

};

#endif /* GDO_GENDLOPEN_HPP */
