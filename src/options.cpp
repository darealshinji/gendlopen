/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2023-2026 Carsten Janssen

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

#include <string.h>
#include <string>
#include <vector>
#include "gendlopen.hpp"
#include "types.hpp"
#include "utils.hpp"

struct args {
    int argc;
    char ** const argv;
    int it;
    bool longpfx;
};

namespace help
{
    extern void print(const char *prog);
    extern void print_full(const char *prog);
}


namespace /* anonymous */
{
    template<size_t N>
    bool get_option(const std::string &str, const char *&ptr, char const (&opt)[N])
    {
        const size_t optlen = N - 1;

        if (str.compare(0, optlen, opt) == 0 && str.size() > optlen) {
            ptr = str.c_str() + optlen;

            /* ignore empty argument */
            return (*ptr != 0);
        }

        return false;
    }


    bool get_argument_len(struct args &a, const char *&ptr, const char *opt, const size_t optlen)
    {
        char pfx[3] = { 0, 0, 0 };

        /* only short prefix for single-letter options */
        if (optlen == 1 && a.longpfx) {
            return false;
        }

        const char *arg = a.argv[a.it];

        /* save prefix letters and skip prefix */
        if (a.longpfx) {
            pfx[0] = arg[0];
            pfx[1] = arg[1];
            arg += 2;
        } else {
            pfx[0] = arg[0];
            arg++;
        }

        /* compare option string */
        if (strncmp(arg, opt, optlen) != 0) {
            return false;
        }

        if (arg[optlen] == 0) {
            /* get next argv[] entry */
            a.it++;

            if (a.it >= a.argc) {
                throw gendlopen::error_cmd(std::string("option '") + std::string(pfx)
                    + std::string(opt) + "' requires an argument");
            }

            ptr = a.argv[a.it];
        }
        else if (optlen == 1) {
            /* -Xabc */
            ptr = arg + optlen;
        }
        else if (arg[optlen] == '=') {
            /* -foo=abc */
            ptr = arg + optlen + 1;
        }
#ifdef _WIN32
        else if (arg[optlen] == ':') {
            /* /foo:abc */
            ptr = arg + optlen + 1;
        }
#endif
        else {
            /* no match */
            return false;
        }

        /* empty argument? */
        if (*ptr == 0) {
            throw gendlopen::error_cmd(std::string("option '")  + std::string(pfx)
                + std::string(opt) + "' requires a non-empty argument");
        }

        return true;
    }


    template<size_t N>
    bool get_argument(struct args &a, const char *&ptr, char const (&opt)[N])
    {
        return get_argument_len(a, ptr, opt, N-1);
    }
}


