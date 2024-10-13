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

/* read prototypes from a Clang AST file */

/***
For reference the AST from helloworld.h:

TranslationUnitDecl 0x5f2a1096a4b8 <<invalid sloc>> <invalid sloc>
<...>
|-VarDecl 0x5f2a109cb050 <line:35:1, col:35> col:35 helloworld_callback 'helloworld_cb_t':'void (*)(const char *)' extern
| `-VisibilityAttr 0x5f2a109cb0b8 <line:25:44, col:65> Default
|-FunctionDecl 0x5f2a109cb2c8 <col:28, line:38:40> line:11:29 helloworld_init 'helloworld *()'
| `-VisibilityAttr 0x5f2a109cb370 <line:25:44, col:65> Default
|-FunctionDecl 0x5f2a109cb510 <col:28, line:41:48> col:17 helloworld_hello 'void (helloworld *)'
| |-ParmVarDecl 0x5f2a109cb400 <col:34, col:46> col:46 hw 'helloworld *'
| `-VisibilityAttr 0x5f2a109cb5c0 <line:25:44, col:65> Default
|-FunctionDecl 0x5f2a109cb890 <col:28, line:42:86> col:17 helloworld_hello2 'void (helloworld *, void (*)(const char *))'
| |-ParmVarDecl 0x5f2a109cb650 <col:35, col:47> col:47 hw 'helloworld *'
| |-ParmVarDecl 0x5f2a109cb770 <col:51, col:85> col:58 helloworld_cb 'void (*)(const char *)'
| `-VisibilityAttr 0x5f2a109cb948 <line:25:44, col:65> Default
`-FunctionDecl 0x5f2a109cba68 <col:28, line:45:50> col:17 helloworld_release 'void (helloworld *)'
  |-ParmVarDecl 0x5f2a109cb9d8 <col:36, col:48> col:48 hw 'helloworld *'
  `-VisibilityAttr 0x5f2a109cbb18 <line:25:44, col:65> Default

***/

#include <stdexcept>
#include <algorithm>
#include <iostream>
#include <fstream>
#include <regex>
#include <vector>
#include <cstdio>
#include <cstring>

#include "gendlopen.hpp"


namespace /* anonymous */
{

enum {
    M_ALL,
    M_PREFIX,
    M_LIST,
    M_PFX_LIST
};

typedef struct decl {
    bool is_func;
    std::string symbol;
    std::string type;
    //std::string notype_args;
} decl_t;


/* get function or variable declaration */
bool get_declarations(decl_t &decl, std::string &line, int mode, const vstring_t &prefix, vstring_t &list)
{
    std::smatch m;

    const std::regex reg(
        "^[|`]-(Function|Var)Decl 0x.*?"
        " ([A-Za-z0-9_]*?) "  /* symbol */
        "'(.*?)'.*"           /* type */
    );

    utils::strip_ansi_colors(line);

    if (!std::regex_match(line, m, reg) || m.size() != 4) {
        return false;
    }

    decl.symbol = m[2];

    switch (mode)
    {
    case M_PREFIX:
        if (!utils::is_prefixed(decl.symbol, prefix)) {
            return false;
        }
        break;
    case M_LIST:
        if (std::erase(list, decl.symbol) == 0) {
            return false;
        }
        break;
    case M_PFX_LIST:
        if (!utils::is_prefixed(decl.symbol, prefix) && std::erase(list, decl.symbol) == 0) {
            return false;
        }
        break;
    default:
        break;
    }

    if (m[1].str().front() == 'F') {
        /* function declaration */
        auto pos = m[3].str().find('(');

        if (pos == std::string::npos) {
            return false;
        }
        decl.is_func = true;
        decl.type = m[3].str().substr(0, pos);
        //decl.notype_args = m[3].str().substr(pos + 1);
        //decl.notype_args.pop_back();
    } else {
        /* variable declaration */
        decl.is_func = false;
        decl.type = m[3];
    }

    utils::strip_spaces(decl.type);

    return true;
}

/* get function parameter declaration */
bool get_parameters(std::string &line, std::string &args, std::string &notype_args, char letter)
{
    const std::regex reg(
        "^.*?-ParmVarDecl 0x.*?"
        "'(.*?)'.*"  /* type */
    );

    std::smatch m;

    utils::strip_ansi_colors(line);

    if (!std::regex_match(line, m, reg) || m.size() != 2) {
        return false;
    }

    notype_args += letter;
    notype_args += ", ";

    /* search for function pointer */
    auto pos = m[1].str().find("(*)");

    if (pos == std::string::npos) {
        /* regular parameter */
        args += ((m[1].str() + ' ') + letter) + ", ";
    } else {
        /* function pointer */
        std::string s = m[1].str();
        s.insert(pos + 2, 1, letter);
        args += s + ", ";
    }

    return true;
}

} /* end anonymous namespace */


/* returns true if a function declaration was found */
bool gendlopen::clang_ast_line(std::string &line, int mode)
{
    decl_t decl;

    if (!get_declarations(decl, line, mode, m_prefix, m_symbols)) {
        return false;
    }

    if (decl.is_func) {
        /* function */
        std::string args, notype_args;
        char letter = 'a';

        /* read next lines for parameters */
        while (m_ifs.getline(line) && get_parameters(line, args, notype_args, letter)) {
            letter++;
        }

        utils::delete_suffix(args, ", ");
        utils::delete_suffix(notype_args, ", ");

        proto_t proto = { decl.type, decl.symbol, args, notype_args };
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
void gendlopen::clang_ast()
{
    std::string line;
    int mode = M_ALL;
    bool list = false;

    if (!m_prefix.empty() && !m_symbols.empty()) {
        mode = M_PFX_LIST;
        list = true;
    } else if (!m_prefix.empty()) {
        mode = M_PREFIX;
    } else if (!m_symbols.empty()) {
        mode = M_LIST;
        list = true;
    }

    /* read lines */
    while (m_ifs.getline(line) && !line.empty()) {
        /* inner loop to read parameters */
        while (clang_ast_line(line, mode) && !line.empty())
        {}

        /* get_declarations() deletes found symbols,
         * so stop if the vector is empty */
        if (list && m_symbols.empty()) {
            break;
        }
    }

    if (m_prototypes.empty() && m_objects.empty()) {
        throw error("no function or object prototypes found in file: " + m_ifile);
    }
}

