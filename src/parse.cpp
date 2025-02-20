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

#include <algorithm>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>
#include "gendlopen.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "parse.hpp"

#define TYPE    " "
#define POINTER "*"
#define SYMBOL  "$"  /* parse::ID */


namespace /* anonymous */
{
    typedef struct _seq {
        const char *front;   /* only TYPE or TYPE + SYMBOL */
        const char *middle;  /* the part where the iterator is */
        const char end;      /* last element */
        const int length;    /* pattern sequence length in bytes */
        const int iter_pos;  /* iterator position within pattern sequence */
    } seq_t;


    /* compare vector elements to pattern sequence */
    bool pat(vstring_t &v, iter_t it, const seq_t &sq)
    {
        /* iterator cannot be begin or end */
        if (it == v.begin() || it == v.end() || sq.iter_pos < 1) {
            return false;
        }

        /* check vector dimensions */
        if (static_cast<long>(v.size()) < sq.length ||     /* minimum vector size */
            std::distance(v.begin(), it) < sq.iter_pos ||  /* iterator distance to vector begin */
            std::distance(it, v.end()) < (sq.length - sq.iter_pos))  /* iterator distance to vector end */
        {
            return false;
        }

        /* must begin with ID */
        if (v.front().at(0) != parse::ID) {
            return false;
        }

        /* "-1" because sq.front doesn't
         * contain a leading TYPE entry */
        const int offset = sq.iter_pos - 1;

        /* apply offset */
        it -= offset;

        /* check sequence */

        for (const char *p = sq.front; *p != 0; p++, it++) {
            if ((*it).front() != *p) {
                return false;
            }
        }

        for (const char *p = sq.middle; *p != 0; p++, it++) {
            if ((*it).front() != *p) {
                return false;
            }
        }

        if (v.back()[0] != sq.end) {
            return false;
        }

        return true;
    }


    /* print all tokens */
    void print_tokens(const vstring_t &v)
    {
        std::cerr << "tokens:";

        for (const auto &e : v) {
            if (e.front() == parse::ID) {
                std::cerr << ' ' << (e.c_str() + 1);
            } else {
                std::cerr << ' ' << e;
            }
        }

        std::cerr << std::endl;
    }


    /* print function name */
    void print_pretty_function(const proto_t &proto)
    {
        std::string type   = proto.type;
        std::string symbol = proto.symbol;
        std::string args   = proto.args;

        utils::strip_spaces(type);
        utils::strip_spaces(args);

        std::erase(type,   parse::ID);
        std::erase(symbol, parse::ID);
        std::erase(args,   parse::ID);

        std::cerr << type << ' ' << symbol << '(' << args << ");\n";
    }


    /* cosmetics on the output */
    void format_prototype(std::string &s)
    {
        utils::strip_spaces(s);
        std::erase(s, parse::ID);

        utils::replace("( ", "(", s);
        utils::replace(") ", ")", s);
        utils::replace(" )", ")", s);

        utils::replace(" [", "[", s);
        utils::replace(" ]", "]", s);
        utils::replace("[ ", "[", s);
        utils::replace("] ", "]", s);

        utils::replace(" ,", ",", s);
        utils::replace("* ", "*", s);
    }


