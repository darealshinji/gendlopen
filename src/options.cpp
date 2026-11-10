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

#ifdef _WIN32
# define OPT_ENABLE_ALL
#else
# define OPT_PREFIX_SHORT
# define OPT_PREFIX_LONG
# define OPT_SEPARATOR_EQUAL
#endif
//#define OPT_ENABLE_ASSERT
#include "opt_parser.h"



class opt
{
private:

    struct opt_args m_args;

    bool get_arg(const char *&ptr, const char *opt, const size_t &optlen)
    {
        if (opt_get_arg(&m_args, opt, optlen)) {
            ptr = m_args.value;
            return true;
        }

        ptr = NULL;
        return false;
    }

public:

    opt(const int &argc, char ** const &argv)
    {
        auto callback = [] (const char *msg) {
            throw gendlopen::error_cmd(msg);
        };

        opt_init(&m_args, argc, argv, 0, callback);
    }

    bool iterate()              { return opt_iterate(&m_args); }
    const char *current() const { return m_args.current; }
    uint8_t pfxlen() const      { return m_args.pfxlen; }
    bool flag(const char *str)  { return opt_arg_eq(&m_args, str); }

    template<size_t N>
    bool arg(const char *&ptr, char const (&opt)[N]) {
        return get_arg(ptr, opt, N-1);
    }
};


namespace /* anonymous */
{
    const char *get_option_len(const std::string &token, const char *opt, const size_t optlen)
    {
        if (token.compare(0, optlen, opt) == 0) {
            if (token.size() > optlen) {
                return token.c_str() + optlen;
            }

            throw gendlopen::error("%option string requires an argument: " + token);
        }

        return NULL;
    }

    template<size_t N>
    bool get_option(const std::string &token, const char *&ptr, char const (&opt)[N])
    {
        return ((ptr = get_option_len(token, opt, N-1)) != NULL);
    }
}


/* parse command line arguments; reminder: empty arguments always throw an exception */
void gendlopen::parse_cmdline(const int &argc, char ** const &argv)
{
    const char *input_file = NULL;
    const char *p = NULL;

    opt o(argc, argv);

    while (o.iterate()) {
        if (o.pfxlen() == 0) {
            /* use first non-option argument as input file, otherwise warn */
            if (!input_file) {
                input_file = o.current();
            } else {
                std::cerr << "warning: non-option argument ignored: " << o.current() << std::endl;
            }
        } else if (o.flag("?") || o.flag("help")) {
            throw E_HELP;
        } else if (o.flag("full-help")) {
            throw E_FULLHELP;
        } else if (o.flag("ast-all-symbols")) {
            ast_all_symbols(true);
        } else if (o.arg(p, "D")) {
            add_def(p);
        } else if (o.arg(p, "dump-templates")) {
            dump_path(p);
            return;
        } else if (o.arg(p, "format")) {
            format(p);
        } else if (o.flag("force")) {
            force(true);
        } else if (o.arg(p, "include")) {
            add_inc(p);
        } else if (o.flag("ignore-options")) {
            read_options(false);
        } else if (o.arg(p, "library")) {
            default_lib(p);
        } else if (o.flag("line")) {
            line_directive(true);
        } else if (o.flag("no-date")) {
            print_date(false);
        } else if (o.flag("no-pragma-once")) {
            pragma_once(false);
        } else if (o.arg(p, "out")) {
            output(p);
        } else if (o.arg(p, "P")) {
            add_pfx(p);
        } else if (o.arg(p, "prefix")) {
            prefix(p);
        } else if (o.arg(p, "param")) {
            parameter_names(p);
        } else if (o.flag("print-symbols")) {
            print_symbols(true);
        } else if (o.arg(p, "S")) {
            add_sym(p);
        } else if (o.flag("separate")) {
            separate(true);
        } else if (o.arg(p, "template")) {
            custom_template(p);
        } else if (o.arg(p, "templates-path")) {
            templates_path(p);
        } else if (o.flag("version")) {
            throw E_VERSION;
        } else {
            throw error_cmd(std::string("unknown option: ") + o.current());
        }
    }

    if (!input_file || *input_file == 0) {
        throw error_cmd("input file is required");
    }

    input(input_file);
}


/* parse `%option' strings */
void gendlopen::parse_options(const vstring_t &options)
{
    const char *p = NULL;

    for (const auto &token : options) {
        if (token == "line") {
            line_directive(true);
        } else if (token == "no-date") {
            print_date(false);
        } else if (token == "no-pragma-once") {
            pragma_once(false);
        } else if (get_option(token, p, "D=")) {
            add_def(p);
        } else if (get_option(token, p, "format=")) {
            format(p);
        } else if (get_option(token, p, "include=")) {
            add_inc(p);
        } else if (get_option(token, p, "library=")) {
            default_lib(p);
        } else if (get_option(token, p, "param=")) {
            parameter_names(p);
        } else if (get_option(token, p, "prefix=")) {
            prefix(p);
        } else {
            throw error("unknown %option string: " + token);
        }
    }
}

