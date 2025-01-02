/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2023-2025 Carsten Janssen

 Permission is hereby  granted, free of charge, to any  person obtaining a copy
 of this software and associated  documentation files (the "Software"), to deal
 in the Software  without restriction, including without  limitation the rights
 to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
 copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
 IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
 FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
 AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
 LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
**/

#include <iostream>
#include <list>
#include <regex>
#include <string.h>

#include "global.hpp"


/* anonymous */
namespace
{
    /* -param=... */
    bool set_parameter_names(gendlopen &gdo, const char *prog, const char *opt, char optpfx)
    {
        if (strcasecmp(opt, "skip") == 0) {
            gdo.parameter_names(param::skip);
            return true;
        } else if (strcasecmp(opt, "create") == 0) {
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
        std::string lib_a, lib_w;
        const char *prog = parse_args::get_prog_name(argv[0]);
        const char *input = NULL;
        const char *cur = NULL;

        parse_args a(argc, argv);

        /* parse arguments */
        for (cur = a.current(); cur != NULL; cur = a.next()) {
            /* non-option argument --> input */
            if (!a.has_prefix() || strcmp(cur, "-") == 0) {
                if (!input) {
                    input = cur;
                } else {
                    std::cerr << "warning: non-option argument ignored: " << cur << std::endl;
                }
                continue;
            }

            /* skip prefix */
            cur++;

            switch(*cur)
            {
            case '?':
                if ( a.get_noarg("?") ) {
                    help::print(prog);
                    std::exit(0);
                }
                break;

            case 'h':
                if ( a.get_noarg("help") ) {
                    help::print(prog);
                    std::exit(0);
                }
                break;

            case 'f':
                if ( a.get_arg("format") ) {
                    gdo.format(a.opt());
                    continue;
                } else if ( a.get_noarg("force") ) {
                    gdo.force(true);
                    continue;
                } else if ( a.get_noarg("full-help") ) {
                    help::print_full(prog);
                    std::exit(0);
                }
                break;

            case 'o':
                if ( a.get_arg("o") ) {
                    gdo.output(a.opt());
                    continue;
                }
                break;

            case 'n':
                if ( a.get_noarg("no-date") ) {
                    gdo.print_date(false);
                    continue;
                } else if ( a.get_noarg("no-pragma-once") ) {
                    gdo.pragma_once(false);
                    continue;
                }
                break;

            case 't':
                if ( a.get_arg("template") ) {
                    gdo.custom_template(a.opt());
                    continue;
                }
                break;

            case 'l':
                if ( a.get_arg("library") ) {
                    gdo.default_lib(a.opt());
                    continue;
                } else if ( a.get_noarg("line") ) {
                    gdo.line_directive(true);
                    continue;
                }
                break;

            case 'i':
                if ( a.get_arg("include") ) {
                    gdo.add_inc(a.opt());
                    continue;
                } else if ( a.get_noarg("ignore-options") ) {
                    gdo.read_options(false);
                    continue;
                }
                break;

            case 'd':
                if ( a.get_arg("define") ) {
                    gdo.add_def(a.opt());
                    continue;
                }
                break;

            case 'D':
                if ( a.get_arg("D") ) {
                    gdo.add_def(a.opt());
                    continue;
                }
                break;

            case 'P':
                if ( a.get_arg("P") ) {
                    gdo.add_pfx(a.opt());
                    continue;
                }
                break;

            case 'p':
                if ( a.get_arg("prefix") ) {
                    gdo.prefix(a.opt());
                    continue;
                } else if ( a.get_arg("param") ) {
                    if (!set_parameter_names(gdo, prog, a.opt(), a.current()[0])) {
                        return false;
                    }
                    continue;
                } else if ( a.get_noarg("print-symbols") ) {
                    gdo.print_symbols(true);
                    continue;
                }
                break;

            case 'S':
                if ( a.get_arg("S") ) {
                    gdo.add_sym(a.opt());
                    continue;
                }
                break;

            case 's':
                if ( a.get_noarg("separate") ) {
                    gdo.separate(true);
                    continue;
                }
                break;

            case 'a':
                if ( a.get_noarg("ast-all-symbols") ) {
                    gdo.ast_all_symbols(true);
                    continue;
                }
                break;

            case '-':
                /* let's support a help option with two dashes too */
                if (strcmp(a.current(), "--help") == 0) {
                    help::print(prog);
                    std::exit(0);
                }
                break;

            default:
                break;
            }

            std::cerr << prog << ": unknown option: " << a.current() << std::endl;
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
    auto print_info = [&] () {
        std::cerr << "See `" << argv[0] << " -help' for more information." << std::endl;
    };

    gendlopen gdo;

    try {
        if (!parse_arguments(gdo, argc, argv)) {
            print_info();
            return 1;
        }
        gdo.generate();
    }
    catch (const parse_args::error &e) {
        std::cerr << parse_args::get_prog_name(argv[0])
            << ": error: " << e.what() << std::endl;
        print_info();
        return 1;
    }
    catch (const gendlopen::error &e) {
        std::cerr << parse_args::get_prog_name(argv[0])
            << ": error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
