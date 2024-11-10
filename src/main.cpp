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

#if defined(__linux__)
# ifndef HAVE_PROGRAM_INVOCATION_NAME
#  define HAVE_PROGRAM_INVOCATION_NAME
# endif
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
    defined(__DragonFly__) || defined(__APPLE__)
# ifndef HAVE_GETPROGNAME
#  define HAVE_GETPROGNAME
# endif
#endif

#include <iostream>
#include <list>
#include <regex>
#include <stdlib.h>

#include "global.hpp"


/* anonymous */
namespace
{
    /* hold infos to parse command line arguments */
    typedef struct _parse_args {
        int         argc = 0;     /* argument count */
        char      **argv = NULL;  /* argument vector */
        int         it   = 0;     /* current argc iterator */
        const char *cur  = NULL;  /* current argv[] element */
        const char *opt  = NULL;  /* current argument's option */
    } parse_args_t;

    typedef enum _parser_retval {
        E_CONTINUE,
        E_EXIT_OK,
        E_EXIT_ERROR
    } parser_retval_t;


    const char *get_prog_name(const char *prog)
    {
        /* get program name without full path */
#ifdef HAVE_PROGRAM_INVOCATION_NAME
        prog = program_invocation_short_name;
#elif defined(HAVE_GETPROGNAME)
        prog = getprogname();
#else
        auto is_path_separator = [] (const char &c) -> bool
        {
# ifdef _WIN32
            return (c == '\\' || c == '/');
# else
            return (c == '/');
# endif
        };

        for (auto p = prog; *p != 0; p++) {
            if (is_path_separator(*p) && *(p+1) != 0) {
                prog = p + 1;
            }
        }
#endif

        return prog;
    }


    /* get argument from an option string */
    bool get_argx(parse_args_t &args, const char *opt, const size_t optlen)
    {
        auto err_noarg = [&] () {
            std::cerr << get_prog_name(args.argv[0]) << ": option requires an argument: "
                << args.argv[args.it] << std::endl;
            std::exit(1);
        };

        args.opt = NULL;

        if (strncmp(args.cur, opt, optlen) != 0) {
            /* not the argument we're looking for */
            return false;
        }

        /* "-foo bar" --> get next item */
        if (strcmp(args.cur, opt) == 0) {
            if ((args.it + 1) >= args.argc) {
                err_noarg();
            }

            args.opt = args.argv[++args.it];

            return true;
        }

        /* -foo=bar, -Dfoo --> get substring */
        if (strlen(args.cur) > optlen) {
            if (optlen == 1) {
                /* -Dfoo */
                args.opt = args.cur + 1;
            } else if (args.cur[optlen] == '=') {
                /* -foo=bar */
                args.opt = args.cur + optlen + 1;
            }

            if (!args.opt || *args.opt == 0) {
                err_noarg();
            }

            return true;
        }

        return false;
    }

    template<size_t N>
    constexpr bool get_arg(parse_args_t &args, char const (&opt)[N])
    {
        return get_argx(args, opt, N-1);
    }


    /* option without argument */
    bool get_noargx(parse_args_t &args, const char *opt, const size_t optlen)
    {
        /* -foo */
        if (strcmp(args.cur, opt) == 0) {
            return true;
        }

        /* -foo=bar */
        if (strncmp(args.cur, opt, optlen) == 0 && args.cur[optlen] == '=') {
            std::cerr << get_prog_name(args.argv[0]) << ": option does not take an argument: "
                << *args.argv[args.it] << opt << std::endl;
            std::exit(1);
        }

        return false;
    }

    template<size_t N>
    constexpr bool get_noarg(parse_args_t &args, char const (&opt)[N])
    {
        return get_noargx(args, opt, N-1);
    }


