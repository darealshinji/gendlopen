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

#ifdef _MSC_VER
# include "strcasecmp.hpp"
#else
# include <strings.h>
#endif
#include <string>
#include <vector>
#include "parse.hpp"
#include "types.hpp"
#include "utils.hpp"

#define POINTER    "*"
#define TRIPLE_DOT "..."


namespace /* anonymous */
{
    /* check if prototype parameters are empty or of type "void"
     * and save "void" parameter if true */
    bool param_void_or_empty(proto_t &proto)
    {
        if (proto.args_vec.empty()) {
            /* no parameters */
            return true;
        }

        /* must be 1 parameter */
        if (proto.args_vec.size() != 1) {
            return false;
        }

        auto &v = proto.args_vec.front();

        /* must be 1 token */
        if (v.size() != 1) {
            return false;
        }

        if (strcasecmp(v.front().c_str(), "void") == 0) {
            /* "void" parameter */
            proto.args = v.front();
            return true;
        }

        /* regular parameters */
        return false;
    }


    void append_name(proto_t &proto, int &param_count, const char *seperator)
    {
        std::string name = "a" + std::to_string(param_count);
        proto.args += name + seperator;
        proto.notype_args += name + ", ";

        /* iterate counter */
        param_count++;
    }


    bool get_array_type(vstring_t &v, iter_t &it, proto_t &proto, int &param_count)
    {
        if (!parse::is_array_no_name(v, it)) {
            return false;
        }

        /*  type [ ]   */
        /*       ^iter */
        parse::append_strings(proto.args, v.begin(), it);
        append_name(proto, param_count, " ");
        parse::append_strings(proto.args, it, v.end());
        proto.args += ", ";

        return true;
    }


    bool get_function_pointer_type(vstring_t &v, iter_t &it, proto_t &proto, int &param_count)
    {
        int offset;

        if (parse::is_function_pointer(v, it)) {
            /* guessing the name should be save */
            /*  type (       * name ) ( )  */
            /*       ^iter + 1 2    3      */
            offset = 3;
        } else if (parse::is_function_pointer_no_name(v, it)) {
            /*  type (       * ) ( )  */
            /*       ^iter + 1 2      */
            offset = 2;
        } else {
            return false;
        }

        parse::append_strings(proto.args, v.begin(), it + 2);
        append_name(proto, param_count, " ");
        parse::append_strings(proto.args, it + offset, v.end());
        proto.args += ", ";

        return true;
    }

} /* end anonymous namespace */


/* get parameter names from function parameter list */
bool parse::read_and_copy_names(proto_t &proto, param::names &parameter_names, std::string &msg)
{
    if (proto.prototype != proto::function || param_void_or_empty(proto)) {
        /* nothing to do */
        return true;
    }

    /* copy parameters */
    for (auto &v : proto.args_vec) {
        append_strings(proto.args, v.begin(), v.end());
        proto.args += ", ";
    }

    utils::delete_suffix(proto.args, ", ");
    utils::strip_spaces(proto.args);

    if (parameter_names == param::skip) {
        /* `-param=skip' was given */
        return true;
    }

    /* get parameter names */
    for (vstring_t &v : proto.args_vec)
    {
        if (v.empty()) {
            msg = "empty parameter in function `" + proto.symbol + "'";
            return false;
        } else if (v.size() == 1) {
            /* check for `...' */
            if (v.back() == TRIPLE_DOT) {
                proto.notype_args += TRIPLE_DOT ", ";
                continue;
            }

            msg = "typename only or incorrect parameter format in function `" + proto.symbol + "'";
            return false;
        }

        /* check if a parameter begins with `*' */
        if (v.front() == POINTER) {
            msg = "parameter in function `" + proto.symbol + "' begins with pointer `*'";
            return false;
        }

        iter_t it = find_first_not_pointer_or_ident(v);

        if (is_object(v, it)) {
            proto.notype_args += v.back() + ", ";
            continue;
        } else if (is_function_pointer(v, it)) {
            /* type (     * symbol ) ( ) */
            /*      ^it + 1 2            */
            proto.notype_args += *(it + 2) + ", ";
            continue;
        } else if (is_array(v, it)) {
            /* type symbol [   ] */
            /*      -1     ^it   */
            proto.notype_args += *(it - 1) + ", ";
            continue;
        }

        msg = "incorrect parameter format in function `" + proto.symbol + "'";
        return false;
    }

    utils::delete_suffix(proto.notype_args, ", ");

    return true;
}


/* create parameter names; */
/* unless type is a function pointer, don't make any assumptions which
 * element could be the name (it could also be a keyword like `const') */
bool parse::create_names(proto_t &proto, std::string &msg)
{
    int param_count = 1;

    if (proto.prototype != proto::function || param_void_or_empty(proto)) {
        /* nothing to do */
        return true;
    }

    for (auto &v : proto.args_vec)
    {
        if (v.empty()) {
            msg = "empty parameter in function `" + proto.symbol + "'";
            return false;
        }

        /* check if a parameter begins with `*' */
        if (v.front() == POINTER) {
            msg = "parameter in function `" + proto.symbol + "' begins with pointer";
            return false;
        }

        /* don't append anything to `...' */
        if (v.size() == 1 && v.back() == TRIPLE_DOT) {
            proto.args += TRIPLE_DOT ", ";
            proto.notype_args += TRIPLE_DOT ", ";
            continue;
        }

        /* function pointer and array types */
        iter_t it = parse::find_first_not_pointer_or_ident(v);

        if (it != v.end()) {
            if (get_function_pointer_type(v, it, proto, param_count) ||
                get_array_type(v, it, proto, param_count))
            {
                continue;
            }

            msg = "cannot read parameter in function `" + proto.symbol + "'";
            return false;
        }

        /* regular object or pointer */
        append_strings(proto.args, v.begin(), v.end());
        append_name(proto, param_count, ", ");
    }

    utils::delete_suffix(proto.args, ", ");
    utils::delete_suffix(proto.notype_args, ", ");

    return true;
}
