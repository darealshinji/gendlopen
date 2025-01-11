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

#ifndef GLOBAL_HPP
#define GLOBAL_HPP

#include <stdexcept>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <regex>
#include <string>
#include <vector>
#include <assert.h>
#include <string.h>

#include "types.hpp"
#include "utils.hpp"
#include "cio_ofstream.hpp"
#include "open_file.hpp"
#include "parse_args.hpp"
#include "gendlopen.hpp"


#ifdef _MSC_VER
template<typename T=char>
int strcasecmp(const T *a, const T *b) {
    return _stricmp(a, b);
}
#else
# include <strings.h>
#endif


/* command line option strings used in error messages */
#define OPT_SYMBOL_NAME     "-S"
#define OPT_SYMBOL_PREFIX   "-P"
#define OPT_AST_ALL_SYMBOLS "-ast-all-symbols"
#define OPT_SKIP_PARAM      "-param=skip"
#define OPT_CREATE_PARAM    "-param=create"


/* lex.yy.c */
enum {
    //MYLEX_EOF = 0,
    MYLEX_OK = 1,
    MYLEX_AST_BEGIN,
    MYLEX_AST_PARMVAR,
    MYLEX_OPTION,
    MYLEX_ID,
    MYLEX_OTHER,
    MYLEX_SEMICOLON,
    MYLEX_ERROR
};

extern "C" char *yytext;
extern "C" int mylex(FILE *fp);
extern "C" const char *mylex_lasterror();


/* help.cpp */
namespace help {
    void print(const char *prog);
    void print_full(const char *prog);
}


namespace parse
{
    enum { ID = '$' };

    /* parse.cpp */
    void append_strings(std::string &buf, const iter_t &it_beg, const iter_t &it_end);
    iter_t find_first_not_pointer_or_ident(vstring_t &v);
    bool is_function_pointer(vstring_t &v, const iter_t &i);
    bool is_function_pointer_no_name(vstring_t &v, const iter_t &i);
    bool is_function_with_parentheses(vstring_t &v, const iter_t &i);
    bool is_function(vstring_t &v, const iter_t &i);
    bool is_array(vstring_t &v, const iter_t &i);

    /* parameters.cpp */
    bool read_and_copy_names(proto_t &proto, param::names &parameter_names);
    bool create_names(proto_t &proto, std::string &msg);
}

#endif /* GLOBAL_HPP */