/* parse command line arguments */
void gendlopen::parse_cmdline(const int &argc, char ** const &argv)
{
    const char *input_file = NULL;
    const char *p = NULL;
    const char *arg = NULL;
    struct args a = { .argc = argc, .argv = argv };

    auto arg_eq = [&] (const char *str) -> bool {
        return (strcmp(arg, str) == 0);
    };

    /**
    parse arguments:
     - options begin with "-" or "--" (or "/" on Windows)
     - single-letter options begin with "-" (or "/" on Windows)
     - "-", "--" and "/" are valid non-option arguments
     - single-letter options are uppercase, no argument separator (-Xfoo)
     - multi-letter options are lowercase, argument separator is "=" (or ":" on Windows) (-foo=bar)
     - arguments can be next entry in argv list (-foo bar -X foo)
    **/
    for (a.it = 1; a.it < argc; a.it++) {
        arg = argv[a.it];

        /* prefixed with dash(es) */
        if (*arg == '-' && !arg_eq("-") && !arg_eq("--")) {
            if (arg[1] == '-') {
                /* long prefix */
                arg += 2;
                a.longpfx = true;
            } else {
                /* short prefix */
                arg++;
                a.longpfx = false;
            }
        }
#ifdef _WIN32
        /* prefixed with forward slash */
        else if (*arg == '/' && arg[1] != 0) {
            arg++;
            a.longpfx = false;
        }
#endif
        else {
            /* use first non-option argument as input file, otherwise warn */
            if (!input_file) {
                input_file = arg;
            } else {
                std::cerr << "warning: non-option argument ignored: " << arg << std::endl;
            }
            continue;
        }

        switch(*arg)
        {
        case '?':
            if (!a.longpfx && arg_eq("?")) {
                help::print(argv[0]);
                std::exit(0);
            }
            break;

        case 'a':
            if (arg_eq("ast-all-symbols")) {
                ast_all_symbols(true);
                continue;
            }
            break;

        case 'D':
            if (get_argument(a, p, "D")) {
                add_def(p);
                continue;
            }
            break;

#if !defined(USE_EXTERNAL_RESOURCES)
        case 'd':
            if (get_argument(a, p, "dump-templates")) {
                dump_templates(p);
                std::exit(0);
            }
            break;
#endif

        case 'f':
            if (get_argument(a, p, "format")) {
                format(p);
                continue;
            } else if (arg_eq("force")) {
                force(true);
                continue;
            } else if (arg_eq("full-help")) {
                help::print_full(argv[0]);
                std::exit(0);
            }
            break;

        case 'h':
            if (arg_eq("help")) {
                help::print(argv[0]);
                std::exit(0);
            }
            break;

        case 'i':
            if (get_argument(a, p, "include")) {
                add_inc(p);
                continue;
            } else if (arg_eq("ignore-options")) {
                read_options(false);
                continue;
            }
            break;

        case 'l':
            if (get_argument(a, p, "library")) {
                default_lib(p);
                continue;
            } else if (arg_eq("line")) {
                line_directive(true);
                continue;
            }
            break;

        case 'n':
            if (arg_eq("no-date")) {
                print_date(false);
                continue;
            } else if (arg_eq("no-pragma-once")) {
                pragma_once(false);
                continue;
            }
            break;

        case 'o':
            if (get_argument(a, p, "out")) {
                output(p);
                continue;
            }
            break;

        case 'P':
            if (get_argument(a, p, "P")) {
                add_pfx(p);
                continue;
            }
            break;

        case 'p':
            if (get_argument(a, p, "prefix")) {
                prefix(p);
                continue;
            } else if (get_argument(a, p, "param")) {
                parameter_names(p);
                continue;
            } else if (arg_eq("print-symbols")) {
                print_symbols(true);
                continue;
            }
            break;

        case 'S':
            if (get_argument(a, p, "S")) {
                add_sym(p);
                continue;
            }
            break;

        case 's':
            if (arg_eq("separate")) {
                separate(true);
                continue;
            }
            break;

        case 't':
            if (get_argument(a, p, "template")) {
                custom_template(p);
                continue;
            }
#ifdef USE_EXTERNAL_RESOURCES
            else if (get_argument(a, p, "templates-path")) {
                templates_path(p);
                utils::append_missing_separator(m_templates_path);
                continue;
            }
#endif
            break;

        default:
            break;
        }

        throw gendlopen::error_cmd(std::string("unknown option: ") + argv[a.it]);
    }

    if (!input_file || *input_file == 0) {
        throw gendlopen::error_cmd("input file is required");
    }

    input(input_file);
}


/* parse `%option' strings */
void gendlopen::parse_options(const vstring_t &options)
{
    const char *p = NULL;

    for (const auto &token : options)
    {
        switch (token[0])
        {
        case 'D':
            if (get_option(token, p, "D=")) {
                add_def(p);
                continue;
            }
            break;

        case 'f':
            if (get_option(token, p, "format=")) {
                format(p);
                continue;
            }
            break;

        case 'i':
            if (get_option(token, p, "include=")) {
                add_inc(p);
                continue;
            }
            break;

        case 'l':
            if (get_option(token, p, "library=")) {
                default_lib(p);
                continue;
            } else if (token == "line") {
                line_directive(true);
                continue;
            }
            break;

        case 'n':
            if (token == "no-date") {
                print_date(false);
                continue;
            } else if (token == "no-pragma-once") {
                pragma_once(false);
                continue;
            }
            break;

        case 'p':
            if (get_option(token, p, "prefix=")) {
                prefix(p);
                continue;
            } else if (get_option(token, p, "param=")) {
                parameter_names(p);
                continue;
            }
            break;

        default:
            break;
        }

        throw gendlopen::error("unknown %option string: " + token);
    }
}

