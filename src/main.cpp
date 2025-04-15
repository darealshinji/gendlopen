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
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include "gendlopen.hpp"
#include "filesystem_compat.hpp"
#include "parse_args.hpp"
#include "types.hpp"

#if defined(__GLIBC__)
/* <features.h> is a Glibc header that defines __GLIBC__ and
 * will be automatically included with standard headers if present */
# ifndef HAVE_PROGRAM_INVOCATION_NAME
#  define HAVE_PROGRAM_INVOCATION_NAME
# endif
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
      defined(__DragonFly__) || defined(__APPLE__)
# ifndef HAVE_GETPROGNAME
#  define HAVE_GETPROGNAME
# endif
#endif



/* anonymous */
namespace
{
    /* get program name without full path */
    const char *get_prog_name(const char *prog)
    {
#ifdef HAVE_PROGRAM_INVOCATION_NAME
        (void)prog;
        return program_invocation_short_name; /* GNU */
#elif defined(HAVE_GETPROGNAME)
        (void)prog;
        return getprogname(); /* BSD */
#else
        static std::string buf;
# ifdef MINGW32_NEED_CONVERT_FILENAME
        buf = fs::filename(fs::convert_filename(prog));
# else
        buf = fs::filename(prog);
# endif
        return buf.c_str();
    }
#endif

    /* -param=... */
    void set_parameter_names(gendlopen &gdo, const char *prog, const char *opt, char optpfx)
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
        const char *prog = get_prog_name(argv[0]);
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
                    set_parameter_names(gdo, prog, a.opt(), a.current()[0]);
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

            throw parse_args::error(std::string("unknown option: ") + a.current());
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
        std::cerr << get_prog_name(argv[0]) << ": error: " << e.what() << std::endl;
        std::cerr << "Try `" << argv[0] << " -help' for more information." << std::endl;
        return 1;
    }
    catch (const gendlopen::error &e) {
        std::cerr << get_prog_name(argv[0]) << ": error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
