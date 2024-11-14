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
    bool parse_arguments(gendlopen &gdo, int argc, char **argv)
    {
        auto is_arg_prefix = [] (const char &c) -> bool
        {
#ifdef _WIN32
            return (c == '-' || c == '/');
#else
            return (c == '-');
#endif
        };

        struct parse_args args = {
            .argc = argc,
            .argv = argv
        };

        const char *prog = parse_args::get_prog_name(argv[0]);
        const char *input = NULL;

        /* parse arguments */
        for (args.it = 1; args.it < argc; args.it++) {
            args.cur = argv[args.it];

            /* non-option argument --> input */
            if (!is_arg_prefix(args.cur[0]) || strcmp(args.cur, "-") == 0) {
                if (!input) {
                    input = args.cur;
                } else {
                    std::cerr << "warning: non-option argument ignored: " << args.cur << std::endl;
                }
                continue;
            }

            auto cur = args.cur;

            /* skip prefix */
            args.cur++;

            switch(*args.cur)
            {
            case '?':
                if ( args.get_noarg("?") ) {
                    help::print(prog);
                    std::exit(0);
                }
                break;

            case 'h':
                if ( args.get_noarg("help") ) {
                    help::print(prog);
                    std::exit(0);
                }
                break;

            case 'f':
                if ( args.get_arg("format") ) {
                    output::format out = utils::format_enum(args.opt);

                    if (out == output::error) {
                        std::cerr << prog << "unknown output format: "
                            << args.opt << std::endl;
                        return false;
                    }
                    gdo.format(out);
                    continue;
                } else if ( args.get_noarg("force") ) {
                    gdo.force(true);
                    continue;
                } else if ( args.get_noarg("full-help") ) {
                    help::print_full(prog);
                    std::exit(0);
                }
                break;

            case 'o':
                if ( args.get_arg("o") ) {
                    gdo.output(args.opt);
                    continue;
                }
                break;

            case 'n':
                if ( args.get_arg("name") ) {
                    gdo.name(args.opt);
                    continue;
                } else if ( args.get_noarg("no-date") ) {
                    gdo.print_date(false);
                    continue;
                }
                break;

            case 't':
                if ( args.get_arg("template") ) {
                    gdo.custom_template(args.opt);
                    continue;
                }
                break;

            case 'l':
                if ( args.get_arg("library") ) {
                    std::string lib_a, lib_w;
                    utils::format_libname(args.opt, lib_a, lib_w);
                    gdo.default_lib(lib_a, lib_w);
                    continue;
                }
                break;

            case 'i':
                if ( args.get_arg("include") ) {
                    gdo.add_inc(utils::format_inc(args.opt));
                    continue;
                } else if ( args.get_noarg("ignore-options") ) {
                    gdo.read_options(false);
                    continue;
                }
                break;

            case 'd':
                if ( args.get_arg("define") ) {
                    gdo.add_def(utils::format_def(args.opt));
                    continue;
                }
                break;

            case 'D':
                if ( args.get_arg("D") ) {
                    gdo.add_def(utils::format_def(args.opt));
                    continue;
                }
                break;

            case 'P':
                if ( args.get_arg("P") ) {
                    gdo.add_pfx(args.opt);
                    continue;
                }
                break;

            case 'p':
                if ( args.get_arg("param") ) {
                    if (utils::eq_str_case(args.opt, "skip")) {
                        gdo.parameter_names(param::skip);
                    } else if (utils::eq_str_case(args.opt, "create")) {
                        gdo.parameter_names(param::create);
                    } else {
                        std::cerr << prog << ": unknown argument for option '"
                            << *cur << "param': " << args.opt << std::endl;
                        return false;
                    }
                    continue;
                } else if ( args.get_noarg("print-symbols") ) {
                    gdo.print_symbols(true);
                    continue;
                }
                break;

            case 'S':
                if ( args.get_arg("S") ) {
                    gdo.add_sym(args.opt);
                    continue;
                }
                break;

            case 's':
                if ( args.get_noarg("separate") ) {
                    gdo.separate(true);
                    continue;
                }
                break;

            case 'a':
                if ( args.get_noarg("ast-all-symbols") ) {
                    gdo.ast_all_symbols(true);
                    continue;
                }
                break;

            default:
                /* let's support a help option with two dashes too */
                if (strcmp(cur, "--help") == 0) {
                    help::print(prog);
                    std::exit(0);
                }
                break;
            }

            std::cerr << prog << ": unknown option: " << cur << std::endl;
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
    gendlopen gdo(argc, argv);

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
