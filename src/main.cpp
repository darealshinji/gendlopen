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

#ifdef _MSC_VER
# include "strcasecmp.hpp"
#else
# include <strings.h>
#endif
#ifdef __MINGW32__
# include <libgen.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <iostream>
#include <string>
#include "gendlopen.hpp"
#include "filesystem_compat.hpp"
#include "parse_args.hpp"
#include "types.hpp"

#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__NetBSD__) || \
    defined(__DragonFly__) || defined(__APPLE__) || defined(LIBBSD_OVERLAY)
# define HAVE_GETPROGNAME
#endif
#if !defined(HAVE_GETPROGNAME) && !defined(__GLIBC__)
# define SAVE_ARGV0
#endif



/* anonymous */
namespace
{
#ifdef SAVE_ARGV0
    std::string argv0;
#endif

    /* get program name without full path */
    std::string get_prog_name()
    {
#ifdef HAVE_GETPROGNAME
        return getprogname();
#elif defined(__GLIBC__)
        return program_invocation_short_name;
#elif defined(__MINGW32__)
        return basename(const_cast<char *>(argv0.data()));
#else
        return fs::filename(argv0);
#endif
    }

    /* -param=... */
    void set_parameter_names(gendlopen &gdo, const char *opt, char optpfx)
    {
        if (strcasecmp(opt, "skip") == 0) {
            gdo.parameter_names(param::skip);
        } else if (strcasecmp(opt, "create") == 0) {
            gdo.parameter_names(param::create);
        } else if (strcasecmp(opt, "read") == 0) {
            gdo.parameter_names(param::read);
        } else {
            std::string msg = "unknown argument for option '";
            msg += optpfx;
            msg += "param': ";
            msg += opt;
            throw parse_args::error(msg);
        }
    }

    /* parse argv[] and set options in 'gdo' object */
    void parse_arguments(gendlopen &gdo, const int &argc, char ** const &argv)
    {
        std::string lib_a, lib_w;
        const char *input = NULL;
        const char *cur = NULL;

        parse_args a(argc, argv);

        /* parse arguments */
        for (cur = a.begin(); cur != NULL; cur = a.next()) {
            /* use first non-option argument (including "-") as input */
            if (!a.has_prefix()) {
                if (!input) {
                    input = cur;
                } else {
                    std::cerr << "warning: non-option argument ignored: " << cur << std::endl;
                }
                continue;
            }

            switch(*(cur+1)) /* skip prefix */
            {
            case '?':
                if ( a.get_noarg("?") ) {
                    help::print(get_prog_name());
                    std::exit(0);
                }
                break;

            case 'h':
                if ( a.get_noarg("help") ) {
                    help::print(get_prog_name());
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
                    set_parameter_names(gdo, a.opt(), *cur);
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
                if (strcmp(cur, "--help") == 0) {
                    help::print(get_prog_name());
                    std::exit(0);
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
    gendlopen gdo;

#ifdef SAVE_ARGV0
    argv0 = argv[0];
#endif

    try {
        parse_arguments(gdo, argc, argv);

        /* read input */
        gdo.tokenize();

        /* print and exit */
        if (gdo.print_symbols()) {
            gdo.print_symbols_to_stdout();
            return 0;
        }

        /* read and process custom template (`-format' will be ignored) */
        if (!gdo.custom_template().empty()) {
            gdo.process_custom_template();
            return 0;
        }

        /* generate output */
        gdo.generate();
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
