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
#include <vector>
#include "gendlopen.hpp"
#include "lex.h"
#include "open_file.hpp"
#include "parse.hpp"
#include "types.hpp"
#include "utils.hpp"


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
                /* typedef or struct */
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
                    curly_count++;
                    break;

                case LEX_CURLY_CLOSE:
                    curly_count--;
                    break;

                /* end of block? */
                case LEX_SEMICOLON:
                    if (curly_count < 1) {
                        curly_count = 0;
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
                /* identifier */
                case LEX_ID:
                    tokens.push_back(yytext);
                    break;

                /* struct, union, enum or typedef */
                case LEX_STRUCT:
                case LEX_UNION:
                case LEX_ENUM:
                case LEX_TYPEDEF:
                    if (tokens.empty()) {
                        block = true;
                    } else {
                        tokens.push_back(yytext);
                    }
                    break;

                /* other tokens */
                case LEX_OTHER:
                    tokens.push_back(yytext);
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
                    utils::find_and_erase(tokens, "extern");

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

        utils::find_and_erase(tokens, "extern");

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

    if (m_read_options) {
        /* parse `%options' strings */
        parse::options(this, options);
    }

    parse(vec_tokens, input_name);
    create_typedefs();
}
