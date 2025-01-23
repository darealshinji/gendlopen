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

#include <string>
#include "types.hpp"


namespace parse
{
    enum { ID = '$' };

    /* parse.cpp */
    void append_strings(std::string &buf, const iter_t &it_beg, const iter_t &it_end);
    iter_t find_first_not_pointer_or_ident(vstring_t &v);
    bool is_function_pointer(vstring_t &v, const iter_t &i);
    bool is_function_pointer_no_name(vstring_t &v, const iter_t &i);
    bool is_function_with_parentheses(vstring_t &v, const iter_t &i);
    bool is_function(vstring_t &v, const iter_t &i);
    bool is_array(vstring_t &v, const iter_t &i);

    /* parameters.cpp */
    bool read_and_copy_names(proto_t &proto, param::names &parameter_names);
    bool create_names(proto_t &proto, std::string &msg);
}
