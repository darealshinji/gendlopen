/**
 * The MIT License (MIT)
 *
 * Copyright (C) 2023 djcj@gmx.de
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
#include "strcasecmp.h"
#include "args.hxx"
#include "gendlopen.hpp"

/* defined in C++23 */
#ifndef unreachable
    #ifdef __GNUC__
        #define unreachable() (__builtin_unreachable())
    #elif defined(_MSC_VER)
        #define unreachable() (__assume(0))
    #else
        #define unreachable() /**/
    #endif
#endif

using StrValue = args::ValueFlag<std::string>;
using args::ArgumentParser;
using args::HelpFlag;
using args::Flag;


static void error_exit(char **argv, const std::string &msg)
{
    std::cerr << msg << std::endl;
    std::cerr << "Try '" << argv[0] << " --help' for more information." << std::endl;
    std::exit(1);
}

static output::format str_to_enum(char **argv, const std::string &fmt)
{
    if (same_string_case(fmt, "C")) {
        return output::c;
    } else if (same_string_case(fmt, "C++")) {
        return output::cxx;
    } else if (same_string_case(fmt, "minimal")) {
        return output::minimal;
    }

    std::string s = "unknown output format: " + fmt;
    error_exit(argv, s);
    unreachable();
}

int main(int argc, char **argv)
{
    ArgumentParser args(
        "Tool to generate library loading code",
        "About the input file format:\n"
        "* all functions that should be loaded must be listed as modern C-style prototypes, ending on semi-colon (;)\n"
        "* function pointers MUST be typedef'd; the typedef can optionally be part of the input file\n"
        "* argument names must be included if you want to use auto-load features\n"
        "* comments are ignored\n"
        "* line-breaks are treated like spaces\n"
        "* any other code will throw an error");

    HelpFlag a_help(args, "",
        "Display this help menu",
        {'h', "help"});

    StrValue a_input(args, "FILE",
        "Input file (use \"-\" for STDIN)",
        {'i', "input"},
        args::Options::Required);

    StrValue a_output(args, "FILE",
        "Set an output file path (default: STDOUT)",
        {'o', "output"},
        "-");

    StrValue a_name(args, "STRING",
        "Custom string to be used as prefix in function and macro names or as "
        "C++ namespace (default: gdo)",
        {'n', "name"},
        "GDO");

    StrValue a_format(args, "STRING",
        "Set output format: C, C++ or minimal (default is C)",
        {'F', "format"});

    Flag a_separate(args, "",
        "Save output into separate header and body files",
        {'s', "separate"});

    Flag a_force(args, "",
        "Always overwrite existing files",
        {'f', "force"});

    try {
        args.ParseCLI(argc, argv);
    }
    catch (const args::Help&) {
        std::cout << args;
        return 0;
    }
    catch (const args::Error &e) {
        error_exit(argv, e.what());
    }

    auto gdo = gendlopen();

    if (a_format) {
        gdo.format(str_to_enum(argv, a_format.Get()));
    }
    gdo.separate(a_separate);
    gdo.force(a_force);
    gdo.generate(a_input.Get(), a_output.Get(), a_name.Get());

    return 0;
}
