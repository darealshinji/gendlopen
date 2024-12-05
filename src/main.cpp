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
#include <list>
#include <regex>
#include <string.h>

#include "global.hpp"


/* anonymous */
namespace
{
    /* whether 'c' is an argument prefix */
    constexpr bool is_arg_prefix(const char &c)
    {
#ifdef _WIN32
        return (c == '-' || c == '/');
#else
        return (c == '-');
#endif
    }

    /* return string + constant string length */
    template<size_t N>
    constexpr parse_args::optstr_t str(char const (&in)[N])
    {
        return { in, N-1 };
    }

    /* -format=... */
    bool set_format(gendlopen &gdo, const char *prog, const char *opt)
    {
        output::format out = utils::format_enum(opt);

        if (out == output::error) {
            std::cerr << prog << "unknown output format: " << opt << std::endl;
            return false;
        }

        gdo.format(out);
        return true;
    }

    /* -param=... */
    bool set_parameter_names(gendlopen &gdo, const char *prog, const char *opt, const char &optpfx)
    {
        if (utils::eq_str_case(opt, "skip")) {
            gdo.parameter_names(param::skip);
            return true;
        } else if (utils::eq_str_case(opt, "create")) {
            gdo.parameter_names(param::create);
            return true;
        }

        std::cerr << prog << ": unknown argument for option '"
            << optpfx << "param': " << opt << std::endl;

        return false;
    }

    /* parse argv[] and set options in 'gdo' object */
    bool parse_arguments(gendlopen &gdo, const int &argc, char ** const &argv)
    {
        const char *prog = parse_args::get_prog_name(argv[0]);
        const char *input = NULL;
        const char *cur = NULL;
        const char *cur_save = NULL;
        const char *opt = NULL;
        int it = 1;

        /* get option argument */
        auto get_arg = [&] (const parse_args::optstr_t &str) {
            opt = parse_args::get_arg_len(str.string, str.size, argc, argv, it);
            return (opt != NULL);
        };

        /* verify option without an argument */
        auto get_noarg = [&] (const parse_args::optstr_t &str) {
            return parse_args::get_noarg_len(str.string, str.size, argv, it);
        };

        /* parse arguments */
        for ( ; it < argc; it++) {
            cur = argv[it];

            /* non-option argument --> input */
            if (!is_arg_prefix(*cur) || strcmp(cur, "-") == 0) {
                if (!input) {
                    input = cur;
                } else {
                    std::cerr << "warning: non-option argument ignored: " << cur << std::endl;
                }
                continue;
            }

            cur_save = cur;

            /* skip prefix */
            cur += parse_args::pfxlen;

            switch(*cur)
            {
            case '?':
                if ( get_noarg(str("?")) ) {
                    help::print(prog);
                    std::exit(0);
                }
                break;

            case 'h':
                if ( get_noarg(str("help")) ) {
                    help::print(prog);
                    std::exit(0);
                }
                break;

            case 'f':
                if ( get_arg(str("format")) ) {
                    if (!set_format(gdo, prog, opt)) {
                        return false;
                    }
                    continue;
                } else if ( get_noarg(str("force")) ) {
                    gdo.force(true);
                    continue;
                } else if ( get_noarg(str("full-help")) ) {
                    help::print_full(prog);
                    std::exit(0);
                }
                break;

            case 'o':
                if ( get_arg(str("o")) ) {
                    gdo.output(opt);
                    continue;
                }
                break;

            case 'n':
                if ( get_noarg(str("no-date")) ) {
                    gdo.print_date(false);
                    continue;
                }
                break;

            case 't':
                if ( get_arg(str("template")) ) {
                    gdo.custom_template(opt);
                    continue;
                }
                break;

            case 'l':
                if ( get_arg(str("library")) ) {
                    std::string lib_a, lib_w;
                    utils::format_libname(opt, lib_a, lib_w);
                    gdo.default_lib(lib_a, lib_w);
                    continue;
                } else if ( get_noarg(str("line")) ) {
                    gdo.line_directive(true);
                    continue;
                }
                break;

            case 'i':
                if ( get_arg(str("include")) ) {
                    gdo.add_inc(utils::format_inc(opt));
                    continue;
                } else if ( get_noarg(str("ignore-options")) ) {
                    gdo.read_options(false);
                    continue;
                }
                break;

            case 'd':
                if ( get_arg(str("define")) ) {
                    gdo.add_def(utils::format_def(opt));
                    continue;
                }
                break;

            case 'D':
                if ( get_arg(str("D")) ) {
                    gdo.add_def(utils::format_def(opt));
                    continue;
                }
                break;

            case 'P':
                if ( get_arg(str("P")) ) {
                    gdo.add_pfx(opt);
                    continue;
                }
                break;

            case 'p':
                if ( get_arg(str("prefix")) ) {
                    gdo.prefix(opt);
                    continue;
                } else if ( get_arg(str("param")) ) {
                    if (!set_parameter_names(gdo, prog, opt, *cur_save)) {
                        return false;
                    }
                    continue;
                } else if ( get_noarg(str("print-symbols")) ) {
                    gdo.print_symbols(true);
                    continue;
                }
                break;

            case 'S':
                if ( get_arg(str("S")) ) {
                    gdo.add_sym(opt);
                    continue;
                }
                break;

            case 's':
                if ( get_noarg(str("separate")) ) {
                    gdo.separate(true);
                    continue;
                }
                break;

            case 'a':
                if ( get_noarg(str("ast-all-symbols")) ) {
                    gdo.ast_all_symbols(true);
                    continue;
                }
                break;

            case '-':
                /* let's support a help option with two dashes too */
                if (strcmp(cur_save, "--help") == 0) {
                    help::print(prog);
                    std::exit(0);
                }
                break;

            default:
                break;
            }

            std::cerr << prog << ": unknown option: " << cur_save << std::endl;
            return false;
        }

        /* input is required */
        if (!input) {
            std::cerr << prog << ": input file required" << std::endl;
            return false;
        }

        gdo.input(input);

        return true;
    }

} /* anonymous namespace */


int main(int argc, char **argv)
{
    /* initialize class */
    gendlopen gdo;

    if (!parse_arguments(gdo, argc, argv)) {
        std::cerr << "Try `" << argv[0] << " -help' for more information." << std::endl;
        return 1;
    }

    /* generate output */
    try {
        gdo.generate();
    }
    catch (const gendlopen::error &e) {
        std::cerr << parse_args::get_prog_name(argv[0]) << ": error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
