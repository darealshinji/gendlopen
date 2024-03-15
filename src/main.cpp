/**
 * The MIT License (MIT)
 *
 * Copyright (C) 2023-2024 djcj@gmx.de
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
#include <sstream>
#include "args.hxx"
#include "common.hpp"
#include "gendlopen.hpp"

using StrValue = args::ValueFlag<std::string>;
using StrList = args::ValueFlagList<std::string>;
using Opt = args::Options;
using args::ArgumentParser;
using args::HelpFlag;
using args::Flag;
using common::replace_string;
using common::same_string_case;


[[noreturn]] static inline
void error_exit(char * const prog, const std::string &msg)
{
    std::cerr << msg << "\nTry `" << prog << " --help' for more information." << std::endl;
    std::exit(1);
}

static output::format str_to_enum(char * const prog, const std::string &fmt)
{
    if (same_string_case(fmt, "C++") ||
        same_string_case(fmt, "CPP") ||
        same_string_case(fmt, "CXX"))
    {
        return output::cxx;
    } else if (same_string_case(fmt, "C")) {
        return output::c;
    } else if (same_string_case(fmt, "minimal")) {
        return output::minimal;
    } else if (same_string_case(fmt, "minimal-C++") ||
        same_string_case(fmt, "minimal-CPP") ||
        same_string_case(fmt, "minimal-CXX"))
    {
        return output::minimal_cxx;
    }

    std::string s = "unknown output format: " + fmt;
    error_exit(prog, s);

    common::unreachable();
}

int main(int argc, char **argv)
{
    ArgumentParser args("Tool to generate library loading code");
/*
        "Tool to generate library loading code",
        "About the input file format:\n"
        "* all functions that should be loaded must be listed as modern C-style prototypes, ending on semi-colon (;)\n"
        "* argument names must be included\n"
        "* comments are ignored\n"
        "* line-breaks are treated like spaces\n"
        "* any other code will throw an error");
*/

    HelpFlag a_help(args, "",
        "show this help",
        {'h', "help"});

    StrValue a_input(args, "FILE",
        "input file (\"-\" = STDIN)",
        {'i', "input"},
        Opt::Single | Opt::Required);

    StrValue a_output(args, "FILE",
        "output file (default: STDOUT)",
        {'o', "output"},
        "-",
        Opt::Single);

    StrValue a_name(args, "STRING",
        "use STRING in names of functions, macros and namespaces",
        //"Custom string to be used as prefix in function and macro names or as "
        //"C++ namespace (default: gdo).",
        {'n', "name"},
        "GDO",
        Opt::Single);

    StrValue a_format(args, "STRING",
        "output format: c (default), c++, minimal, minimal-c++",
        //"Set output format: C, C++, minimal or minimal-C++ (default is C).",
        {'F', "format"},
        Opt::Single);

    StrValue a_default_lib(args, "STRING",
        "default library to load",
        //"Set a default library name to load. "
        //"Hint: use the macro QUOTE_STRING(...) to put quotes around the library name.",
        {"default-library"},
        Opt::Single);

    StrList a_include(args, "STRING",
        "header to include (can be used multiple times)",
        //"Header file to include (can be used multiple times) "
        //"Hint: use the macro QUOTE_STRING(...) to put quotes around the header name.",
        {'I', "include"});

    StrList a_define(args, "STRING",
        "define preprocessor macro (can be used multiple times)",
        //"Add preprocessor definitions to the output (can be used multiple times).",
        {'D', "define"});

    Flag a_separate(args, "",
        "save into header and body files",
        //"Save output into separate header and body files.",
        {'s', "separate"});

    Flag a_force(args, "",
        "overwrite existing files",
        //"Always overwrite existing files.",
        {'f', "force"});

    Flag a_skip_parameter_names(args, "",
        "skip parameter name lookup in function prototypes",
        //"Don't try to look for parameter names in function prototypes. "
        //"This will disable any kind of wrapped functions in the output.",
        {"skip-parameter-names"});

    Flag a_clang_ast(args, "",
        "input is Clang AST file",
        //"Treat input as a Clang abstract syntax tree file. "
        //"The input should include ANSI escape codes and can be created like this: "
        //"clang -Xclang -ast-dump -fansi-escape-codes [...] header-file.h",
        {"clang-ast"});

    StrValue a_prefix(args, "STRING",
        "use only symbols prefixed with STRING",
        {"prefix"});

    StrList a_symbol(args, "STRING",
        "list symbols to use (rules out `--prefix')",
        {'S', "symbol"});


    /* parse arguments */
    try {
        args.ParseCLI(argc, argv);
    }
    catch (const args::Help&) {
        std::cout << args;
        return 0;
    }
    catch (const args::Error &e) {
        error_exit(*argv, e.what());
    }
    catch (...) {
        error_exit(*argv, "an unknown error has occurred");
    }


    auto gdo = gendlopen(&argc, &argv);

    /* --format */
    if (a_format) {
        gdo.format(str_to_enum(*argv, a_format.Get()));
    }

    /* --default-library */
    if (a_default_lib) {
        gdo.default_lib(a_default_lib.Get());
    }

    /* --define */
    if (a_define) {
        for (const auto &e : args::get(a_define)) {
            gdo.add_def(e);
        }
    }

    /* --include */
    if (a_include) {
        for (const auto &e : args::get(a_include)) {
            gdo.add_inc(e);
        }
    }

    /* --prefix and --symbol */
    if (a_prefix && a_symbol) {
        error_exit(*argv, "cannot use --prefix and --symbol together");
    }

    /* --prefix */
    if (a_prefix) {
        gdo.prefix(a_prefix.Get());
    }

    /* --symbol */
    if (a_symbol) {
        for (const auto &e : args::get(a_symbol)) {
            gdo.add_sym(e);
        }
    }

    /* flags */
    gdo.force(a_force);
    gdo.separate(a_separate);
    gdo.skip_parameter_names(a_skip_parameter_names);
    gdo.clang_ast(a_clang_ast);

    /* --clang-ast */
    if (a_clang_ast) {
        error_exit(*argv, "not implemented yet");
    }

    return gdo.generate(a_input.Get(), a_output.Get(), a_name.Get());
}