    parser_retval_t parse_arguments(gendlopen &gdo, int argc, char **argv)
    {
        auto is_arg_prefix = [] (const char &c) -> bool
        {
#ifdef _WIN32
            return (c == '-' || c == '/');
#else
            return (c == '-');
#endif
        };

        parse_args_t args;
        const char *input = NULL;

        args.argc = argc;
        args.argv = argv;

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
                if ( get_noarg(args, "?") ) {
                    help::print(get_prog_name(argv[0]));
                    return E_EXIT_OK;
                }
                break;

            case 'h':
                if ( get_noarg(args, "help") ) {
                    help::print(get_prog_name(argv[0]));
                    return E_EXIT_OK;
                }
                break;

            case 'f':
                if ( get_arg(args, "format") ) {
                    output::format out = utils::format_enum(args.opt);

                    if (out == output::error) {
                        std::cerr << get_prog_name(argv[0]) << "unknown output format: "
                            << args.opt << std::endl;
                        return E_EXIT_ERROR;
                    }
                    gdo.format(out);
                    continue;
                } else if ( get_noarg(args, "force") ) {
                    gdo.force(true);
                    continue;
                } else if ( get_noarg(args, "full-help") ) {
                    help::print_full(get_prog_name(argv[0]));
                    return E_EXIT_OK;
                }
                break;

            case 'o':
                if ( get_arg(args, "o") ) {
                    gdo.output(args.opt);
                    continue;
                }
                break;

            case 'n':
                if ( get_arg(args, "name") ) {
                    gdo.name(args.opt);
                    continue;
                }
                break;

            case 't':
                if ( get_arg(args, "template") ) {
                    gdo.custom_template(args.opt);
                    continue;
                }
                break;

            case 'l':
                if ( get_arg(args, "library") ) {
                    std::string lib_a, lib_w;
                    utils::format_libname(args.opt, lib_a, lib_w);
                    gdo.default_lib(lib_a, lib_w);
                    continue;
                }
                break;

            case 'i':
                if ( get_arg(args, "include") ) {
                    gdo.add_inc(utils::format_inc(args.opt));
                    continue;
                } else if ( get_noarg(args, "ignore-options") ) {
                    gdo.read_options(false);
                    continue;
                }
                break;

            case 'd':
                if ( get_arg(args, "define") ) {
                    gdo.add_def(utils::format_def(args.opt));
                    continue;
                }
                break;

            case 'D':
                if ( get_arg(args, "D") ) {
                    gdo.add_def(utils::format_def(args.opt));
                    continue;
                }
                break;

            case 'P':
                if ( get_arg(args, "P") ) {
                    gdo.add_pfx(args.opt);
                    continue;
                }
                break;

            case 'p':
                if ( get_arg(args, "param") ) {
                    if (utils::eq_str_case(args.opt, "skip")) {
                        gdo.parameter_names(param::skip);
                    } else if (utils::eq_str_case(args.opt, "create")) {
                        gdo.parameter_names(param::create);
                    } else {
                        std::cerr << get_prog_name(argv[0]) << ": unknown argument for option '"
                            << *cur << "param': " << args.opt << std::endl;
                        return E_EXIT_ERROR;
                    }
                    continue;
                } else if ( get_noarg(args, "print-symbols") ) {
                    gdo.print_symbols(true);
                    continue;
                }
                break;

            case 'S':
                if ( get_arg(args, "S") ) {
                    gdo.add_sym(args.opt);
                    continue;
                }
                break;

            case 's':
                if ( get_noarg(args, "separate") ) {
                    gdo.separate(true);
                    continue;
                }
                break;

            case 'a':
                if ( get_noarg(args, "ast-all-symbols") ) {
                    gdo.ast_all_symbols(true);
                    continue;
                }
                break;

            default:
                /* let's support a help option with two dashes too */
                if (strcmp(cur, "--help") == 0) {
                    help::print(get_prog_name(argv[0]));
                    return E_EXIT_OK;
                }
                break;
            }

            std::cerr << get_prog_name(argv[0]) << ": unknown option: " << cur << std::endl;
            return E_EXIT_ERROR;
        }

        /* input is required */
        if (!input) {
            std::cerr << get_prog_name(argv[0]) << ": input file required" << std::endl;
            return E_EXIT_ERROR;
        }

        gdo.input(input);

        return E_CONTINUE;
    }

} /* anonymous namespace */


int main(int argc, char **argv)
{
    /* initialize class */
    gendlopen gdo(argc, argv);

    /* parse command line */
    switch (parse_arguments(gdo, argc, argv))
    {
    case E_CONTINUE:
        break;

    case E_EXIT_OK:
        return 0;

    case E_EXIT_ERROR:
    default:
        std::cerr << "Try `" << argv[0] << " -help' for more information." << std::endl;
        return 1;
    }

    /* generate output */
    try {
        gdo.generate();
    }
    catch (const gendlopen::error &e) {
        std::cerr << get_prog_name(argv[0]) << ": error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
