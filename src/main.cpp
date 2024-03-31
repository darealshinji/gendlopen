/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2023-2024 Carsten Janssen
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
#include <cstdlib>
#include "args.hxx"
#include "gendlopen.hpp"

using StrValue = args::ValueFlag<std::string>;
using StrList = args::ValueFlagList<std::string>;
using Opt = args::Options;
using args::ArgumentParser;
using args::HelpFlag;
using args::Flag;


/* anonymous */
namespace
{
    inline void print_info(char * const argv0) {
        std::cerr << "Try `" << argv0 << " --help' for more information." << std::endl;
    }

    std::string quote_lib(const std::string &lib)
    {
        if (lib.front() == '"' && lib.back() == '"') {
            return lib;
        }

        return "\"" + (lib + "\"");
    }

    std::string quote_inc(const std::string &inc)
    {
        if ((inc.front() == '<' && inc.back() == '>') ||
            (inc.front() == '"' && inc.back() == '"'))
        {
            return inc;
        }

        return "\"" + (inc + "\"");
    }

    output::format str_to_enum(char * const argv0, const std::string &fmt)
    {
        switch (fmt.front())
        {
        case 'c':
        case 'C':
        case 'm':
        case 'M':
            if (utils::eq_str_case(fmt, "C++") ||
                utils::eq_str_case(fmt, "CPP") ||
                utils::eq_str_case(fmt, "CXX"))
            {
                return output::cxx;
            } else if (utils::eq_str_case(fmt, "C")) {
                return output::c;
            } else if (utils::eq_str_case(fmt, "minimal")) {
                return output::minimal;
            } else if (utils::eq_str_case(fmt, "minimal-C++") ||
                utils::eq_str_case(fmt, "minimal-CPP") ||
                utils::eq_str_case(fmt, "minimal-CXX"))
            {
                return output::minimal_cxx;
            }
            break;
        default:
            break;
        }

        std::cerr << "error: unknown output format given: " << fmt << std::endl;
        print_info(argv0);
        std::exit(1);

        utils::unreachable();
    }

} /* anonymous namespace */


