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

#include <iostream>
#include <vector>
#include "global.hpp"


namespace /* anonymous */
{

/* tokenize stream into prototype tokens and options */
int tokenize_stream(FILE *fp, std::vector<vstring_t> &vec, vstring_t *options)
{
    int rv = MYLEX_ERROR;
    vstring_t tokens;
    bool loop = true;

    while (loop)
    {
        rv = mylex(fp);

        switch (rv)
        {
        /* identifier */
        case MYLEX_ID:
            /* don't add "extern" keyword */
            if (strcasecmp(yytext, "extern") != 0) {
                tokens.push_back(std::string(1, parse::ID) + yytext);
            }
            break;

        /* other tokens */
        case MYLEX_OTHER:
            tokens.push_back(yytext);
            break;

        /* end of prototype declaration */
        case MYLEX_SEMICOLON:
            if (!tokens.empty()) {
                vec.push_back(tokens);
                tokens.clear();
            }
            break;

        /* "%option" line */
        case MYLEX_OPTION:
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

    /* push back if last prototype didn't end on semicolon */
    if (!tokens.empty()) {
        vec.push_back(tokens);
    }

    return rv;
}

} /* end anonymous namespace */


/* read input and tokenize */
void gendlopen::tokenize()
{
    std::vector<vstring_t> vec_tokens;
    vstring_t options;
    vstring_t *poptions = m_read_options ? &options : NULL;
    vproto_t vproto;

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

    /* read and tokenize input */
    int ret = tokenize_stream(file.file_pointer(), vec_tokens, poptions);

    if (ret == MYLEX_ERROR) {
        /* lexer error */
        const char *p = mylex_lasterror();

        if (p) {
            throw error(input_name + '\n' + p);
        }
        throw error(input_name);
    } else if (ret == MYLEX_AST_BEGIN) {
        /* input is a clang AST file */
        clang_ast(file.file_pointer());
        return;
    }

    parse(vec_tokens, options, vproto, input_name);
}
