/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2025 Carsten Janssen

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

#pragma once

#include <iterator>
#include <string>
#include "types.hpp"

class gendlopen;


namespace parse
{
    /* append strings, separated by space */
    template <typename T>
    void append_strings(std::string &buf, const T &it_beg, const T &it_end) {
        for (auto it = it_beg; it != it_end; it++) {
            buf += *it;
            buf += ' ';
        }
    }

    /* parse_parameters.cpp */
    bool get_parameter_names(proto_t &proto, param::names &parameter_names, std::string &msg);
    bool create_parameter_names(proto_t &proto, std::string &msg);

    /* parse_options.cpp */
    void options(gendlopen *gdo, const vstring_t &options);

    /* parse.cpp */
    bool is_function_pointer(vstring_t &v, const iter_t &i);
    bool is_function_pointer_no_name(vstring_t &v, const iter_t &i);
    bool is_function_parentheses(vstring_t &v, const iter_t &i);
    bool is_function_paren_pointer1(vstring_t &v, const iter_t &i);
    bool is_function_paren_pointer2(vstring_t &v, const iter_t &i);
    bool is_function(vstring_t &v, const iter_t &i);
    bool is_array(vstring_t &v, const iter_t &i);
    bool is_array_no_name(vstring_t &v, const iter_t &it);
    bool is_object(vstring_t &v, const iter_t &it);

    /* find first element that isn't an identifier or pointer */
    vstring_t::iterator find_first_not_pointer_or_ident(vstring_t &v);
}

