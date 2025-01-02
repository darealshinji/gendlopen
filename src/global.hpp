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
inline int strcasecmp(const char *a, const char *b) {
    return _stricmp(a, b);
}
#else
#include <strings.h>
#endif

/* lex.yy.c */
enum {
    //MYLEX_EOF = 0,
    MYLEX_OK = 1,
    MYLEX_AST_BEGIN,
    MYLEX_AST_PARMVAR,
    MYLEX_OPTION,
    MYLEX_TOKEN,
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

#endif /* GLOBAL_HPP */
