/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2024-2026 Carsten Janssen

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


/* templates path environment variable */
#define TEMPLATES_ENV  "TEMPLATES"

/* default search path for external resources */
#ifndef DEFAULT_TEMPLATES_PATH
#define DEFAULT_TEMPLATES_PATH  "templates/"
#endif



namespace save
{
    /* global output file stream; defaults to STDOUT */
    extern cio::ofstream ofs;

    /* open output file stream for writing */
    void open_ofstream(const std::filesystem::path &opath, bool force);
}


namespace data
{
    /* load template into memory */
    void load_template(templates::name file);

    /* concatenate templates and create template_t vector lists */
    void create_template_lists(vtemplate_t &header, vtemplate_t &body, output::format format, bool separate);

    /* save all templates into current working directory */
    void dump_templates();
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

    class error : public std::runtime_error {
        public:
            error(const std::string &message) : std::runtime_error(message) {}
            virtual ~error() {}
    };

private:

    vstring_t m_includes, m_symbol_list, m_prefix_list, m_typedefs;
    vproto_t m_prototypes, m_objects;
    std::string m_defines;

    std::string m_pfx = "gdo"; /* can be mixed case, used to create header name on STDOUT */
    std::string m_pfx_upper = "GDO";
    std::string m_pfx_lower = "gdo";

    /* used by std::regex_replace() */
    std::string m_fmt_upper, m_fmt_lower, m_fmt_standalone;

    /* shared variable for line substitution */
    size_t m_substitute_lineno = 0;

    /* gendlopen.cpp */
    std::string replace_prefixes(const std::string &input);

    /* tokenize.cpp */
    void create_typedefs();

    /* clang_ast.cpp */
    bool get_declarations(int mode);
    void parse_clang_ast();

    /* parse.cpp */
    void parse(std::vector<vstring_t> &vec_tokens, const std::string &input_name);

    /* options.cpp */
    void parse_options(const vstring_t &options);

    /* data*.cpp */
    void load_template(templates::name file);
    void create_template_lists(vtemplate_t &header, vtemplate_t &body);
    void dump_templates();

    /* generate.cpp */
    size_t save_data(const template_t *list);

    /* substitute.cpp */
    size_t replace_function_prototypes(const std::string &entry);
    size_t replace_object_prototypes(const std::string &entry);
    size_t replace_symbol_names(const std::string &entry);
    size_t substitute_line(const template_t &line, bool &skip_code);
    size_t substitute(const vtemplate_t &data);

public:

    /* c'tor */
    gendlopen();

    /* d'tor */
    ~gendlopen();

    /* get() / set() an option */

    /*   type            method name      default value */
    OPT( std::string,    input,           {}          )
    OPT( std::string,    output,          "-"         )
    OPT( output::format, format,          output::c   )
    OPT( param::names,   parameter_names, param::read )
    OPT( std::string,    custom_template, {}          )
    OPT( std::string,    templates_path,  DEFAULT_TEMPLATES_PATH )
    OPT( std::string,    default_lib,     {}          )
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
    void format(const std::string &s);
    void print_symbols_to_stdout();
    void process_custom_template();
    void parameter_names(const char *str); /* set from string */

    /* generate.cpp */
    void generate();

    /* options.cpp */
    void parse_cmdline(const int &argc, char ** const &argv);

    /* tokenize.cpp */
    void tokenize();
};

#undef OPT

