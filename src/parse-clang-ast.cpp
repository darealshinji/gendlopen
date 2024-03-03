/**
 * The MIT License (MIT)
 *
 * Copyright (C) 2024 djcj@gmx.de
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

#include <iostream>
#include <regex>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include "common.hpp"

/* ANSI color codes used in the Clang AST output */

/* escaped variants for regex */
#define COL(x)    "\x1B\\[" #x "m"
#define C0        COL(0)        /* default */
#define CGREEN    COL(0;32)     /* green */
#define CFGREEN   COL(0;1;32)   /* fat green */
#define CFBLUE    COL(0;1;36)   /* fat blue */

/* unescaped variants for std::cout */
#define sCOL(x)   "\x1B[" #x "m"
#define sC0       sCOL(0)       /* default */
#define sCORANGE  sCOL(0;33)    /* orange */
#define sCFGREEN  sCOL(0;1;32)  /* fat green */


namespace ast
{

typedef enum {
    M_NONE,
    M_ALL,
    M_PREFIX,
    M_LIST,
    M_SINGLE
} mode_t;

typedef struct {
    bool is_func;
    std::string symbol;
    std::string type;
} decl_t;


/* check if a symbol begins with an illegal character */
inline void check_illegal_character(const char *sym)
{
    if (!common::range(sym[0], 'a', 'z') && !common::range(sym[0], 'A', 'Z') && sym[0] != '_') {
        std::cerr << "error: given symbol name `" << sym
            << "' starts with illegal ASCII character!" << std::endl;
        std::exit(1);
    }
}

/* get function or variable declaration */
decl_t get_declarations(const std::string &line, mode_t mode, const char *symbol, vstring_t &vec)
{
    decl_t decl;
    std::smatch m;

    const std::regex reg("^.*?"
        CFGREEN "(Function|Var)Decl" C0 ".*?"
        CFBLUE  " (.*?)" C0 " "  /* symbol */
        CGREEN  "'(.*?)'.*"      /* type */
    );

    if (!std::regex_match(line, m, reg) || m.size() != 4) {
        return {};
    }

    switch(mode)
    {
    case M_PREFIX:
        if (!m[2].str().starts_with(symbol)) {
            return {};
        }
        break;

    case M_SINGLE:
        if (m[2] != symbol) {
            return {};
        }
        break;

    case M_LIST:
        /* returns how many times the element was
         * found and erased from vector */
        if (std::erase(vec, m[2]) == 0) {
            return {};
        }
        break;

    default:
        break;
    }

    decl.symbol = m[2];

    if (m[1].str().front() == 'F') {
        /* function declaration */
        auto pos = m[3].str().find('(');

        if (pos == std::string::npos) {
            return {};
        }
        decl.is_func = true;
        decl.type = m[3].str().substr(0, pos);
        //decl.param_novars = m[3].str().substr(pos + 1);
        //decl.param_novars.pop_back();
    } else {
        /* variable declaration */
        decl.is_func = false;
        decl.type = m[3];
    }

    common::strip_spaces(decl.type);

    return decl;
}

/* get function parameter declaration */
bool get_parameters(std::string &param, size_t &count)
{
    const std::regex reg("^.*?"
        CFGREEN "ParmVarDecl" C0 ".*?"
        CGREEN  "'(.*?)'.*"  /* type */
    );

    std::smatch m;
    std::string buf;

    if (!std::getline(std::cin, buf) ||
        !std::regex_match(buf, m, reg) || m.size() != 2)
    {
        return false;
    }

    std::string var = "arg" + std::to_string(++count);

    /* search for function pointer */
    auto pos = m[1].str().find("(*)");

    if (pos == std::string::npos) {
        /* regular parameter */
        param += m[1].str() + ' ';
        param += var + ", ";
    } else {
        /* function pointer */
        buf = m[1].str();
        buf.insert(pos + 2, var);
        param += buf + ", ";
    }

    return true;
}

void parse(mode_t mode, const char *sym, vstring_t &vec)
{
    std::string line;

    auto err_exit = [] (const char *msg) {
        std::cerr << "error: " << msg << std::endl;
        std::exit(1);
    };

    /* check first line */

    if (!std::getline(std::cin, line)) {
        err_exit("std::getline()");
    }

    if (line.empty()) {
        err_exit("empty line");
    }

    const char* const first_line = sCFGREEN "TranslationUnitDecl" sC0 sCORANGE " 0x";

    if (!line.starts_with(first_line)) {
        err_exit("file is missing TranslationUnitDecl line");
    }

    /* parse lines */
    while (std::getline(std::cin, line))
    {
        if (line.empty()) {
            err_exit("empty line");
        }

        auto decl = get_declarations(line, mode, sym, vec);

        if (decl.symbol.empty()) {
            /* nothing found */
            continue;
        } else if (decl.is_func) {
            /* function */
            std::string param;
            size_t count = 0;

            while (get_parameters(param, count)) {}

            if (param.ends_with(", ")) {
                param.erase(param.size()-2);
            }

            std::cout << decl.type << ' ' << decl.symbol << " (" << param << ");" << std::endl;
        } else {
            /* variable */
            std::cout << decl.type << ' ' << decl.symbol << ";" << std::endl;
        }

        /* stop if the vector is empty */
        if (mode == M_LIST && vec.empty()) {
            return;
        }
    }

    /* inform if symbols were not found */
    if (mode == M_LIST && !vec.empty()) {
        std::cerr << "warning: the following symbols were not found:";

        for (const auto &e : vec) {
            std::cerr << ' ' << e;
        }
        std::cerr << std::endl;
    }
}

/* turn comma-separated list into vector */
void vectorize_list(vstring_t &vec, const char *symbols)
{
    std::string token;
    std::string str = symbols;

    /* replace separators with space */
    for (char &c : str) {
        if (c == ',') c = ' ';
    }

    std::istringstream iss(str);

    while (iss >> token) {
        check_illegal_character(token.c_str());
        vec.push_back(token);
    }

    if (vec.empty()) {
        std::cerr << "error: empty list provided: " << symbols << std::endl;
        std::exit(1);
    }

    /* sort and remove duplicates */
    std::sort(vec.begin(), vec.end());
    vec.erase(std::unique(vec.begin(), vec.end()), vec.end());
}

} /* namespace ast */

