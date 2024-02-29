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
#include <stdio.h>
#include <string.h>
#include "common.hpp"

#define COL(x)   "\x1B\\[" #x "m"

/* ANSI color codes used in the Clang AST output */
#define C0       COL(0)      /* default */
#define CGREEN   COL(0;32)   /* green */
#define CORANGE  COL(0;33)   /* orange */
#define CBLUE    COL(0;34)   /* blue */
#define CFGREEN  COL(0;1;32) /* fat green */
#define CFBLUE   COL(0;1;36) /* fat blue */

#ifdef NDEBUG
#define DEBUG_PRINT(x)  /**/
#else
#define DEBUG_PRINT(x)  std::cout << x << std::endl
#endif


typedef struct {
    bool is_func;
    std::string symbol;
    std::string type;
    std::string param_novars;
} FunctionVarDecl_t;

typedef struct {
    std::string param;
    std::string varsonly;
} ParmVarDecl_t;


/* get function or variable declaration */
FunctionVarDecl_t GetFunctionVarDecl(const std::string &line, const char *prefix, const char *symbol)
{
    const std::regex reg("^.*?"
        CFGREEN "(Function|Var)Decl" C0 ".*?"
        CFBLUE  " (.*?)" C0 " "  /* symbol */
        CGREEN  "'(.*?)'.*"      /* type */
    );

    std::smatch m;

    if (!std::regex_match(line, m, reg) || m.size() != 4) {
        return {};
    }

    if (prefix && !m[2].str().starts_with(prefix)) {
        return {};
    }

    if (symbol && m[2].str() != symbol) {
        return {};
    }

    FunctionVarDecl_t decl;
    decl.symbol = m[2];

    if (m[1].str().front() == 'F') {
        /* function declaration */
        const auto &s = m[3].str();
        const auto pos = s.find('(');

        if (pos == std::string::npos) {
            return {};
        }
        decl.is_func = true;
        decl.type = s.substr(0, pos);
        decl.param_novars = s.substr(pos + 1);
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
bool GetParmVarDecl(ParmVarDecl_t &decl, size_t &count)
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

    const auto &s = m[1].str();
    const std::string var = "arg" + std::to_string(++count);
    decl.varsonly += var + ", ";

    /* search for function pointer */
    const auto pos = s.find("(*)");

    if (pos == std::string::npos) {
        /* regular parameter */
        decl.param += s + var + ", ";
    } else {
        /* function pointer */
        std::string fptr = s;
        fptr.insert(pos + 2, var);
        decl.param += fptr + ", ";
    }

    return true;
}

int parse_clang_ast(const char *prefix, const char *symbol)
{
    std::string line;
    char c = 0;

    auto strip_end = [] (std::string &s)
    {
        if (s.ends_with(", ")) {
            s.erase(s.size()-2);
        }
    };

    if (prefix) {
        c = prefix[0];
    } else if (symbol) {
        c = symbol[0];
    }

    if (c != 0 && !common::range(c,'a','z') && !common::range(c,'A','Z') && c != '_') {
        std::cerr << "error: given symbol name starts with illegal ASCII character: "
            << c << std::endl;
        ::fclose(stdin);
        return 1;
    }

    while (std::getline(std::cin, line)) {
        auto decl = GetFunctionVarDecl(line, prefix, symbol);

        if (decl.symbol.empty()) {
            continue;
        } else if (decl.is_func) {
            ParmVarDecl_t decl2;
            size_t count = 0;

            if (!decl.param_novars.empty() &&
                !common::same_string_case(decl.param_novars, "void"))
            {
                while (GetParmVarDecl(decl2, count)) {}
                strip_end(decl2.param);
                strip_end(decl2.varsonly);
            }

            DEBUG_PRINT("\n//function "
                << decl.type << " | "
                << decl.symbol << " | ("
                << decl2.param << ") | ("
                << decl.param_novars << ") | ("
                << decl2.varsonly << ") ;");

            std::cout << decl.type << ' ' << decl.symbol
                << " (" << decl2.param << ");" << std::endl;
        } else {
            DEBUG_PRINT("\n//variable " << decl.type << " | " << decl.symbol << " ;");
            std::cout << decl.type << ' ' << decl.symbol << ";" << std::endl;
        }
    }

    return 0;
}

void print_help(const char *argv0)
{
    std::cerr << "Read clang AST data from standard input and create machine readable\n"
        "C prototypes writing to standard output.\n"
        "\n"
        "Usage:\n"
        "  " << argv0 << " --help\n"
        "  " << argv0 << " prefix PFX      search for symbols beginning with PFX\n"
        "  " << argv0 << " symbol SYM      search for the symbol SYM\n"
        "  " << argv0 << " all             search for all symbols\n"
        "\n"
        "examples:\n"
        "  clang -Xclang -ast-dump -fansi-escape-codes [...] HEADER-FILE | " << argv0 << " all\n"
        "  " << argv0 << " prefix mylibraryapi_ < AST-DUMP-FILE\n"
        << std::endl;
}


int main(int argc, char **argv)
{
    auto arg1eq = [argv] (const char *s) -> bool {
        return (::strcmp(argv[1], s) == 0);
    };

    if (argc == 2) {
        if (arg1eq("all")) {
            return parse_clang_ast(NULL, NULL);
        } else if (arg1eq("--help")) {
            print_help(argv[0]);
            return 0;
        }
    } else if (argc == 3) {
        if (arg1eq("prefix")) {
            return parse_clang_ast(argv[2], NULL);
        } else if (arg1eq("symbol")) {
            return parse_clang_ast(NULL, argv[2]);
        }
    }

    std::cerr << "error: incorrect or missing command line options\n"
        "try `" << argv[0] << " --help'" << std::endl;

    /* close a possibly open pipe input stream before exit */
    ::fclose(stdin);

    return 1;
}
