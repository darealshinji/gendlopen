/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2025-2026 Carsten Janssen

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

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "gendlopen.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "parse.hpp"


namespace /* anonymous */
{
    /* print all tokens */
    void print_tokens(const vstring_t &v)
    {
        std::cerr << "tokens:";

        for (const auto &e : v) {
            std::cerr << ' ' << e;
        }

        std::cerr << std::endl;
    }


    /* check for multiple definitions of a symbol */
    bool multiple_definitions(const vproto_t &vproto, std::string &buf)
    {
        /* https://stackoverflow.com/a/72800146/5687704 */
        for (auto i = vproto.begin(); i != vproto.end(); ++i) {
            for (auto j = vproto.begin(); i != j; ++j) {
                if ((*i).symbol == (*j).symbol) {
                    buf = (*i).symbol;
                    return true;
                }
            }
        }

        return false;
    }


    /* copy function parameters, don't mess up function pointers */
    void copy_parameters(const iter_t &it_beg, const iter_t &it_end, proto_t &proto)
    {
        int scope = 0;
        vstring_t tokens;

        for (auto it = it_beg; it != it_end; it++)
        {
            switch (utils::str_front(*it))
            {
            case '(':
                scope++;
                break;
            case ')':
                scope--;
                break;
            case ',':
                if (scope == 0) {
                    proto.args_vec.push_back(tokens);
                    tokens.clear();
                    continue;
                }
                break;
            default:
                break;
            }

            tokens.push_back(*it);
        }

        if (!tokens.empty()) {
            proto.args_vec.push_back(tokens);
        }
    }


    /* TYPE SYMBOL */
    bool check_object(vstring_t &v, proto_t &p)
    {
        if (!parse::is_object(v, v.end())) {
            return false;
        }

        p.prototype = proto::object;
        parse::append_strings(p.type, v.begin(), v.end() - 1 /* symbol name */);
        p.symbol = v.back();

        return true;
    }


    /* TYPE SYMBOL [ ] */
    bool check_array(vstring_t &v, proto_t &p, iter_t &it)
    {
        if (!parse::is_array(v, it)) {
            return false;
        }

        p.prototype = proto::object_array;
        parse::append_strings(p.type, v.begin(), it - 1 /* symbol name */);
        parse::append_strings(p.type, it /* first bracket */, v.end());
        p.symbol = *(it - 1);

        return true;
    }


    /* TYPE ( * SYMBOL ) ( ) */
    bool check_function_pointer(vstring_t &v, proto_t &p, iter_t &it)
    {
        if (!parse::is_function_pointer(v, it)) {
            return false;
        }

        p.prototype = proto::function_pointer;

        /*  type (   * symbol ) ( )  */
        /*       ^ + 1 2      3 4    */
        parse::append_strings(p.type, v.begin(), it /* first lparen */);
        p.type += "(*)";
        parse::append_strings(p.type, it + 4 /* param begin */, v.end());

        p.symbol = *(it + 2);

        return true;
    }


    /* TYPE ( SYMBOL ) ( )  <- this may be the result of a macro expansion */
    bool check_function_paren(vstring_t &v, proto_t &p, iter_t &it)
    {
        if (!parse::is_function_parentheses(v, it)) {
            return false;
        }

        p.prototype = proto::function;
        parse::append_strings(p.type, v.begin(), it /* first lparen */);
        p.symbol = *(it + 1);

        /*  type (   symbol ) ( <..> )  */
        /*       ^ + 1      2 3 4       */
        copy_parameters(it + 4, v.end() - 1, p);

        return true;
    }


    /* TYPE ( ** SYMBOL ( ) )  <- these can be found in the output of "gcc -aux-info" */
    bool check_function_paren_pointer(vstring_t &v, proto_t &p, iter_t &it)
    {
        int n;

        if (parse::is_function_paren_pointer1(v, it)) {
            n = 1; /* 1 pointer */
        } else if (parse::is_function_paren_pointer2(v, it)) {
            n = 2; /* 2 pointers */
        } else {
            return false;
        }

        p.prototype = proto::function;
        parse::append_strings(p.type, v.begin(), it /* first lparen */);
        parse::append_strings(p.type, it + 1, it + 1 + n);
        p.symbol = *(it + 1 + n);

        /*  type (   ** symbol ( <..>  )  )  */
        /*       ^ + n  1      2 3    -2 -1  */
        copy_parameters(it + n + 3, v.end() - 2, p);

        return true;
    }