namespace help
{
    void print(const char *argv0)
    {
        std::cerr << "Read clang AST data from standard input and create machine readable\n"
            "C prototypes writing to standard output.\n"
            "\n"
            "Usage:\n"
            "  " << argv0 << " --help\n"
            "  " << argv0 << " all                   get all symbols\n"
            "  " << argv0 << " prefix PFX            get all symbols that begin with PFX\n"
            "  " << argv0 << " list SYM1[,SYM2,...]  search for one or more symbols in a comma-separated list\n"
            "\n"
            "examples:\n"
            "  clang -Xclang -ast-dump -fansi-escape-codes [...] HEADER-FILE | " << argv0 << " all\n"
            "  " << argv0 << " prefix mylibraryapi_ < AST-DUMP-FILE\n"
            << std::endl;
    }
}

int main(int argc, char **argv)
{
    vstring_t vec;
    const char *sym = NULL;
    ast::mode_t mode = ast::M_NONE;

    switch (argc)
    {
    case 2:
        if (::strcmp(argv[1], "all") == 0) {
            mode = ast::M_ALL;
        } else if (::strcmp(argv[1], "--help") == 0) {
            help::print(argv[0]);
            return 0;
        }
        break;

    case 3:
        sym = argv[2];

        if (::strcmp(argv[1], "prefix") == 0) {
            mode = ast::M_PREFIX;
        } else if (::strcmp(argv[1], "list") == 0) {
            mode = ::strchr(sym, ',') ? ast::M_LIST : ast::M_SINGLE;
        }
        break;

    default:
        break;
    }

    switch (mode)
    {
    case ast::M_PREFIX:
    case ast::M_SINGLE:
        ast::check_illegal_character(sym);
        break;

    case ast::M_LIST:
        ast::vectorize_list(vec, sym);
        break;

    case ast::M_ALL:
        break;

    case ast::M_NONE:
        std::cerr << "error: incorrect or missing command line options" << std::endl;
        std::cerr << "try `" << argv[0] << " --help'" << std::endl;
        return 1;
    }

    ast::parse(mode, sym, vec);

    return 0;
}
