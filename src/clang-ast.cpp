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

/* read prototypes from a Clang AST file */

#include <algorithm>
#include <iostream>
#include <fstream>
#include <regex>
#include <sstream>
#include <vector>
#include <stdio.h>
#include <string.h>

#include "clang-ast.h"
#include "cin_ifstream.hpp"
#include "gendlopen.hpp"


namespace /* anonymous */
{

enum {
    M_NONE,
    M_ALL,
    M_PREFIX,
    M_LIST
};

typedef struct decl {
    bool is_func;
    std::string symbol;
    std::string type;
    std::string param_novars;
} decl_t;


/* get function or variable declaration */
decl_t get_declarations(const std::string &line, int mode, const std::string &symbol, vstring_t &list)
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

    if (mode == M_PREFIX) {
        if (!m[2].str().starts_with(symbol)) {
            return {};
        }
    } else if (mode == M_LIST) {
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
bool get_parameters(const std::string &line, std::string &param, size_t &count)
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

} /* end anonymous namespace */


/* returns true if a function declaration was found */
bool gendlopen::clang_ast_line(cin_ifstream &ifs, std::string &line, int mode)
{
    decl_t decl = get_declarations(line, mode, m_prefix, m_symbols);

    if (decl.is_func) {
        /* function */
        std::string param;
        size_t count = 0;

        /* read next lines for parameters */
        while (ifs.getline(line) && get_parameters(line, param, count))
        {}

        if (param.ends_with(", ")) {
            param.erase(param.size()-2);
        }

        proto_t proto = { decl.type, decl.symbol, param, decl.param_novars };
        m_prototypes.push_back(proto);

        /* continue to analyze the current line stored in buffer */
        return true;
    } else if (!decl.symbol.empty()) {
        /* variable */
        proto_t obj = { decl.type, decl.symbol, {}, {} };
        m_objects.push_back(obj);
    }

    return false;
}

/* read Clang AST */
bool gendlopen::clang_ast(cin_ifstream &ifs, const std::string &ifile)
{
    /* nothing found? */
    auto check_empty = [&, this] () -> bool
    {
        if (m_prototypes.empty() && m_objects.empty()) {
            std::cerr << "error: no function or object prototypes found in file: "
                << ifile << std::endl;
            return false;
        }
        return true;
    };

    std::string line;
    int mode = M_ALL;

    /* handle mode */
    if (!m_prefix.empty()) {
        mode = M_PREFIX;
    } else if (!m_symbols.empty()) {
        mode = M_LIST;
    }

    /* read lines */
    while (ifs.getline(line))
    {
        bool loop = true;

        while (loop)
        {
            /* assume end of file */
            if (line.empty()) {
                return check_empty();
            }

            loop = clang_ast_line(ifs, line, mode);

            /* get_declarations() deletes found symbols,
             * so stop if the vector is empty */
            if (mode == M_LIST && m_symbols.empty()) {
                return check_empty();
            }
        }
    }

    return check_empty();
}