    /* TYPE SYMBOL ( ) */
    bool check_function(vstring_t &v, proto_t &p, iter_t &it)
    {
        if (!parse::is_function(v, it)) {
            return false;
        }

        p.prototype = proto::function;
        parse::append_strings(p.type, v.begin(), it - 1 /* symbol name */);
        p.symbol = *(it - 1);
        copy_parameters(it + 1 /* first lparen */, v.end() - 1, p);

        return true;
    }


    /* check and parse tokens, save prototype */
    bool check_prototype(vstring_t &v, proto_t &p)
    {
        /* minimum size is 2 (type + symbol) */
        if (v.size() < 2) {
            return false;
        }

        /* first element must be an identifier */
        if (!parse::is_ident(utils::str_front(v.front()))) {
            return false;
        }

        iter_t it = parse::find_first_not_pointer_or_ident(v);

        if (it == v.end()) {
            return check_object(v, p);
        }

        switch (utils::str_front(*it))
        {
        case '(':
            return (check_function_paren(v, p, it)         || /* TYPE ( SYMBOL ) ( ) */
                    check_function_paren_pointer(v, p, it) || /* TYPE ( * SYMBOL ( ) ) */
                    check_function(v, p, it)               || /* TYPE SYMBOL ( ) */
                    check_function_pointer(v, p, it));        /* TYPE ( * SYMBOL ) ( ) */

        case '[':
            return check_array(v, p, it);

        default:
            break;
        }

        return false;
    }


    /* parse tokens vector and save prototypes */
    bool parse_tokens(std::vector<vstring_t> &vec_tokens, vproto_t &vproto)
    {
        for (vstring_t &v : vec_tokens) {
            proto_t p;

            if (!check_prototype(v, p)) {
                print_tokens(v);
                return false;
            }

            vproto.push_back(p);
        }

        return true;
    }

} /* end anonymous namespace */


void gendlopen::parse(std::vector<vstring_t> &vec_tokens, const std::string &input_name)
{
    std::string sym, msg;
    vproto_t vproto;

    /* parse tokens */
    if (!parse_tokens(vec_tokens, vproto)) {
        throw error(input_name + "\nfailed to read prototypes");
    }

    /* nothing found? */
    if (vproto.empty()) {
        throw error("no function or object prototypes found in " + input_name);
    }

    /* check for multiple definitions of a symbol */
    if (multiple_definitions(vproto, sym)) {
        throw error("multiple definitions of symbol `" + sym + "' found in " + input_name);
    }

    /* parameters */
    if (m_parameter_names == param::create) {
        /* create parameter names */
        for (auto &proto : vproto) {
            if (!parse::create_parameter_names(proto, msg)) {
                throw error(input_name + '\n' + msg);
            }
        }
    } else {
        /* param::read or param::skip */
        for (auto &proto : vproto) {
            if (!parse::get_parameter_names(proto, m_parameter_names, msg)) {
                throw error(input_name + '\n' + msg);
            }
        }
    }

    /* filter and copy symbols */

    auto save_symbol = [this] (const proto_t &p)
    {
        if (p.prototype == proto::function) {
            m_prototypes.push_back(p);
        } else {
            m_objects.push_back(p);
        }
    };

    if (m_prefix_list.empty() && m_symbol_list.empty()) {
        /* copy all symbols */
        for (const auto &e : vproto) {
            save_symbol(e);
        }
    } else {
        /* copy prefixed symbols */
        if (!m_prefix_list.empty()) {
            for (const auto &e : vproto) {
                if (utils::is_prefixed(e.symbol, m_prefix_list)) {
                    save_symbol(e);
                }
            }
        }

        /* copy whitelisted symbols */
        if (!m_symbol_list.empty()) {
            auto it_beg = m_symbol_list.begin();
            auto it_end = m_symbol_list.end();

            for (const auto &e : vproto) {
                /* look for item e.symbol in list m_symbol_list */
                if (std::find(it_beg, it_end, e.symbol) != it_end) {
                    save_symbol(e);
                }
            }
        }
    }

    /* nothing found? */
    if (m_prototypes.empty() && m_objects.empty()) {
        throw error("no function or object prototypes found in " + input_name);
    }
}
