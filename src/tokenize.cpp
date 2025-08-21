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

/**
 * Tokenize the input text files and save the function and object prototypes
 * into vectors.
 */

#include <stdio.h>
#include <string>
#include <algorithm>
#include <vector>
#include "gendlopen.hpp"
#include "lex.h"
#include "open_file.hpp"
#include "parse.hpp"
#include "types.hpp"
#include "utils.hpp"


typedef struct _paren_entry {
    std::string str;
    int num;
} paren_entry_t;


namespace /* anonymous */
{
    /* tokenize stream into prototype tokens and options */
    int tokenize_stream(std::vector<vstring_t> &vec, vstring_t *options)
    {
        int rv = LEX_ERROR;
        vstring_t tokens;
        bool loop = true;
        bool block = false;
        int curly_count = 0;

        while (loop)
        {
            rv = yylex();

            if (block)
            {
                /* struct, union, enum or typedef block */
                switch (rv)
                {
                case LEX_STRUCT:
                case LEX_UNION:
                case LEX_ENUM:
                case LEX_TYPEDEF:
                case LEX_ID:
                case LEX_OTHER:
                case LEX_EQUAL:
                    /* ignore */
                    break;

                case LEX_CURLY_OPEN:
                    ++curly_count;
                    break;

                case LEX_CURLY_CLOSE:
                    --curly_count;
                    break;

                /* end of block? */
                case LEX_SEMICOLON:
                    if (curly_count < 1) {
                        block = false;
                        tokens.clear();
                    }
                    break;

                /* "%option" line */
                case LEX_OPTION:
                    if (options) {
                        options->push_back(yytext);
                    }
                    break;

                default:
                    /* EOF, error, etc. */
                    loop = false;
                    break;
                }
            }
            else
            {
                switch (rv)
                {
                /* identifier, other tokens */
                case LEX_ID:
                case LEX_OTHER:
                    tokens.push_back(yytext);
                    break;

                /* struct, union, enum or typedef */
                case LEX_STRUCT:
                case LEX_UNION:
                case LEX_ENUM:
                case LEX_TYPEDEF:
                    if (tokens.empty()) {
                        block = true;
                        curly_count = 0;
                    } else {
                        tokens.push_back(yytext);
                    }
                    break;

                /* {}= */
                case LEX_CURLY_OPEN:
                case LEX_CURLY_CLOSE:
                case LEX_EQUAL:
                    set_illegal_char();
                    rv = LEX_ERROR;
                    loop = false;
                    break;

                /* end of prototype declaration */
                case LEX_SEMICOLON:
                    std::erase(tokens, "extern");

                    if (!tokens.empty()) {
                        vec.push_back(tokens);
                        tokens.clear();
                    }
                    break;

                /* "%option" line */
                case LEX_OPTION:
                    if (options) {
                        options->push_back(yytext);
                    }
                    break;

                default:
                    /* EOF, error, etc. */
                    loop = false;
                    break;
                }
            }
        } /* end of loop body */

        std::erase(tokens, "extern");

        /* push back if last prototype didn't end on semicolon */
        if (!tokens.empty()) {
            vec.push_back(tokens);
        }

        return rv;
    }


    /* 'void (*)()'  ==>  'void (*name)()' */
    std::string fptr_typedef(const std::string &type, const std::string &name)
    {
        auto pos = type.find("(*)");

        if (pos == std::string::npos || type.find('(') != pos) {
            return {};
        }

        std::string s = type;
        pos += 2; /* insert after '*' */

        return s.insert(pos, name);
    }


