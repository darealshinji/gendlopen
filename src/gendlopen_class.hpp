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

#ifndef GDO_GENDLOPEN_CLASS_HPP
#define GDO_GENDLOPEN_CLASS_HPP


class gendlopen
{
private:

    int *m_argc = NULL;
    char ***m_argv = NULL;

    vstring_t m_includes, m_symbols, m_prefix;
    vproto_t m_prototypes, m_objects;

    std::string m_name_upper, m_name_lower;
    std::string m_ifile, m_defines, m_custom_template;
    std::string m_deflib_a, m_deflib_w;

    output::format m_format = output::c;

    bool m_force = false;
    bool m_separate = false;
    bool m_skip_parameter_names = false;
    bool m_ast_all_symbols = false;

    bool separate_is_supported() const {
        return (m_format == output::c || m_format == output::cxx);
    }

    bool output_is_c() const {
        return (m_format != output::cxx && m_format != output::minimal_cxx);
    }

    /* clang-ast.cpp */
    bool clang_ast_line(cio::ifstream &ifs, std::string &line, int mode);
    bool clang_ast(cio::ifstream &ifs);

    /* tokenize.cpp */
    bool tokenize(cio::ifstream &ifs);
    void filter_and_copy_symbols(vproto_t &tmp_proto, vproto_t &tmp_objs);

    /* generate.cpp */
    bool tokenize_input();
    int parse_custom_template(const std::string &ofile);

    /* parse.cpp */
    std::string parse(std::string &data);

public:

    /* c'tor */
    gendlopen(int *argc, char ***argv)
    : m_argc(argc), m_argv(argv)
    {}

    /* d'tor */
    ~gendlopen()
    {}

    /* set options */
    void format(output::format val) { m_format = val; }
    void custom_template(const std::string &s) { m_custom_template = s; }
    void force(bool b) { m_force = b; }
    void separate(bool b) { m_separate = b; }
    void skip_parameter_names(bool b) { m_skip_parameter_names = b; }
    void ast_all_symbols(bool b) { m_ast_all_symbols = b; }

    void default_lib(const std::string &lib_a, const std::string &lib_w) {
        assert(!lib_a.empty() && !lib_w.empty());
        m_deflib_a = lib_a;
        m_deflib_w = lib_w;
    }

    /* add code */
    void add_def(const std::string &s) { m_defines += s; }
    void add_inc(const std::string &s) { m_includes.push_back(s); }
    void add_pfx(const std::string &s) { m_prefix.push_back(s); }
    void add_sym(const std::string &s) { m_symbols.push_back(s); }

    /* generate output */
    int generate(const std::string ifile, const std::string ofile, const std::string name);

};

#endif /* GDO_GENDLOPEN_CLASS_HPP */
