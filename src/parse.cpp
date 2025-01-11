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
#include <vector>
#include "global.hpp"


#define IT(OFFSET) (*(i + OFFSET)).front()


namespace /* anonymous */
{

/* print all tokens */
void print_tokens(const vstring_t &v)
{
    std::cerr << "error:";

    for (const auto &e : v) {
        std::cerr << ' ' << e;
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

    for (auto i = it_beg; i != it_end; i++)
    {
        switch (IT(0))
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

        tokens.push_back(*i);
        buf += *i + ' ';
    }

    if (!buf.empty()) {
        proto.args_vec.push_back(tokens);
    }
}


/* check vector dimensions */
bool check_bounds(vstring_t &v, const long &min_size, const iter_t &i, const long &iter_pos)
{
    if (i == v.begin() || i == v.end() || iter_pos > min_size) {
        return false;
    }

    if (static_cast<long>(v.size()) < min_size ||
        std::distance(v.begin(), i) < iter_pos ||
        std::distance(i, v.end()) < (min_size - iter_pos))
    {
        return false;
    }

    return true;
}


/* compare vector elements to pattern sequence */
bool check_pattern(vstring_t &v, const long &min_size, const iter_t &i, const long &iter_pos,
                    const char *seq, const char &end, long off = 0)
{
    if (!check_bounds(v, min_size, i, iter_pos)) {
        return false;
    }

    /* check last element */
    if (v.back()[0] != end) {
        return false;
    }

    /* check rest of sequence */
    for (const char *p = seq; *p != 0; p++, off++) {
        if (IT(off) != *p) {
            return false;
        }
    }

    return true;
}


void save_object_prototype(vstring_t &v, proto_t &proto)
{
    proto.prototype = proto::object;

    /* from begin to symbol name */
    parse::append_strings(proto.type, v.begin(), v.end()-1);

    proto.symbol = v.back();
}


bool save_array_prototype(vstring_t &v, iter_t &it_bracket, proto_t &proto)
{
    if (!parse::is_array(v, it_bracket)) {
        return false;
    }

    proto.prototype = proto::object_array;

    /* from begin to symbol name */
    parse::append_strings(proto.type, v.begin(), it_bracket - 1);

    /* symbol name */
    proto.symbol = *(it_bracket - 1);

    /* from bracket to end */
    parse::append_strings(proto.type, it_bracket, v.end());

    return true;
}


bool save_function_pointer_prototype(vstring_t &v, iter_t &it_lparen, proto_t &proto)
{
    if (!parse::is_function_pointer(v, it_lparen)) {
        return false;
    }

    proto.prototype = proto::function_pointer;

    /* from begin to lparen */
    parse::append_strings(proto.type, v.begin(), it_lparen);

    proto.type += "(*)";

    /* symbol name */
    proto.symbol = *(it_lparen + 2);

    /*  parameters:              */
    /*  type (   * symbol ) ( )  */
    /*       ^ + 1 2      3 4    */
    parse::append_strings(proto.type, it_lparen + 4, v.end());

    return true;
}


/* function name within parentheses */
bool save_function_prototype_with_parentheses(vstring_t &v, iter_t &it_lparen, proto_t &proto)
{
    if (!parse::is_function_with_parentheses(v, it_lparen)) {
        return false;
    }

    proto.prototype = proto::function;

    /* from begin to lparen */
    parse::append_strings(proto.type, v.begin(), it_lparen);

    /* symbol name */
    proto.symbol = *(it_lparen + 1);

    /*  parameters:                 */
    /*  type (   symbol ) ( <..> )  */
    /*       ^ + 1      2 3 4       */
    copy_parameters(it_lparen + 4, v.end() - 1, proto);

    return true;
}


bool save_function_prototype(vstring_t &v, iter_t &it_lparen, proto_t &proto)
{
    /* check for function name within parentheses first */
    if (save_function_prototype_with_parentheses(v, it_lparen, proto)) {
        return true;
    }

    if (!parse::is_function(v, it_lparen)) {
        return false;
    }

    proto.prototype = proto::function;

    /* from begin to symbol name */
    parse::append_strings(proto.type, v.begin(), it_lparen - 1);

    /* symbol name */
    proto.symbol = *(it_lparen - 1);

    /* parameters */
    copy_parameters(it_lparen + 1, v.end() - 1, proto);

    return true;
}


/* parse tokens and save prototypes */
bool parse_tokens(vstring_t &v, proto_t &proto)
{
    iter_t i;

    /* check size (minimum of 2 for objects) */
    if (v.size() < 2) {
        return false;
    }

    switch (v.back()[0])
    {
    case parse::ID:
        save_object_prototype(v, proto);
        return true;

    case ']':
        i = parse::find_first_not_pointer_or_ident(v);
        return save_array_prototype(v, i, proto);

    case ')':
        i = parse::find_first_not_pointer_or_ident(v);

        /* check for function pointer first! */
        if (save_function_pointer_prototype(v, i, proto) ||
            save_function_prototype(v, i, proto))
        {
            return true;
        }
        break;

    default:
        break;
    }

    return false;
}


/* parse tokens vector and save prototypes */
bool parse_tokens_vector(std::vector<vstring_t> &vec_tokens, vproto_t &vproto)
{
    for (vstring_t &tokens : vec_tokens) {
        proto_t proto;

        if (!parse_tokens(tokens, proto)) {
            print_tokens(tokens);
            return false;
        }

        vproto.push_back(proto);
    }

    return true;
}

} /* end anonymous namespace */



namespace parse
{

/* append strings, separated by space */
void append_strings(std::string &buf, const iter_t &it_beg, const iter_t &it_end)
{
    for (auto i = it_beg; i != it_end; i++) {
        buf += *i;
        buf += ' ';
    }
}


/* find first element that isn't an identifier or pointer */
iter_t find_first_not_pointer_or_ident(vstring_t &v)
{
    auto i = v.begin();

    for ( ; i != v.end(); i++) {
        if (IT(0) != '*' && IT(0) != ID) {
            break;
        }
    }

    return i;
}


bool is_function_pointer(vstring_t &v, const iter_t &i)
{
    /*  minimal size is 7:     */
    /*  type ( * symbol ) ( )  */
    /*       ^i                */
    return check_pattern(v, 7, i, 1, "(*$)(", ')');
}


bool is_function_pointer_no_name(vstring_t &v, const iter_t &i)
{
    /*  minimal size is 6:  */
    /*  type ( * ) ( )      */
    /*       ^i             */
    return check_pattern(v, 6, i, 1, "(*)(", ')');
}


bool is_function_with_parentheses(vstring_t &v, const iter_t &i)
{
    /*  minimal size is 6:   */
    /*  type ( symbol ) ( )  */
    /*       ^i              */
    return check_pattern(v, 6, i, 1, "($)(", ')');
}


bool is_function(vstring_t &v, const iter_t &i)
{
    /*  minimal size is 4:  */
    /*  type symbol ( )     */
    /*              ^i      */
    return check_pattern(v, 4, i, 2, "$(", ')', -1);
}


bool is_array(vstring_t &v, const iter_t &i)
{
    /*  minimal size is 4:  */
    /*  type symbol [ ]     */
    /*              ^i      */
    return check_pattern(v, 4, i, 2, "$[", ']', -1);
}

} /* end parse namespace */


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
    if (!parse_tokens_vector(vec_tokens, vproto)) {
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
