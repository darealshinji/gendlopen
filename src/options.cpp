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


struct args {
    int argc;
    char ** const argv;
    int it;
};


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

    template<size_t N>
    bool get_argument(struct args &a, const char *&ptr, char const (&opt)[N])
    {
        const char *arg = a.argv[a.it] + 1;
        const size_t optlen = N-1;

        if (strncmp(arg, opt, optlen) == 0) {
            if (arg[optlen] == 0) {
                a.it++;

                if (a.it < a.argc) {
                    /* next argv[] entry */
                    ptr = a.argv[a.it];
                    return true;
                }

                throw gendlopen::error(std::string("option '-") + std::string(opt) + "' requires an argument");
            } else if (optlen == 1) {
                ptr = arg + optlen; /* -Xabc */
                return true;
            } else if (arg[optlen] == '=') {
                ptr = arg + optlen + 1; /* -abc=X */
                return true;
            }
        }

        return false;
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
    struct args a = { .argc = argc, .argv = argv };

    /**
    parse arguments:
     - options begin with single dash (-foo)
     - "-" is a valid non-option argument
     - single-letter options are uppercase, no argument separator (-Xfoo)
     - multi-letter options are lowercase, argument separator is "=" (-foo=bar)
     - arguments can be next entry in argv list (-foo bar -X foo)
    **/
    for (a.it = 1; a.it < argc; a.it++) {
        arg = argv[a.it];

        /* use first non-option argument as input file */
        if (*arg != '-' || strcmp(arg, "-") == 0) {
            if (!input_file) {
                input_file = arg;
            } else {
                std::cerr << "warning: non-option argument ignored: " << arg << std::endl;
            }
            continue;
        }

        /* skip "-" */
        arg++;

        switch(*arg)
        {
        case '?':
            if (strcmp(arg, "?") == 0) {
                help::print(argv[0]);
                std::exit(0);
            }
            break;

        case 'a':
            if (strcmp(arg, "ast-all-symbols") == 0) {
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

#ifdef USE_EXTERNAL_RESOURCES
        case 'd':
            if (strcmp(arg, "dump-templates") == 0) {
                dump_templates();
                std::exit(0);
            }
            break;
#endif

        case 'f':
            if (get_argument(a, p, "format")) {
                format(p);
                continue;
            } else if (strcmp(arg, "force") == 0) {
                force(true);
                continue;
            } else if (strcmp(arg, "full-help") == 0) {
                help::print_full(argv[0]);
                std::exit(0);
            }
            break;

        case 'h':
            if (strcmp(arg, "help") == 0) {
                help::print(argv[0]);
                std::exit(0);
            }
            break;

        case 'i':
            if (get_argument(a, p, "include")) {
                add_inc(p);
                continue;
            } else if (strcmp(arg, "ignore-options") == 0) {
                read_options(false);
                continue;
            }
            break;

        case 'l':
            if (get_argument(a, p, "library")) {
                default_lib(p);
                continue;
            } else if (strcmp(arg, "line") == 0) {
                line_directive(true);
                continue;
            }
            break;

        case 'n':
            if (strcmp(arg, "no-date") == 0) {
                print_date(false);
                continue;
            } else if (strcmp(arg, "no-pragma-once") == 0) {
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
                parameter_names( param_value(p) );
                continue;
            } else if (strcmp(arg, "print-symbols") == 0) {
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
            if (strcmp(arg, "separate") == 0) {
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

        throw gendlopen::error(std::string("unknown option: ") + argv[a.it]);
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

