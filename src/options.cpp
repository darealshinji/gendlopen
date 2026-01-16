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
        if (str.compare(0, N-1, opt) == 0 && str.size() > N-1) {
            ptr = str.c_str() + (N-1);
            return true;
        }

        return false;
    }


    const char *get_argument(const char *arg, const int &argc, char ** const &argv, int &i, const char *opt, size_t optlen)
    {
        if (strncmp(arg, opt, optlen) == 0) {
            if (arg[optlen] == 0) {
                if (++i < argc) {
                    return argv[i]; /* next argv[] entry */
                }

                throw gendlopen::error(std::string("option '-") + std::string(opt) + "' requires an argument");
            } else if (optlen == 1) {
                return arg + optlen; /* -Xabc */
            } else if (arg[optlen] == '=') {
                return arg + optlen + 1; /* -abc=X */
            }
        }

        return NULL;
    }


    /* get param enum from string */
    param::names param_value(const char *opt)
    {
        if (utils::strcasecmp(opt, "skip") == 0) {
            return param::skip;
        } else if (utils::strcasecmp(opt, "create") == 0) {
            return param::create;
        } else if (utils::strcasecmp(opt, "read") == 0) {
            return param::read;
        }

        throw gendlopen::error(std::string("unknown argument for option '-param': ") + opt);
    }
}


/* parse command line arguments */
void gendlopen::parse_cmdline(const int &argc, char ** const &argv)
{
    const char *input_file = NULL;
    const char *p = NULL;
    const char *arg = NULL;
    int i = 1; /* skip argv[0] */

    auto arg_eq = [&] (const char *opt) -> bool {
        return (strcmp(arg, opt) == 0);
    };

    auto get_arg_len = [&] (const char *opt, size_t len) -> bool {
        return ((p = get_argument(arg, argc, argv, i, opt, len)) != NULL);
    };

    /**
    parse arguments:
     - options begin with single dash (-foo)
     - "-" is a valid non-option argument
     - single-letter options are uppercase, no argument separator (-Xfoo)
     - multi-letter options are lowercase, argument separator is "=" (-foo=bar)
     - arguments can be next entry in argv list (-foo bar -X foo)
    **/
    for ( ; i < argc; i++) {
        arg = argv[i];

        /* use first non-option argument as input file */
        if (*arg != '-' || arg_eq("-")) {
            if (!input_file) {
                input_file = arg;
            } else {
                std::cerr << "warning: non-option argument ignored: " << arg << std::endl;
            }
            continue;
        }

        arg++;

#define XSTRLEN(x) (sizeof(x)-1)
#define get_arg(x) get_arg_len(x, XSTRLEN(x))

        switch(*arg)
        {
        case '?':
            if (arg_eq("?")) {
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
            if (get_arg("D")) {
                add_def(p);
                continue;
            }
            break;

#ifdef USE_EXTERNAL_RESOURCES
        case 'd':
            if (arg_eq("dump-templates")) {
                dump_templates();
                std::exit(0);
            }
            break;
#endif

        case 'f':
            if (get_arg("format")) {
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
            if (get_arg("include")) {
                add_inc(p);
                continue;
            } else if (arg_eq("ignore-options")) {
                read_options(false);
                continue;
            }
            break;

        case 'l':
            if (get_arg("library")) {
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
            if (get_arg("out")) {
                output(p);
                continue;
            }
            break;

        case 'P':
            if (get_arg("P")) {
                add_pfx(p);
                continue;
            }
            break;

        case 'p':
            if (get_arg("prefix")) {
                prefix(p);
                continue;
            } else if (get_arg("param")) {
                parameter_names( param_value(p) );
                continue;
            } else if (arg_eq("print-symbols")) {
                print_symbols(true);
                continue;
            }
            break;

        case 'S':
            if (get_arg("S")) {
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
            if (get_arg("template")) {
                custom_template(p);
                continue;
            }
#ifdef USE_EXTERNAL_RESOURCES
            else if (get_arg("templates-path")) {
                templates_path(p);
                utils::append_missing_separator(m_templates_path);
                continue;
            }
#endif
            break;

        default:
            break;
        }

        throw gendlopen::error(std::string("unknown option: -") + arg);
    }

    /* input_file is required */
    if (!input_file || *input_file == 0) {
        throw gendlopen::error("input file required");
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
                parameter_names( param_value(p) );
                continue;
            }
            break;

        default:
            break;
        }

        throw gendlopen::error("unknown %option string: " + token);
    }
}

