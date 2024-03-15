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

#include <algorithm>
#include <iostream>
#include <fstream>
#include <regex>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <string.h>
#include "common.hpp"
#include "parse-clang-ast.hpp"

/* ANSI color codes used in the Clang AST output */

/* escaped variants for regex */
#define COL(x)    "\x1B\\[" #x "m"
#define C0        COL(0)        /* default */
#define CGREEN    COL(0;32)     /* green */
#define CFGREEN   COL(0;1;32)   /* fat green */
#define CFBLUE    COL(0;1;36)   /* fat blue */

/* unescaped variants for std::string */
#define sCOL(x)   "\x1B[" #x "m"
#define sC0       sCOL(0)       /* default */
#define sCORANGE  sCOL(0;33)    /* orange */
#define sCFGREEN  sCOL(0;1;32)  /* fat green */


typedef struct {
    bool is_func;
    std::string symbol;
    std::string type;
    std::string param_novars;
} decl_t;


namespace /* anonymous */
{
    /* check if a symbol begins with an illegal character */
    inline void check_begins_with_illegal_character(const std::string &sym)
    {
        if (!common::range(sym.front(), 'a', 'z') &&
            !common::range(sym.front(), 'A', 'Z') &&
            sym.front() != '_')
        {
            std::cerr << "error: given symbol name `" << sym
                << "' starts with illegal ASCII character!" << std::endl;
            std::exit(1);
        }
    }

    /* getline for filestream or stdin */
    inline bool xgetline(std::ifstream &ifs, std::string &line)
    {
        if (ifs.is_open()) {
            return (std::getline(ifs, line)) ? true : false;
        }
        return (std::getline(std::cin, line)) ? true : false;
    }
}

/* get function or variable declaration */
static decl_t get_declarations(const std::string &line, mode_t mode, const std::string &symbol, vstring_t &vec)
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
    case ast::M_PREFIX:
        if (!m[2].str().starts_with(symbol)) {
            return {};
        }
        break;

    case ast::M_SINGLE:
        if (m[2] != symbol) {
            return {};
        }
        break;

    case ast::M_LIST:
        /* returns how many times the element was
         * found and erased from vector */
        if (std::erase(vec, m[2]) == 0) {
            return {};
        }
        break;

    case ast::M_ALL:
        break;

    case ast::M_NONE:
        common::unreachable();
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
        decl.param_novars = m[3].str().substr(pos + 1);
        decl.param_novars.pop_back();
    } else {
        /* variable declaration */
        decl.is_func = false;
        decl.type = m[3];
    }

    common::strip_spaces(decl.type);

    return decl;
}

/* get function parameter declaration */
static bool get_parameters(const std::string &line, std::string &param, size_t &count)
{
    const std::regex reg("^.*?"
        CFGREEN "ParmVarDecl" C0 ".*?"
        CGREEN  "'(.*?)'.*"  /* type */
    );

    std::smatch m;
    std::string buf;

    if (!std::regex_match(line, m, reg) || m.size() != 2) {
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

/* turn comma-separated list into vector */
static void vectorize_list(vstring_t &vec, const std::string &symbols)
{
    std::string token;
    std::string copy = symbols;

    /* replace separators with space */
    for (char &c : copy) {
        if (c == ',') c = ' ';
    }

    std::istringstream iss(copy);

    while (iss >> token) {
        check_begins_with_illegal_character(token);
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

/* parse Clang AST */
bool ast::parse(const std::string &ifile, mode_t mode, const std::string &sym, vproto_t &vproto, vobj_t &vobj)
{
    std::string line;
    std::ifstream ifs;
    vstring_t vlist;

    if (ifile != "-") {
        /* open file for reading */
        ifs.open(ifile);

        if (!ifs.is_open()) {
            std::cerr << "error: failed to open file for reading: " << ifile << std::endl;
            return false;
        }
    }

    /* handle mode */
    switch (mode)
    {
    case M_PREFIX:
    case M_SINGLE:
        check_begins_with_illegal_character(sym);
        break;

    case M_LIST:
        vectorize_list(vlist, sym);
        break;

    case M_ALL:
        break;

    case M_NONE:
        common::unreachable();
        std::abort();
    }

    /* check first line */

    xgetline(ifs, line);

    if (line.empty()) {
        std::cerr << "error: empty line" << std::endl;
        return false;
    }

    const char* const first_line = sCFGREEN "TranslationUnitDecl" sC0 sCORANGE " 0x";

    if (!line.starts_with(first_line)) {
        std::cerr << "error: file is missing TranslationUnitDecl line" << std::endl;
        return false;
    }

    /* parse lines */
    while (xgetline(ifs, line))
    {
        bool loop = true;

        if (line.empty()) {
            std::cerr << "error: empty line" << std::endl;
            return false;
        }

        while (loop == true)
        {
            auto decl = get_declarations(line, mode, sym, vlist);

            if (decl.symbol.empty()) {
                /* nothing found */
                loop = false;
            } else if (decl.is_func) {
                /* function: get parameters */
                std::string param;
                size_t count = 0;

                while (xgetline(ifs, line) && get_parameters(line, param, count))
                {}

                if (param.ends_with(", ")) {
                    param.erase(param.size()-2);
                }

                proto_t proto = { decl.type, decl.symbol, param, decl.param_novars };
                vproto.emplace_back(proto);
            } else {
                /* variable */
                obj_t obj = { decl.type, decl.symbol };
                vobj.emplace_back(obj);
                loop = false;
            }

            /* stop if the vector is empty */
            if (mode == M_LIST && vlist.empty()) {
                return true;
            }
        }
    }

    /* inform if symbols were not found */
    if (mode == M_LIST && !vlist.empty()) {
        std::cerr << "warning: the following symbols were not found:";

        for (const auto &e : vlist) {
            std::cerr << ' ' << e;
        }
        std::cerr << std::endl;
    }

    return true;
}


static void print_help(const char *argv0)
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


int main(int argc, char **argv)
{
    vproto_t vproto;
    vobj_t vobj;
    std::string sym, file;
    ast::mode_t mode = ast::M_NONE;

    switch (argc)
    {
    case 2:
        if (::strcmp(argv[1], "all") == 0) {
            mode = ast::M_ALL;
        } else if (::strcmp(argv[1], "--help") == 0) {
            print_help(argv[0]);
            return 0;
        }
        break;

    case 3:
        if (::strcmp(argv[1], "prefix") == 0) {
            mode = ast::M_PREFIX;
        } else if (::strcmp(argv[1], "list") == 0) {
            mode = ::strchr(argv[2], ',') ? ast::M_LIST : ast::M_SINGLE;
        }
        sym = argv[2];
        break;

    default:
        break;
    }

    if (mode == ast::M_NONE) {
        std::cerr << "error: incorrect or missing command line options" << std::endl;
        std::cerr << "try `" << argv[0] << " --help'" << std::endl;
        return 1;
    }

    file = "-"; // stdin

    if (!ast::parse(file, mode, sym, vproto, vobj)) {
        return 1;
    }

    for (const auto &e : vproto) {
        std::cout << e.type << ' ' << e.symbol
            << '(' << e.args << ");  // " << e.notype_args << std::endl;
    }

    for (const auto &e : vobj) {
        std::cout << e.type << ' ' << e.symbol << ';' << std::endl;
    }

    return 0;
}