    /* check for multiple definitions of a symbol */
    bool multiple_definitions(const vproto_t &vproto, std::string &symbol)
    {
        /* https://stackoverflow.com/a/72800146/5687704 */
        for (auto i = vproto.begin(); i != vproto.end(); ++i) {
            for (auto j = vproto.begin(); i != j; ++j) {
                if ((*i).symbol == (*j).symbol) {
                    symbol = (*i).symbol;
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
        std::string buf;
        vstring_t tokens;

        for (auto it = it_beg; it != it_end; it++)
        {
            switch ((*it).front())
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
                    buf.clear();
                    tokens.clear();
                    continue;
                }
                break;
            default:
                break;
            }

            tokens.push_back(*it);
            buf += *it + ' ';
        }

        if (!buf.empty()) {
            proto.args_vec.push_back(tokens);
        }
    }


    bool check_object_prototype(vstring_t &v, proto_t &p, iter_t &it)
    {
        if (!parse::is_object(v, it)) {
            return false;
        }

        p.prototype = proto::object;

        /* from begin to symbol name */
        parse::append_strings(p.type, v.begin(), v.end()-1);

        p.symbol = v.back();

        return true;
    }


    bool check_array_prototype(vstring_t &v, proto_t &p, iter_t &it)
    {
        if (!parse::is_array(v, it)) {
            return false;
        }

        p.prototype = proto::object_array;

        /* from begin to symbol name */
        parse::append_strings(p.type, v.begin(), it - 1);

        /* symbol name */
        p.symbol = *(it - 1);

        /* from bracket to end */
        parse::append_strings(p.type, it, v.end());

        return true;
    }


    bool check_function_pointer_prototype(vstring_t &v, proto_t &p, iter_t &it)
    {
        if (!parse::is_function_pointer(v, it)) {
            return false;
        }

        p.prototype = proto::function_pointer;

        /* from begin to lparen */
        parse::append_strings(p.type, v.begin(), it);

        p.type += "(*)";

        /* symbol name */
        p.symbol = *(it + 2);

        /*  parameters:              */
        /*  type (   * symbol ) ( )  */
        /*       ^ + 1 2      3 4    */
        parse::append_strings(p.type, it + 4, v.end());

        return true;
    }


    /* function name within parentheses */
    bool check_function_prototype_with_parentheses(vstring_t &v, proto_t &p, iter_t &it)
    {
        if (!parse::is_function_with_parentheses(v, it)) {
            return false;
        }

        p.prototype = proto::function;

        /* from begin to lparen */
        parse::append_strings(p.type, v.begin(), it);

        /* symbol name */
        p.symbol = *(it + 1);

        /*  parameters:                 */
        /*  type (   symbol ) ( <..> )  */
        /*       ^ + 1      2 3 4       */
        copy_parameters(it + 4, v.end() - 1, p);

        return true;
    }


    bool check_function_prototype(vstring_t &v, proto_t &p, iter_t &it)
    {
        /* check for function name within parentheses first */
        if (check_function_prototype_with_parentheses(v, p, it)) {
            return true;
        }

        if (!parse::is_function(v, it)) {
            return false;
        }

        p.prototype = proto::function;

        /* from begin to symbol name */
        parse::append_strings(p.type, v.begin(), it - 1);

        /* symbol name */
        p.symbol = *(it - 1);

        /* parameters */
        copy_parameters(it + 1, v.end() - 1, p);

        return true;
    }


    /* parse tokens vector and save prototypes */
    bool parse_tokens(std::vector<vstring_t> &vec_tokens, vproto_t &vproto)
    {
        for (vstring_t &v : vec_tokens) {
            if (v.size() >= 2 && v.front().at(0) == parse::ID) {
                iter_t it = parse::find_first_not_pointer_or_ident(v);
                proto_t p;

                /* check for function pointer first! */
                if (check_function_pointer_prototype(v, p, it) ||
                    check_function_prototype(v, p, it) ||
                    check_object_prototype(v, p, it) ||
                    check_array_prototype(v, p, it))
                {
                    vproto.push_back(p);
                    continue;
                }
            }

            print_tokens(v);
            return false;
        }

        return true;
    }


    /* create a seq_t struct */
    template<size_t sz_front, size_t sz_mid>
    constexpr seq_t seq(char const (&front)[sz_front], char const (&mid)[sz_mid], char const (&end)[2])
    {
        /* strlen(front) + strlen(mid) + strlen(end) */
        constexpr const int len = (sz_front-1) + (sz_mid-1) + 1;

        const seq_t sq = {
            .front    = front + 1,  /* skip leading TYPE element */
            .middle   = mid,
            .end      = end[0],
            .length   = len,
            .iter_pos = sz_front - 1
        };

        return sq;
    }

} /* end anonymous namespace */


bool parse::is_function_pointer(vstring_t &v, const iter_t &it)
{
    return pat(v, it, seq(
        TYPE,  "(" POINTER SYMBOL ")"  "(", ")"
    ));
}


bool parse::is_function_pointer_no_name(vstring_t &v, const iter_t &it)
{
    return pat(v, it, seq(
        TYPE,  "(" POINTER ")"  "(", ")"
    ));
}


bool parse::is_function_with_parentheses(vstring_t &v, const iter_t &it)
{
    return pat(v, it, seq(
        TYPE,  "(" SYMBOL ")"  "(", ")"
    ));
}


bool parse::is_function(vstring_t &v, const iter_t &it)
{
    return pat(v, it, seq(
        TYPE SYMBOL, "(", ")"
    ));
}


bool parse::is_array(vstring_t &v, const iter_t &it)
{
    return pat(v, it, seq(
        TYPE SYMBOL, "[", "]"
    ));
}


bool parse::is_array_no_name(vstring_t &v, const iter_t &it)
{
    return pat(v, it, seq(
        TYPE, "[", "]"
    ));
}


bool parse::is_object(vstring_t &v, const iter_t &it)
{
    /* TYPE SYMBOL */
    return (it == v.end() &&              /* only IDs and pointers */
            v.size() >= 2 &&              /* type + symbol */
            v.front().at(0) == parse::ID &&  /* type must begin with ID */
            v.back().at(0) == parse::ID);    /* symbol */
}


void gendlopen::filter_and_copy_symbols(vproto_t &vproto)
{
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

    create_typedefs();

    /* cosmetics */

    auto format = [] (vproto_t &vec) {
        for (auto &e : vec) {
            e.symbol.erase(0, 1); // erase parse::ID
            std::erase(e.notype_args, parse::ID);
            format_prototype(e.type);
            format_prototype(e.args);
        }
    };

    format(m_prototypes);
    format(m_objects);

    for (auto &e : m_typedefs) {
        format_prototype(e);
    }
}


void gendlopen::parse(std::vector<vstring_t> &vec_tokens, vstring_t &options, vproto_t &vproto, std::string &input_name)
{
    std::string sym, msg;

    /* parse `%options' strings */
    parse_options(options);

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
            if (!parse::create_names(proto, msg)) {
                throw error(input_name + '\n' + msg);
            }
        }
    } else {
        /* read parameter names */
        for (auto &proto : vproto) {
            if (!parse::read_and_copy_names(proto, m_parameter_names)) {
                print_pretty_function(proto);
                throw error(input_name + "\nfailed to read function parameter names");
            }
        }
    }

    /* copy */
    filter_and_copy_symbols(vproto);
}
