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

#include <string.h>
#include <string>
#include <vector>
#include "gendlopen.hpp"
#include "get_args.hpp"
#include "types.hpp"
#include "utils.hpp"

namespace help
{
    extern void print(const char *prog);
    extern void print_full(const char *prog);
}


namespace /* anonymous */
{
    /* get argument from an option string */
    const char *get_arg_len(const std::string &str, const char *opt, const size_t optlen)
    {
        /* strncmp(str.c_str(), opt, optlen) */
        if (str.compare(0, optlen, opt) != 0) {
            return NULL;  /* not the argument we're looking for */
        }

        if (str.size() > optlen) {
            return str.c_str() + optlen;
        }

        /* no argument */
        return NULL;
    }


    template<size_t N>
    constexpr bool get_arg(const std::string &str, const char *&ptr, char const (&opt)[N])
    {
        return ((ptr = get_arg_len(str, opt, N-1)) != NULL);
    }


    /* get param enum from string */
    param::names param_value(const char *opt, std::string prefix)
    {
        if (utils::strcasecmp(opt, "skip") == 0) {
            return param::skip;
        } else if (utils::strcasecmp(opt, "create") == 0) {
            return param::create;
        } else if (utils::strcasecmp(opt, "read") == 0) {
            return param::read;
        }

        std::string msg = "unknown argument for option '";
        msg += prefix;
        msg += "param': ";
        msg += opt;

        throw get_args::error(msg);
    }
}


/* parse command line arguments */
void gendlopen::parse_cmdline(const int &argc, char ** const &argv)
{
    const char *input_file = NULL;
    const char *cur = NULL;

    get_args a(argc, argv);

    /* parse arguments */
    for (cur = a.begin(); cur != NULL; cur = a.next()) {
        /* use first non-option argument as input_file file */
        if (a.pfxlen() == 0) {
            if (!input_file) {
                input_file = cur;
            } else {
                std::cerr << "warning: non-option argument ignored: " << cur << std::endl;
            }
            continue;
        }

        /* skip prefix */
        const char *p = cur + a.pfxlen();

        switch(*p)
        {
        /* single letters (uppercase only!) */
        case 'D':
            if (a.get_arg("D")) {
                add_def(a.opt());
                continue;
            }
            break;

        case 'P':
            if (a.get_arg("P")) {
                add_pfx(a.opt());
                continue;
            }
            break;

        case 'S':
            if (a.get_arg("S")) {
                add_sym(a.opt());
                continue;
            }
            break;

        case '?':
            if (a.get_noarg("?")) {
                help::print(argv[0]);
                std::exit(0);
            }
            break;

        /* multiple letters */
        case 'a':
            if (a.get_noarg("ast-all-symbols")) {
                ast_all_symbols(true);
                continue;
            }
            break;

        case 'd':
            if (a.get_arg("define")) {
                add_def(a.opt());
                continue;
            }
#ifdef USE_EXTERNAL_RESOURCES
            else if (a.get_noarg("dump-templates")) {
                dump_templates();
                std::exit(0);
            }
#endif
            break;

        case 'f':
            if (a.get_arg("format")) {
                format(a.opt());
                continue;
            } else if (a.get_noarg("force")) {
                force(true);
                continue;
            } else if (a.get_noarg("full-help")) {
                help::print_full(argv[0]);
                std::exit(0);
            }
            break;

        case 'h':
            if (a.get_noarg("help")) {
                help::print(argv[0]);
                std::exit(0);
            }
            break;

        case 'i':
            if (a.get_arg("include")) {
                add_inc(a.opt());
                continue;
            } else if (a.get_noarg("ignore-options")) {
                read_options(false);
                continue;
            }
            break;

        case 'l':
            if (a.get_arg("library")) {
                default_lib(a.opt());
                continue;
            } else if (a.get_noarg("line")) {
                line_directive(true);
                continue;
            }
            break;

        case 'n':
            if (a.get_noarg("no-date")) {
                print_date(false);
                continue;
            } else if (a.get_noarg("no-pragma-once")) {
                pragma_once(false);
                continue;
            }
            break;

        case 'o':
            if (a.get_arg("out")) {
                output(a.opt());
                continue;
            }
            break;

        case 'p':
            if (a.get_arg("prefix")) {
                prefix(a.opt());
                continue;
            } else if (a.get_arg("param")) {
                parameter_names( param_value(a.opt(), a.prefix()) );
                continue;
            } else if (a.get_noarg("print-symbols")) {
                print_symbols(true);
                continue;
            }
            break;

        case 's':
            if (a.get_noarg("separate")) {
                separate(true);
                continue;
            }
            break;

        case 't':
            if (a.get_arg("template")) {
                custom_template(a.opt());
                continue;
            }
#ifdef USE_EXTERNAL_RESOURCES
            else if (a.get_arg("templates-path")) {
                templates_path(a.opt());
                utils::append_missing_separator(m_templates_path);
                continue;
            }
#endif
            break;

        default:
            break;
        }

        throw get_args::error(std::string("unknown option: ") + cur);
    }

    /* input_file is required */
    if (!input_file) {
        throw get_args::error("input file required");
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
        case 'f':
            if (get_arg(token, p, "format=")) {
                format(p);
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

        case 'l':
            if (get_arg(token, p, "library=")) {
                default_lib(p);
                continue;
            } else if (token == "line") {
                line_directive(true);
                continue;
            }
            break;

        case 'i':
            if (get_arg(token, p, "include=")) {
                add_inc(p);
                continue;
            }
            break;

        case 'd':
            if (get_arg(token, p, "define=")) {
                add_def(p);
                continue;
            }
            break;

        case 'p':
            if (get_arg(token, p, "prefix=")) {
                prefix(p);
                continue;
            } else if (get_arg(token, p, "param=")) {
                parameter_names( param_value(p, "") );
                continue;
            }
            break;

        default:
            break;
        }

        throw gendlopen::error("unknown %option string: " + token);
    }
}

