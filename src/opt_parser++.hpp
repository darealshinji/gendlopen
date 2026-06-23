/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2026 Carsten Janssen

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

#ifndef _OPT_PARSERPP_HPP_
#define _OPT_PARSERPP_HPP_

#include "opt_parser.h"


/* small convenient C++ class wrapped around "opt_parser.h" */

class opt
{
public:

    typedef void (*errhandle_t)(const char *msg);

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

    opt(const int &argc, char ** const &argv, errhandle_t cb = nullptr) {
        opt_init(&m_args, argc, argv, 0, cb);
    }

    ~opt() {}

    bool iterate() {
        return opt_iterate(&m_args);
    }

    const char *current() const {
        return m_args.current;
    }

    uint8_t pfxlen() const {
        return m_args.pfxlen;
    }

    bool flag(const char *str) {
        return opt_arg_eq(&m_args, str);
    }

    template<size_t N>
    bool arg(const char *&ptr, char const (&opt)[N]) {
        return get_arg(ptr, opt, N-1);
    }
};

#endif //_OPT_PARSERPP_HPP_
