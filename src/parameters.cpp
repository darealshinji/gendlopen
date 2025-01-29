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

        /* check for "void" parameter */
        if (proto.args_vec.size() == 1) {
            /* only 1 parameter entry (token list) */
            auto &v = proto.args_vec.at(0);

            if (v.size() == 1) {
                /* only 1 token */

                if (v.at(0).empty()) {
                    /* no parameters */
                    return true;
                }

                const char *str = v.at(0).c_str();

                if (*str == parse::ID && strcasecmp(str+1, "void") == 0) {
                    /* "void" parameter */
                    proto.args = v.at(0);
                    return true;
                }
            }
        }

        /* regular parameters */
        return false;
    }


    void append_name(proto_t &proto, char &name)
    {
        proto.args += name;
        proto.args += ' ';
        proto.notype_args += name;
        proto.notype_args += ", ";
        name++;
    }


    bool get_array_type(vstring_t &v, proto_t &proto, char &name)
    {
        auto i = parse::find_first_not_pointer_or_ident(v);

        if (parse::is_array(v, i)) {
            /*  type [ ]  */
            /*       ^i   */
            parse::append_strings(proto.args, v.begin(), i);
            append_name(proto, name);
            parse::append_strings(proto.args, i, v.end());
            proto.args += ", ";

            return true;
        }

        return false;
    }


    bool get_function_pointer_type(vstring_t &v, proto_t &proto, char &name)
    {
        auto i = parse::find_first_not_pointer_or_ident(v);

        if (parse::is_function_pointer(v, i)) {
            /* guessing the name should be save */
            /*  type ( * name ) ( )  */
            /*       ^i              */
            parse::append_strings(proto.args, v.begin(), i+2);
            append_name(proto, name);
            parse::append_strings(proto.args, i+3, v.end());
            proto.args += ", ";
            return true;
        } else if (parse::is_function_pointer_no_name(v, i)) {
            /*  type ( * ) ( )  */
            /*       ^i         */
            parse::append_strings(proto.args, v.begin(), i+1);
            append_name(proto, name);
            parse::append_strings(proto.args, i+2, v.end());
            proto.args += ", ";
            return true;
        }

        return false;
    }
} /* end anonymous namespace */


/* get parameter names from function parameter list */
bool parse::read_and_copy_names(proto_t &proto, param::names &parameter_names)
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
    for (auto &v : proto.args_vec) {
        iter_t i;
        char last = v.back()[0];

        if (v.size() == 1 && last != '.') {
            /* typename only */
            return false;
        }

        switch (last)
        {
        case ID:
            /* identifier */
            proto.notype_args += v.back();
            break;

        case '*':
            /* pointer, no param name */
            return false;

        case ')':
            /* function pointer */
            i = find_first_not_pointer_or_ident(v);

            if (is_function_pointer(v, i)) {
                /* type (   * symbol ) ( ) */
                /*      ^ + 1 2            */
                proto.notype_args += *(i + 2);
                break;
            }
            return false;

        case ']':
            /* array */
            i = find_first_not_pointer_or_ident(v);

            if (is_array(v, i)) {
                proto.notype_args += *(i - 1);
                break;
            }
            return false;

        case '.':
            /* `...' (single dots aren't lexed) */
            proto.notype_args += v.back();
            break;

        default:
            return false;
        }

        proto.notype_args += ", ";
    }

    utils::delete_suffix(proto.notype_args, ", ");

    return true;
}


/* create parameter names `a-z'; */
/* unless type is function pointer, don't make any assumptions on which
 * element could be the name (it could also be a keyword like `const') */
bool parse::create_names(proto_t &proto, std::string &msg)
{
    char name = 'a';

    if (proto.prototype != proto::function || param_void_or_empty(proto)) {
        /* nothing to do */
        return true;
    }

    for (auto &v : proto.args_vec)
    {
        if (name > 'z') {
            msg = "too many parameters to handle in function `" + proto.symbol + "'";
            return false;
        }

        switch (v.back()[0])
        {
        case ')':
            /* function pointer */
            if (!get_function_pointer_type(v, proto, name)) {
                msg = "failed to read function pointer type `" + proto.symbol + "'";
                return false;
            }
            continue;
        case ']':
            /* array */
            if (!get_array_type(v, proto, name)) {
                msg = "failed to read array type `" + proto.symbol + "'";
                return false;
            }
            continue;
        default:
            break;
        }

        /* copy parameters */
        append_strings(proto.args, v.begin(), v.end());

        /* don't append anything to `...' */
        if (v.size() == 1 && v.back() == "...") {
            proto.args += ", ";
            proto.notype_args += "... , ";
            continue;
        }

        /* add name */
        proto.args += name;
        proto.args += ", ";
        proto.notype_args += name;
        proto.notype_args += ", ";

        name++;
    }

    utils::delete_suffix(proto.args, ", ");
    utils::delete_suffix(proto.notype_args, ", ");

    return true;
}
