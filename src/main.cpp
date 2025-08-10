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

#include <errno.h>  /* program_invocation_short_name */
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include "gendlopen.hpp"
#include "parse_args.hpp"
#include "types.hpp"
#include "utils.hpp"

#if !defined(HAVE_GETPROGNAME) && !defined(HAVE_PROGRAM_INVOCATION_SHORT_NAME)
# define SAVE_ARGV0
#endif

#ifdef _WIN32
# define PATH_SEPARATOR '\\'
#else
# define PATH_SEPARATOR '/'
#endif



/* anonymous */
namespace
{
#ifdef SAVE_ARGV0
    const char *argv0;
#endif

    /* get program name without full path */
    const char *get_prog_name()
    {
#ifdef HAVE_PROGRAM_INVOCATION_SHORT_NAME
        return program_invocation_short_name; /* GNU */

#elif defined(HAVE_GETPROGNAME)
        return getprogname(); /* BSD */

#else
        const char *p = strrchr(argv0, PATH_SEPARATOR);

        if (p && *(p+1) != 0) {
            return p + 1;
        }

        return argv0;
#endif
    }

    /* -param=... */
    void set_parameter_names(gendlopen &gdo, parse_args &arg)
    {
        const char *opt = arg.opt();

        if (utils::strcasecmp(opt, "skip") == 0) {
            gdo.parameter_names(param::skip);
        } else if (utils::strcasecmp(opt, "create") == 0) {
            gdo.parameter_names(param::create);
        } else if (utils::strcasecmp(opt, "read") == 0) {
            gdo.parameter_names(param::read);
        } else {
            std::string msg = "unknown argument for option '";
            msg += arg.prefix();
            msg += "param': ";
            msg += opt;
            throw parse_args::error(msg);
        }
    }

    /* parse argv[] and set options in 'gdo' object */
    void parse_arguments(gendlopen &gdo, const int &argc, char ** const &argv)
    {
        const char *input = NULL;
        const char *cur = NULL;

        parse_args a(argc, argv);

        /* parse arguments */
        for (cur = a.begin(); cur != NULL; cur = a.next()) {
            /* use first non-option argument as input */
            if (a.pfxlen() == 0) {
                if (!input) {
                    input = cur;
                } else {
                    std::cerr << "warning: non-option argument ignored: " << cur << std::endl;
                }
                continue;
            }

            /* skip prefix */
            const char *p = cur + a.pfxlen();

            /* use uppercase for single-letter arguments only */
            switch(*p)
            {
            /* single letter */
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

            case 'S':
                if ( a.get_arg("S") ) {
                    gdo.add_sym(a.opt());
                    continue;
                }
                break;

            case '?':
                if ( a.get_noarg("?") ) {
                    help::print(get_prog_name());
                    std::exit(0);
                }
                break;

            /* multiple letters */
            case 'a':
                if ( a.get_noarg("ast-all-symbols") ) {
                    gdo.ast_all_symbols(true);
                    continue;
                }
                break;

            case 'd':
                if ( a.get_arg("define") ) {
                    gdo.add_def(a.opt());
                    continue;
                } else if ( a.get_noarg("dump-templates") ) {
                    data::dump_templates();
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
                    help::print_full(get_prog_name());
                    std::exit(0);
                }
                break;

            case 'h':
                if ( a.get_noarg("help") ) {
                    help::print(get_prog_name());
                    std::exit(0);
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

            case 'l':
                if ( a.get_arg("library") ) {
                    gdo.default_lib(a.opt());
                    continue;
                } else if ( a.get_noarg("line") ) {
                    gdo.line_directive(true);
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

            case 'o':
                if ( a.get_arg("out") ) {
                    gdo.output(a.opt());
                    continue;
                }
                break;

            case 'p':
                if ( a.get_arg("prefix") ) {
                    gdo.prefix(a.opt());
                    continue;
                } else if ( a.get_arg("param") ) {
                    set_parameter_names(gdo, a);
                    continue;
                } else if ( a.get_noarg("print-symbols") ) {
                    gdo.print_symbols(true);
                    continue;
                } else if ( a.get_noarg("print-lookup") ) {
                    gdo.print_lookup(true);
                    continue;
                }
                break;

            case 's':
                if ( a.get_noarg("separate") ) {
                    gdo.separate(true);
                    continue;
                }
                break;

            case 't':
                if ( a.get_arg("template") ) {
                    gdo.custom_template(a.opt());
                    continue;
                }
                break;

            default:
                break;
            }

            throw parse_args::error(std::string("unknown option: ") + cur);
        }

        /* input is required */
        if (!input) {
            throw parse_args::error("input file required");
        }

        gdo.input(input);
    }

} /* anonymous namespace */


int main(int argc, char **argv)
{
#ifdef SAVE_ARGV0
    argv0 = argv[0];
#endif

    try {
        gendlopen gdo;
        parse_arguments(gdo, argc, argv);
        gdo.process();
    }
    catch (const parse_args::error &e) {
        std::cerr << get_prog_name() << ": error: " << e.what() << std::endl;
        std::cerr << "Try `" << argv[0] << " -help' for more information." << std::endl;
        return 1;
    }
    catch (const gendlopen::error &e) {
        std::cerr << get_prog_name() << ": error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
