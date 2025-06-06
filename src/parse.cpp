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
#include <ctype.h>
#include "gendlopen.hpp"
#include "types.hpp"
#include "utils.hpp"
#include "parse.hpp"


/* placeholder characters */
#define TYPE   "\x01"
#define SYMBOL "\x02"


namespace /* anonymous */
{
    typedef struct _seq {
        const bool front_has_sym;  /* SYMBOL is among the first part */
        const char *middle;        /* the part where the iterator is */
        const char end;            /* last element */
        const size_t length;       /* pattern sequence length in bytes */
        const int dist_front;      /* distance between front and iterator */
        const int dist_end;        /* distance between end and iterator */
    } seq_t;


    inline bool is_ident(char c) {
        return (isalnum(c) || c == '_');
    }


    /* compare vector elements to pattern sequence
     *
     * first element was already checked in `check_prototype()'
     * and is an identifier and iterator is NOT v.begin() */
    bool check_pattern(vstring_t &v, iter_t it, const seq_t &sq)
    {
        /* compare e_vec (vector) against e_seq (sequence);
         * if e_seq is SYMBOL: check if e_vec is an identificator */
        auto matching = [] (const char &e_vec, const char &e_seq) -> bool {
            return (e_vec == e_seq || (e_seq == SYMBOL[0] && is_ident(e_vec)));
        };

        /* check vector size and boundaries */

        if (v.size() < sq.length) {
            return false;
        }

        if (it != v.end() &&  /* vector contains parentheses/brackets/etc. */
            (std::distance(v.begin(), it) < sq.dist_front ||
             std::distance(it, v.end()) < sq.dist_end))
        {
            return false;
        }

        /* front */
        if (sq.front_has_sym && !is_ident(utils::str_front(*(it-1)))) {
            return false;
        }

        /* end */
        if (!matching(utils::str_front(v.back()), sq.end)) {
            return false;
        }

        /* middle */
        for (auto p = sq.middle; *p != 0; p++, it++) {
            if (!matching(utils::str_front(*it), *p)) {
                return false;
            }
        }

        return true;
    }


    /* print all tokens */
    void print_tokens(const vstring_t &v)
    {
        std::cerr << "tokens:";

        for (const auto &e : v) {
            std::cerr << ' ' << e;
        }

        std::cerr << std::endl;
    }


    /* cosmetics on the output */
    void format_prototype(std::string &s)
    {
        utils::strip_spaces(s);

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
        std::string buf;
        vstring_t tokens;

        for (auto it = it_beg; it != it_end; it++)
        {
            if ((*it).empty()) {
                continue;
            }

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


    /* TYPE SYMBOL */
    bool check_object_prototype(vstring_t &v, proto_t &p, iter_t &it)
    {
        if (!parse::is_object(v, it)) {
            return false;
        }

        p.prototype = proto::object;
        parse::append_strings(p.type, v.begin(), v.end() - 1 /* symbol name */);
        p.symbol = v.back();

        return true;
    }


    /* TYPE SYMBOL [ ] */
    bool check_array_prototype(vstring_t &v, proto_t &p, iter_t &it)
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
    bool check_function_pointer_prototype(vstring_t &v, proto_t &p, iter_t &it)
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


    /* TYPE ( SYMBOL ) ( ) */
    bool check_function_prototype_with_parentheses(vstring_t &v, proto_t &p, iter_t &it)
    {
        if (!parse::is_function_with_parentheses(v, it)) {
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

    /* TYPE SYMBOL ( ) */
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

        iter_t it = v.begin();
        char c = utils::str_front(*it);

        /* first element must be an identifier */
        if (!is_ident(c)) {
            return false;
        }

        it = parse::find_first_not_pointer_or_ident(v);

        if (it == v.end()) {
            return check_object_prototype(v, p, it);
        } else if (it == v.begin()) {
            return false;
        }

        c = utils::str_front(*it);

        if (c == '(') {
            return (check_function_prototype(v, p, it) ||
                    check_function_pointer_prototype(v, p, it));
        } else if (c == '[') {
            return check_array_prototype(v, p, it);
        }

        return false;
    }


    /* parse tokens vector and save prototypes */
    bool parse_tokens(std::vector<vstring_t> &vec_tokens, vproto_t &vproto)
    {
        for (vstring_t &v : vec_tokens) {
            proto_t p;

            /* ignore (simple) typedefs */
            if (!v.empty() && v.front() == "typedef") {
                continue;
            }

            if (!check_prototype(v, p)) {
                print_tokens(v);
                return false;
            }

            vproto.push_back(p);
        }

        return true;
    }

} /* end anonymous namespace */


#define XSTRLEN(x) (sizeof(x)-1)

#define MKFUNC(NAME,FRONT,MIDDLE,END) \
    bool NAME(vstring_t &v, const iter_t &it) \
    { \
        static_assert(*FRONT == *TYPE, \
            "macro parameter `FRONT' must begin with `TYPE'"); \
            \
        static_assert(XSTRLEN(FRONT) == 1 || XSTRLEN(FRONT) == 2, \
            "macro parameter `FRONT' must contain exactly 1 or 2 elements"); \
            \
        static_assert(XSTRLEN(END) == 1, \
            "macro parameter `END' must contain exactly 1 element"); \
        \
        const seq_t sq = { \
            .front_has_sym = (XSTRLEN(FRONT) == 2 && FRONT[1] == SYMBOL[0]), \
            .middle        = MIDDLE, \
            .end           = *END, \
            .length        = XSTRLEN(FRONT) + XSTRLEN(MIDDLE) + XSTRLEN(END), \
            .dist_front    = XSTRLEN(FRONT), \
            .dist_end      = XSTRLEN(MIDDLE) + XSTRLEN(END) \
        }; \
        \
        return check_pattern(v, it, sq); \
    }

MKFUNC( parse::is_function,                   TYPE        SYMBOL,     "(", ")" )
MKFUNC( parse::is_function_with_parentheses,  TYPE,  "("  SYMBOL ")"  "(", ")" )
MKFUNC( parse::is_function_pointer,           TYPE,  "(*" SYMBOL ")"  "(", ")" )
MKFUNC( parse::is_function_pointer_no_name,   TYPE,  "(*" /****/ ")"  "(", ")" )
MKFUNC( parse::is_object,                     TYPE,  "",  SYMBOL               )
MKFUNC( parse::is_array,                      TYPE        SYMBOL,     "[", "]" )
MKFUNC( parse::is_array_no_name,              TYPE,       /****/      "[", "]" )


vstring_t::iterator parse::find_first_not_pointer_or_ident(vstring_t &v)
{
    for (auto i = v.begin(); i != v.end(); i++) {
        char c = utils::str_front(*i);

        if (c != '*' && !is_ident(c)) {
            return i;
        }
    }

    return v.end();
}


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

    /* cosmetics */

    for (auto &e : m_prototypes) {
        format_prototype(e.type);
        format_prototype(e.args);
    }

    for (auto &e : m_objects) {
        format_prototype(e.type);
        format_prototype(e.args);
    }

    for (auto &e : m_typedefs) {
        format_prototype(e);
    }
}
