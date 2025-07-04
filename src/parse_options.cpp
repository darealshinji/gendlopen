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
#include "parse.hpp"
#include "types.hpp"
#include "utils.hpp"


namespace /* anonymous */
{
    /* get argument from an option string */
    const char *get_arg_len(const std::string &str, const char *opt, const size_t optlen)
    {
        if (strncmp(str.c_str(), opt, optlen) != 0) {
            /* not the argument we're looking for */
            return NULL;
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
}

/* parse `%option' strings */
void parse::options(gendlopen *gdo, const vstring_t &options)
{
    auto set_parameter_names = [&] (const char *opt)
    {
        if (utils::strcasecmp(opt, "skip") == 0) {
            gdo->parameter_names(param::skip);
        } else if (utils::strcasecmp(opt, "create") == 0) {
            gdo->parameter_names(param::create);
        } else if (utils::strcasecmp(opt, "read") == 0) {
            gdo->parameter_names(param::read);
        } else {
            throw gendlopen::error("unknown argument for option 'param': " + std::string(opt));
        }
    };

    const char *p = NULL;

    for (const auto &token : options)
    {
        switch (token[0])
        {
        case 'f':
            if (get_arg(token, p, "format=")) {
                gdo->format(p);
                continue;
            }
            break;

        case 'n':
            if (token == "no-date") {
                gdo->print_date(false);
                continue;
            } else if (token == "no-pragma-once") {
                gdo->pragma_once(false);
                continue;
            }
            break;

        case 'l':
            if (get_arg(token, p, "library=")) {
                gdo->default_lib(p);
                continue;
            } else if (token == "line") {
                gdo->line_directive(true);
                continue;
            }
            break;

        case 'i':
            if (get_arg(token, p, "include=")) {
                gdo->add_inc(p);
                continue;
            }
            break;

        case 'd':
            if (get_arg(token, p, "define=")) {
                gdo->add_def(p);
                continue;
            }
            break;

        case 'p':
            if (get_arg(token, p, "prefix=")) {
                gdo->prefix(p);
                continue;
            } else if (get_arg(token, p, "param=")) {
                set_parameter_names(p);
                continue;
            }
            break;

        default:
            break;
        }

        throw gendlopen::error("unknown %option string: " + token);
    }
}
