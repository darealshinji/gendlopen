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
#include "gendlopen.hpp"


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


namespace ast
{
    typedef enum {
        M_NONE,
        M_ALL,
        M_PREFIX,
        M_LIST
    } mode_t;

    typedef struct {
        bool is_func;
        std::string symbol;
        std::string type;
        std::string param_novars;
    } decl_t;
}


/* get function or variable declaration */
static ast::decl_t get_declarations(const std::string &line, ast::mode_t mode, const std::string &symbol, vstring_t &list)
{
    ast::decl_t decl;
    std::smatch m;

    const std::regex reg("^.*?"
        CFGREEN "(Function|Var)Decl" C0 ".*?"
        CFBLUE  " (.*?)" C0 " "  /* symbol */
        CGREEN  "'(.*?)'.*"      /* type */
    );

    if (!std::regex_match(line, m, reg) || m.size() != 4) {
        return {};
    }

    if (mode == ast::M_PREFIX) {
        if (!m[2].str().starts_with(symbol)) {
            return {};
        }
    } else if (mode == ast::M_LIST) {
        /* returns how many times the element was
         * found and erased from vector */
        if (std::erase(list, m[2]) == 0) {
            return {};
        }
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

/* parse Clang AST */
bool gendlopen::parse_ast(const std::string &ifile)
{
    std::string line;
    std::ifstream ifs;
    ast::mode_t mode = ast::M_ALL;

    /* getline for filestream or stdin */
    auto xgetline = [] (std::ifstream &ifs, std::string &line) -> bool {
        if (ifs.is_open()) {
            return (std::getline(ifs, line)) ? true : false;
        }
        return (std::getline(std::cin, line)) ? true : false;
    };

    if (ifile != "-") {
        /* open file for reading */
        ifs.open(ifile);

        if (!ifs.is_open()) {
            std::cerr << "error: failed to open file for reading: " << ifile << std::endl;
            return false;
        }
    }

    /* handle mode */
    if (!m_prefix.empty()) {
        mode = ast::M_PREFIX;
    } else if (!m_symbols.empty()) {
        mode = ast::M_LIST;
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
JMP1:
        if (line.empty()) {
            /* assume end of file */
            return true;
        }

        auto decl = get_declarations(line, mode, m_prefix, m_symbols);

        if (decl.symbol.empty()) {
            /* nothing found */
            continue;
        } else if (decl.is_func) {
            /* save values and continue to read params */
            std::string param;
            size_t count = 0;

            /* read next line(s) */
            while (xgetline(ifs, line) && get_parameters(line, param, count))
            {}

            if (param.ends_with(", ")) {
                param.erase(param.size()-2);
            }

            proto_t proto = { decl.type, decl.symbol, param, decl.param_novars };
            m_prototypes.push_back(proto);

            /* continue to analyze the current line in buffer */
            goto JMP1;
        } else {
            /* variable */
            obj_t obj = { decl.type, decl.symbol };
            m_objects.push_back(obj);
        }

        /* stop if the vector is empty */
        if (mode == ast::M_LIST && m_symbols.empty()) {
            return true;
        }
    }

    return true;
}