    /* 'char[32]'  ==>  'char name[32]' */
    std::string array_typedef(std::string &type, const std::string &name)
    {
        auto pos = type.find('[');

        if (pos == std::string::npos) {
            return {};
        }

        std::string s = type;

        /* insert space if needed */
        if (pos > 0 && utils::str_at(type, pos-1) != ' ') {
            s.insert(pos, 1, ' ');
            pos++;
        }

        return s.insert(pos, name);
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


    void recount_parentheses(std::vector<paren_entry_t> &v, std::vector<paren_entry_t>::iterator &start)
    {
        std::vector<int> stack;
        int n = 0;

        for (auto it = start; it != v.end(); it++) {
            if ((*it).str == "(") {
                n++;
                (*it).num = n;
                stack.push_back(n);
            } else if ((*it).str == ")") {
                (*it).num = stack.back();
                stack.pop_back();
            } else {
                (*it).num = 0;
            }
        }
    }


    bool find_closing_parentheses(std::vector<paren_entry_t> &v, std::vector<paren_entry_t>::iterator &it_inner)
    {
        for (auto it2_inner = it_inner + 1; it2_inner != v.end(); it2_inner++) {
            auto next = it2_inner + 1;

            if ((*it2_inner).str == ")" && next != v.end() && (*next).str == ")" &&  /* ")" && ")" */
                (*it2_inner).num == (*it_inner).num && /* numbers of inner closing paren and
                                                        * inner opening paren are the same */
                (*it2_inner).num == ((*next).num + 1)) /* consecutive lowering numbers i.e. ")<3>" && ")<2>" */
            {
                v.erase(it2_inner);
                v.erase(it_inner);
                return true;
            }
        }

        return false;
    }


    bool find_opening_parentheses(std::vector<paren_entry_t> &v, std::vector<paren_entry_t>::iterator &it)
    {
        for ( ; it != v.end(); it++) {
            auto it_inner = it + 1;

            /* find matching superfluous parentheses and remove the inner ones */
            if ((*it).str == "(" && it_inner != v.end() && (*it_inner).str == "(" &&  /* "(" && "(" */
                ((*it).num + 1) == (*it_inner).num) /* consecutive rising numbers i.e. "(<2>" && "(<3>" */
            {
                if (find_closing_parentheses(v, it_inner)) {
                    return true;
                }
            }
        }

        return false;
    }


    void remove_superfluous_parentheses(vstring_t &v_in)
    {
        std::vector<paren_entry_t> v;

        /* minimum size is 4 -> "(())"" */
        if (v_in.size() < 4) {
            return;
        }

        /* check if an opening parentheses exists */
        if (std::find(v_in.begin(), v_in.end(), "(") == v_in.end()) {
            return;
        }

        /* copy from input vector into temporary vector */
        for (const auto &e : v_in) {
            paren_entry_t tmp = { e, 0 };
            v.push_back(tmp);
        }

        auto it = v.begin();
        recount_parentheses(v, it);

        /* find and remove superfluous parentheses */
        while (find_opening_parentheses(v, it)) {
            recount_parentheses(v, it);
        }

        /* copy from temporary vector back to input vector */
        v_in.clear();

        for (const auto &e : v) {
            v_in.push_back(e.str);
        }
    }
} /* end anonymous namespace */


/* create typedefs for function pointers and arrays */
void gendlopen::create_typedefs()
{
    /* create typename */
    auto mk_name = [this] (const std::string &symbol) {
        return m_pfx_lower + '_' + symbol + "__t";
    };

    for (auto &p : m_objects) {
        std::string def, name;

        if (p.prototype == proto::function_pointer) {
            /* function pointer */
            name = mk_name(p.symbol);
            def = fptr_typedef(p.type, name);
        } else if (p.prototype == proto::object_array) {
            /* array type */
            name = mk_name(p.symbol);
            def = array_typedef(p.type, name);
        } else {
            continue;
        }

        if (def.empty()) {
            throw error("failed to create typedef for symbol `" + p.symbol + "'");
        }

        m_typedefs.push_back(def); /* add to typedefs */
        p.type = name; /* replace old type */
    }
}


/* read input and tokenize */
void gendlopen::tokenize()
{
    std::vector<vstring_t> vec_tokens;
    vstring_t options;
    vstring_t *poptions = m_read_options ? &options : NULL;

    /* open input file */

    if (m_input.empty()) {
        throw error("input file required");
    }

    if (m_input == "-" && m_custom_template == "-") {
        throw error("cannot read input file and custom template both from STDIN");
    }

    std::string input_name = (m_input == "-")
        ? "<STDIN>"
        : "file: " + m_input;

    open_file file(m_input);

    if (!file.is_open()) {
        throw error(input_name + "\nfailed to open file for reading");
    }

    /* set input file pointer */
    yyset_in(file.file_pointer());

    /* read and tokenize input */
    int ret = tokenize_stream(vec_tokens, poptions);

    if (ret == LEX_ERROR) {
        if (*lex_errmsg) {
            throw error(input_name + '\n' + lex_errmsg);
        }
        throw error(input_name);
    } else if (ret == LEX_AST_BEGIN) {
        /* input is a clang AST file */
        parse_clang_ast();
        create_typedefs();
        return;
    }

    /* parse `%options' strings */
    if (m_read_options) {
        parse_options(options);
    }

    /* remove superfluous parentheses on each entry */
    for (auto &v : vec_tokens) {
        remove_superfluous_parentheses(v);
    }

    /* parse tokens */
    parse(vec_tokens, input_name);
    create_typedefs();

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
