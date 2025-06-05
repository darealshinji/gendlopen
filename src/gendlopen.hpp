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

#include <stddef.h>
#include <stdexcept>
#include <string>
#include <vector>
#include "cio_ofstream.hpp"
#include "types.hpp"



namespace help
{
    void print(const char *prog);
    void print_full(const char *prog);
}


namespace save
{
    /* global output file stream; defaults to STDOUT */
    extern cio::ofstream ofs;

    /* creates the GDO_CHECK_SYMBOL_NAME() macro and saves it to save::ofs */
    size_t symbol_name_lookup(const std::string &pfx_upper, const vproto_t &v_prototypes, const vproto_t &v_objects);
}


namespace data
{
    /* save filename macros to save::ofs */
    size_t filename_macros(bool line_directive);

    /* save license header to save::ofs */
    size_t license(bool line_directive);

    /* concatenate templates and create template_t vector lists */
    void create_template_lists(vtemplate_t &header, vtemplate_t &body, output::format format, bool separate);
}


/* create option plus setter and getter methods */
#define OPT(TYPE, NAME, DEFVAL) \
    private: \
        TYPE m_##NAME = DEFVAL; \
    public: \
        void NAME(const TYPE &var) { m_##NAME = var; } \
        TYPE NAME() { return m_##NAME; }


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
    std::string m_fmt_upper, m_fmt_lower, m_fmt_standalone;

    std::string m_defines;
    std::string m_deflib_a, m_deflib_w;

    /* tokenize.cpp */
    void create_typedefs();

    /* clang-ast.cpp */
    bool get_declarations(int mode);
    void parse_clang_ast();

    /* parse.cpp */
    void parse(std::vector<vstring_t> &vec_tokens, const std::string &input_name);

    /* generate.cpp */
    void open_ofstream(const fs_path_t &opath);

    /* substitute.cpp */
    size_t replace_function_prototypes(const size_t &templ_lineno, const std::string &entry);
    size_t replace_object_prototypes(const size_t &templ_lineno, const std::string &entry);
    size_t replace_symbol_names(const size_t &templ_lineno, const std::string &entry);
    size_t substitute_line(const template_t &line, size_t &templ_lineno, bool &skip_code);
    size_t substitute(const vtemplate_t &data);

public:

    /* c'tor */
    gendlopen();

    /* d'tor */
    ~gendlopen();

    void tokenize();
    void print_symbols_to_stdout();
    void process_custom_template();
    void generate();

    /* get() / set() an option */

    /*   type            method name      default value */
    OPT( std::string,    input,           {}          )
    OPT( std::string,    output,          "-"         )
    OPT( output::format, format,          output::c   )
    OPT( param::names,   parameter_names, param::read )
    OPT( std::string,    custom_template, {}          )
    OPT( bool,           force,           false       )
    OPT( bool,           separate,        false       )
    OPT( bool,           ast_all_symbols, false       )
    OPT( bool,           print_symbols,   false       )
    OPT( bool,           read_options,    true        )
    OPT( bool,           print_date,      true        )
    OPT( bool,           line_directive,  false       )
    OPT( bool,           pragma_once,     true        )

    void add_pfx(const std::string &s) { m_prefix_list.push_back(s); }
    void add_sym(const std::string &s) { m_symbol_list.push_back(s); }

    /* gendlopen.cpp */
    void add_inc(const std::string &s);
    void add_def(const std::string &s);
    void prefix(const std::string &s);
    void default_lib(const std::string &s);
    void format(const std::string &s);
};

#undef SET

