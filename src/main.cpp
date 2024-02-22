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
using args::ArgumentParser;
using args::HelpFlag;
using args::Flag;
using Opt = args::Options;

using common::replace_string;
using common::same_string_case;


static inline
void error_exit(char *prog, const std::string &msg)
{
    std::cerr << msg << "\nTry `" << prog << " --help' for more information." << std::endl;
    std::exit(1);
}

static output::format str_to_enum(char *prog, const std::string &fmt)
{
    if (same_string_case(fmt, "C++")) {
        return output::cxx;
    } else if (same_string_case(fmt, "C")) {
        return output::c;
    } else if (same_string_case(fmt, "minimal")) {
        return output::minimal;
    }

    std::string s = "unknown output format: " + fmt;
    error_exit(prog, s);

    common::unreachable();
}

int main(int argc, char **argv)
{
    ArgumentParser args(
        "Tool to generate library loading code",
        "About the input file format:\n"
        "* all functions that should be loaded must be listed as modern C-style prototypes, ending on semi-colon (;)\n"
        "* argument names must be included\n"
        "* comments are ignored\n"
        "* line-breaks are treated like spaces\n"
        "* any other code will throw an error");

    HelpFlag a_help(args, "",
        "Display this help menu",
        {'h', "help"});

    StrValue a_input(args, "FILE",
        "Input file (use \"-\" for STDIN)",
        {'i', "input"},
        Opt::Single | Opt::Required);

    StrValue a_output(args, "FILE",
        "Set an output file path (default: STDOUT)",
        {'o', "output"},
        "-",
        Opt::Single);

    StrValue a_name(args, "STRING",
        "Custom string to be used as prefix in function and macro names or as "
        "C++ namespace (default: gdo)",
        {'n', "name"},
        "GDO",
        Opt::Single);

    StrValue a_format(args, "STRING",
        "Set output format: C, C++ or minimal (default is C)",
        {'F', "format"},
        Opt::Single);

    StrValue a_default_lib(args, "STRING",
        "Set a default library name to load; either set the name explicitly "
        "(i.e. libfoo.so.1) or use the format 'foo:1', which will create a "
        "system-agnostic filename macro",
        {"default-library"},
        Opt::Single);

    StrList a_include(args, "STRING",
        "Header file to include (can be used multiple times)",
        {'I', "include"});

    StrList a_define(args, "STRING",
        "Add preprocessor definitions to the output (can be used multiple times)",
        {'D', "define"});

    Flag a_separate(args, "",
        "Save output into separate header and body files",
        {'s', "separate"});

    Flag a_force(args, "",
        "Always overwrite existing files",
        {'f', "force"});

    Flag a_skip_parameter_names(args, "",
        "Don't try to look for parameter names in function prototypes;"
        " this will disable any kind of wrapped functions in the output",
        {"skip-parameter-names"});


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

    /* flags */
    gdo.force(a_force);
    gdo.separate(a_separate);
    gdo.skip_parameter_names(a_skip_parameter_names);

    /* --input --output --name */
    gdo.generate(a_input.Get(), a_output.Get(), a_name.Get());

    return 0;
}