int main(int argc, char **argv)
{
    std::string more_help;

    /* gendlopen --more-help | wc -c */
    more_help.reserve(4500);

    ArgumentParser args("Tool to generate library loading code");

//  more_help += "<<--------------------------------- 80 chars --------------------------------->>" "\n";
    more_help +=                                                                                    "\n";
    more_help += "  Gendlopen is a small tool intended to help with the creation of code that"      "\n";
    more_help += "  dynamically loads shared libraries. It takes text files with C prototype"       "\n";
    more_help += "  declarations as input and creates C or C++ header files as output."             "\n";
    more_help +=                                                                                    "\n";
    more_help += "  The output is compatible with POSIX dlopen() and win32 LoadLibrary() API."      "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";
    more_help += "OPTIONS:"                                                                         "\n";
    more_help +=                                                                                    "\n";


    /* --help */

    HelpFlag a_help(args, {}, "show this help", {'h', "help"});
    HelpFlag a_help2(args, {}, {}, {'?'}, Opt::Hidden);

    more_help +=                                                                                    "\n";
    more_help += "  -h, --help"                                                                     "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Print a summary of all avaiable options with a brief description."            "\n";
    more_help +=                                                                                    "\n";


    /* --more-help */

    HelpFlag a_more_help(args, {},
        "show full help with additional info",
        {"more-help"});

    more_help +=                                                                                    "\n";
    more_help += "  --more-help"                                                                    "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Show this more detailed information."                                         "\n";
    more_help +=                                                                                    "\n";


    /* --input */

    StrValue a_input(args, "FILE",
        "input file",
        {'i', "input"},
        Opt::Single | Opt::Required);

    more_help +=                                                                                    "\n";
    more_help += "  -i[FILE], --input=[FILE]"                                                       "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Specify an input file. Use `-' to read data from STDIN."                      "\n";
    more_help += "    This flag is required unless `--help' or `--more-help' was passed."           "\n";
    more_help +=                                                                                    "\n";
    more_help += "    The input text must contain all symbols that should be loaded. They must be"  "\n";
    more_help += "    listed as modern C-style prototypes, ending on semi-colon (;). Comments are"  "\n";
    more_help += "    ignored, line-breaks are treated as spaces. Any other code will throw an"     "\n";
    more_help += "    error."                                                                       "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Alternatively the input may be an Abstract Syntax Tree (AST) generated by"    "\n";
    more_help += "    Clang. The AST must include ANSI escape codes - this is the default on most"  "\n";
    more_help += "    systems, otherwise add `-fansi-escape-codes' to clang's flags."               "\n";
    more_help += "    To dump the AST created from `foo.h' you can run the following command:"      "\n";
    more_help += "    clang -Xclang -ast-dump -fansi-escape-codes foo.h > ast.txt"                  "\n";
    more_help +=                                                                                    "\n";


    /* --output */

    StrValue a_output(args, "FILE",
        "output file",
        {'o', "output"},
        "-", Opt::Single);

    more_help +=                                                                                    "\n";
    more_help += "  -o[FILE], --output=[FILE]"                                                      "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Specify an output file. If this flag isn't set or if FILE is `-' output will" "\n";
    more_help += "    be printed to STDOUT."                                                        "\n";
    more_help +=                                                                                    "\n";


    /* --name */

    StrValue a_name(args, "STRING",
        "use STRING in names of functions, macros and namespaces (default: gdo)",
        {'n', "name"},
        "gdo", Opt::Single);

    more_help +=                                                                                    "\n";
    more_help += "  -n[STRING], --name=[STRING]"                                                    "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Use STRING as a prefix in names of functions and macros or as C++ namespace"  "\n";
    more_help += "    when generating output to avoid symbol clashes. The default string is `gdo'." "\n";
    more_help += "    Upper/lower case and underscores will be set accordingly."                    "\n";
    more_help +=                                                                                    "\n";


    /* --format */

    StrValue a_format(args, "STRING",
        "output format: c (default), c++, minimal, minimal-c++",
        {'F', "format"},
        Opt::Single);

    more_help +=                                                                                    "\n";
    more_help += "  -F[STRING], --format=[STRING]"                                                  "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Use one of the following output formats:"                                     "\n";
    more_help += "    C            -  many features, this is the default"                           "\n";
    more_help += "    C++          -  many features but no exception handling (due to the design)"  "\n";
    more_help += "    minimal      -  small C header"                                               "\n";
    more_help += "    minimal-C++  -  small C++ header with exception handling"                     "\n";
    more_help +=                                                                                    "\n";
    more_help += "    More information can be found in the comments of the header files."           "\n";
    more_help +=                                                                                    "\n";


    /* --library */
    StrValue a_library(args, "STRING",
        "default library to load",
        {'l', "library"},
        Opt::Single);

    more_help +=                                                                                    "\n";
    more_help += "  -l[STRING], --library=[STRING]"                                                 "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Set a default library name to load. Quotation marks are put around the"       "\n";
    more_help += "    filename if it's not already enclosed in quotation marks. This flag cannot"   "\n";
    more_help += "    cannot be combined with `--library-nq'."                                      "\n";
    more_help +=                                                                                    "\n";


    /* --library-nq */
    StrValue a_library_nq(args, "STRING",
        "default library to load (no quotes added)",
        {"library-nq"},
        Opt::Single);

    more_help +=                                                                                    "\n";
    more_help += "  --library-nq=[STRING]"                                                          "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Set a default library name to load. Quotation marks are never added. This"    "\n";
    more_help += "    flag cannot cannot be combined with `--library'."                             "\n";
    more_help +=                                                                                    "\n";


    /* --include */
    StrList a_include(args, "STRING",
        "header to include",
        {'I', "include"});

    more_help +=                                                                                    "\n";
    more_help += "  -I[STRING...], --include=[STRING...]"                                           "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Set a header file name to be included at the top of the output code."         "\n";
    more_help += "    Quotation marks are put around the filename if it's not enclosed in brackets" "\n";
    more_help += "    or quotation marks. This flag may be passed multiple times."                  "\n";
    more_help +=                                                                                    "\n";


    /* --include-no-quotes */
    StrList a_include_nq(args, "STRING",
        "header to include (no quotes added)",
        {"include-nq"});

    more_help +=                                                                                    "\n";
    more_help += "  --include-nq=[STRING...]"                                                       "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Set a header file name to be included at the top of the output code."         "\n";
    more_help += "    Quotation marks are never added. This flag may be passed multiple times."     "\n";
    more_help +=                                                                                    "\n";


    /* --define */
    StrList a_define(args, "STRING",
        "define preprocessor macro",
        {'D', "define"});

    more_help +=                                                                                    "\n";
    more_help += "  -D[STRING...], --define=[STRING...]"                                            "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Set a preprocessor definition macro to be added at the top of the output"     "\n";
    more_help += "    code. This macro may include a value in the form of `FOO=1'. This flag may"   "\n";
    more_help += "    be passed multiple times."                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --separate */
    Flag a_separate(args, {},
        "save into header and body files",
        {'s', "separate"});

    more_help +=                                                                                    "\n";
    more_help += "  -s, --separate"                                                                 "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Save output into separate body and header files. The filename extensions"     "\n";
    more_help += "    will be set to .c/.h or .cpp/.hpp accordingly. This flag is ignored if the"   "\n";
    more_help += "    output is printed to STDOUT. Currently this is only supported if the output"  "\n";
    more_help += "    format is `C'. On other formats this flag is ignored."                        "\n";
    more_help +=                                                                                    "\n";


    /* --force */
    Flag a_force(args, {},
        "overwrite existing files",
        {'f', "force"});

    more_help +=                                                                                    "\n";
    more_help += "  -f, --force"                                                                    "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Always overwrite existing output files. Use with care."                       "\n";
    more_help +=                                                                                    "\n";


    /* --skip-parameter-names */
    Flag a_skip_parameter_names(args, {},
        "skip parameter name lookup in function prototypes",
        {"skip-parameter-names"});

    more_help +=                                                                                    "\n";
    more_help += "  --skip-parameter-names"                                                         "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Don't try to look for parameter names in function prototypes when the input"  "\n";
    more_help += "    is being processed. This will disable any kind of wrapped functions in the"   "\n";
    more_help += "    output (if the selected output format makes use of them)."                    "\n";
    more_help +=                                                                                    "\n";


    /* --prefix */
    StrList a_prefix(args, "STRING",
        "look for symbols prefixed with STRING",
        {'P', "prefix"});

    more_help +=                                                                                    "\n";
    more_help += "  -P[STRING...], --prefix=[STRING]"                                               "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Look for symbols that begin with STRING when parsing the input. This is most" "\n";
    more_help += "    useful if the input is a Clang AST to ignore unwanted declarations coming"    "\n";
    more_help += "    from i.e. standard C headers."                                                "\n";
    more_help +=                                                                                    "\n";


    /* --symbol */
    StrList a_symbol(args, "STRING",
        "look for symbol STRING",
        {'S', "symbol"});

    more_help +=                                                                                    "\n";
    more_help += "  -S[STRING...], --symbol=[STRING...]"                                            "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Look for the symbol name STRING when parsing the input. This is most useful"  "\n";
    more_help += "    useful if the input is a Clang AST to ignore unwanted declarations coming"    "\n";
    more_help += "    from i.e. standard C headers. This flag may be passed multiple times."        "\n";
    more_help +=                                                                                    "\n";


    /* --ast-all-symbols */
    Flag a_ast_all_symbols(args, {},
        "use all symbols from a Clang AST",
        {"ast-all-symbols"});

    more_help +=                                                                                    "\n";
    more_help += "  --ast-all-symbols"                                                              "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Pass this flag if you really want to use all symbols found in a Clang AST."   "\n";
    more_help += "    Be careful, as this might include unwanted prototypes from other headers."    "\n";
    more_help += "    It's recommended to use `--prefix' and/or `--symbol' instead. This flag "     "\n";
    more_help += "    cannot be combined with `--prefix' or `--symbol'."                            "\n";
    more_help +=                                                                                    "\n";
    more_help += "    This flag has no effect if the input isn't a Clang AST."                      "\n";
    more_help +=                                                                                    "\n";


    /* parse arguments */
    try {
        args.ParseCLI(argc, argv);
    }
    catch (const args::Help&) {
        if (a_more_help) {
            std::cout << "  " << *argv << " {OPTIONS}\n\n";
            std::cout << more_help << std::flush;
        } else {
            std::cout << args << std::flush;
        }
        return 0;
    }
    catch (const args::Error &e) {
        std::cerr << "error: " << e.what() << std::endl;
        print_info(*argv);
        return 1;
    }
    catch (...) {
        std::cerr << "an unknown error has occurred" << std::endl;
        return 1;
    }


    /* check excluding flags */

    if (a_library && a_library_nq) {
        std::cerr << "error: cannot combine `--library' and `--library-nq'" << std::endl;
        print_info(*argv);
        return 1;
    }

    if (a_ast_all_symbols && (a_symbol || a_prefix)) {
        std::cerr << "error: cannot combine `--ast-all-symbols' with `--symbol' or `--prefix'" << std::endl;
        print_info(*argv);
        return 1;
    }


    auto gdo = gendlopen(&argc, &argv);

    /* --input (flagged as required) */
    gdo.input(a_input.Get());

    /* --format */
    if (a_format) {
        gdo.format(str_to_enum(*argv, a_format.Get()));
    }

    /* --library(-no-quotes) */
    if (a_library) {
        gdo.default_lib(quote_lib(a_library.Get()));
    } else if(a_library_nq) {
        gdo.default_lib(a_library_nq.Get());
    }

    /* --define */
    for (const auto &e : args::get(a_define)) {
        gdo.add_def(e);
    }

    /* --include */
    for (const auto &e : args::get(a_include)) {
        gdo.add_inc(quote_inc(e));
    }

    /* --include-no-quotes */
    for (const auto &e : args::get(a_include_nq)) {
        gdo.add_inc(e);
    }

    /* --prefix */
    for (const auto &e : args::get(a_prefix)) {
        gdo.add_pfx(e);
    }

    /* --symbol */
    for (const auto &e : args::get(a_symbol)) {
        gdo.add_sym(e);
    }

    /* other flags */
    gdo.force(a_force);
    gdo.separate(a_separate);
    gdo.skip_parameter_names(a_skip_parameter_names);
    gdo.ast_all_symbols(a_ast_all_symbols);

    /* generate output */
    return gdo.generate(a_output.Get(), a_name.Get());
}
