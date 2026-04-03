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

#define OPT_ENABLE_ASSERT
#define OPT_ENABLE_SHORTOPT
#define OPT_ENABLE_LONGOPT
#ifdef _WIN32
# define OPT_ENABLE_WIN32OPT
#endif
#include "opt_parser.h"

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

    template<size_t N>
    bool get_arg(struct opt_args *a, char const (&opt)[N])
    {
        return opt_get_arg(a, opt, N-1);
    }

    void opt_error(const char *msg)
    {
        throw gendlopen::error_cmd(msg);
    }
}


/* parse command line arguments */
void gendlopen::parse_cmdline(const int &argc, char ** const &argv)
{
    struct opt_args a;
    const char *input_file = NULL;

    opt_init(&a, argc, argv, 0, opt_error);

    while (opt_iterate(&a)) {
        if (a.pfxlen == 0) {
            /* use first non-option argument as input file, otherwise warn */
            if (!input_file) {
                input_file = a.current;
            } else {
                std::cerr << "warning: non-option argument ignored: " << a.current << std::endl;
            }
        } else if (opt_arg_eq(&a, "?") || opt_arg_eq(&a, "help")) {
            help::print(argv[0]);
            std::exit(0);
        } else if (opt_arg_eq(&a, "full-help")) {
            help::print_full(argv[0]);
            std::exit(0);
        } else if (opt_arg_eq(&a, "ast-all-symbols")) {
            ast_all_symbols(true);
        } else if (get_arg(&a, "D")) {
            add_def(a.value);
        }
#if !defined(USE_EXTERNAL_RESOURCES)
        else if (get_arg(&a, "dump-templates")) {
            dump_templates(a.value);
            std::exit(0);
        }
#endif
        else if (get_arg(&a, "format")) {
            format(a.value);
        } else if (opt_arg_eq(&a, "force")) {
            force(true);
        } else if (get_arg(&a, "include")) {
            add_inc(a.value);
        } else if (opt_arg_eq(&a, "ignore-options")) {
            read_options(false);
        } else if (get_arg(&a, "library")) {
            default_lib(a.value);
        } else if (opt_arg_eq(&a, "line")) {
            line_directive(true);
        } else if (opt_arg_eq(&a, "no-date")) {
            print_date(false);
        } else if (opt_arg_eq(&a, "no-pragma-once")) {
            pragma_once(false);
        } else if (get_arg(&a, "out")) {
            output(a.value);
        } else if (get_arg(&a, "P")) {
            add_pfx(a.value);
        } else if (get_arg(&a, "prefix")) {
            prefix(a.value);
        } else if (get_arg(&a, "param")) {
            parameter_names(a.value);
        } else if (opt_arg_eq(&a, "print-symbols")) {
            print_symbols(true);
        } else if (get_arg(&a, "S")) {
            add_sym(a.value);
        } else if (opt_arg_eq(&a, "separate")) {
            separate(true);
        } else if (get_arg(&a, "template")) {
            custom_template(a.value);
        }
#ifdef USE_EXTERNAL_RESOURCES
        else if (get_arg(&a, "templates-path")) {
            templates_path(a.value);
        }
#endif
        else if (opt_arg_eq(&a, "version")) {
            std::cout << "gendlopen " << gendlopen::version() << std::endl;
            std::exit(0);
        } else {
            throw gendlopen::error_cmd(std::string("unknown option: ") + a.current);
        }
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

    for (const auto &token : options) {
        if (get_option(token, p, "D=")) {
            add_def(p);
        } else if (get_option(token, p, "format=")) {
            format(p);
        } else if (get_option(token, p, "include=")) {
            add_inc(p);
        } else if (get_option(token, p, "library=")) {
            default_lib(p);
        } else if (token == "line") {
            line_directive(true);
        } else if (token == "no-date") {
            print_date(false);
        } else if (token == "no-pragma-once") {
            pragma_once(false);
        } else if (get_option(token, p, "prefix=")) {
            prefix(p);
        } else if (get_option(token, p, "param=")) {
            parameter_names(p);
        } else {
            throw gendlopen::error("unknown %option string: " + token);
        }
    }
}

