/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2023-2025 Carsten Janssen

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
 *
 * The regular expressions used to identify the prototypes are far from perfect
 * but they are good enough for our purpose.
 */

#include <algorithm>
#include <iostream>
#include <fstream>
#include <list>
#include <ranges>
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>

#include "gendlopen.hpp"

/* regex macros */
#define LPAREN   "\\("
#define RPAREN   "\\)"
#define PARENS   LPAREN RPAREN
#define LBRACK   "\\["
#define RBRACK   "\\]"
#define BRACKETS LBRACK RBRACK
#define PTR      "\\*"
#define DOT      "\\."
#define SPC      " "
#define COMMA    ","
#define ALPHA    "a-zA-Z_"
#define ALNUM    "a-zA-Z0-9_"
#define EMPTY    /**/

#define REG_TYPE   "[" ALPHA "][" ALNUM PTR SPC "]*?"
#define REG_SYMBOL "[" ALPHA "][" ALNUM "]*?"

/* dot is for "..." */
#define REG_PARAM  "[" ALNUM PTR SPC COMMA DOT PARENS BRACKETS "]*?"

/* array size can be a macro */
#define REG_ARRAY \
    " " LBRACK "[" ALNUM SPC BRACKETS "]*? " RBRACK \
    "|" EMPTY



namespace /* anonymous */
{

enum {
    E_TYPE   = 1,
    E_SYMBOL = 2,
    E_PARAM  = 3,
    E_ARRAY  = 3
};

/* cut bytes from left and right of string */
template<typename T=std::string>
T cut_lr(const T &str, const size_t &left, const size_t &right) {
    return str.substr(left, str.size() - (left + right));
}

/* compare s against a list of very basic types and keywords that
 * may appear in parameter lists to guess if it could be a name */
bool keyword_or_type(const std::string &s)
{
    const list_t keywords =
    {
        "char",
        "int", "long", "short",
        "float", "double",
        "void",
        "signed", "unsigned",
        "const", "volatile",
        "struct", "union", "enum",
        "restrict", "register"
    };

    /* i.e. "size_t" */
    if (s.size() > 2 && (s.ends_with("_t") || s.ends_with("_T"))) {
        return true;
    }

    for (const auto &e : keywords) {
        if (strcasecmp(s.c_str(), e) == 0) {
            return true;
        }
    }

    return false;
}

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
        /* any regular token */
        case MYLEX_TOKEN:
            /* don't add "extern" keyword */
            if (strcmp(yytext, "extern") != 0) {
                tokens.push_back(yytext);
            }
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

/* extract a function parameter name */
std::string get_param_name(const std::string &element)
{
    /* regular parameter type */
    const std::regex reg_object(
        "^" REG_TYPE   " "
        "(" REG_SYMBOL ")"
        "(" REG_ARRAY  ")"
    );

    /* function pointer */
    const std::regex reg_fptr(
        "^"         REG_TYPE   " "
        "\\( \\* (" REG_SYMBOL ") \\) "
        "\\( "      REG_PARAM    "\\)"
    );

    std::smatch m;

    if ((std::regex_match(element, m, reg_object) && m.size() == 3) ||
        (std::regex_match(element, m, reg_fptr) && m.size() == 2))
    {
        return m.str(1);
    }

    return {};
}

/* check for object prototype */
bool is_object(const std::string &line, proto_t &proto)
{
    const std::regex reg(
        "^(" REG_TYPE   ") "
         "(" REG_SYMBOL ")"
         "(" REG_ARRAY  ")"
    );

    std::smatch m;

    if (!std::regex_match(line, m, reg) || m.size() != 4) {
        return false;
    }

    proto.type = m.str(E_TYPE);
    proto.symbol = m.str(E_SYMBOL);

    if (m.str(E_ARRAY).empty()) {
        proto.prototype = proto::object;
    } else {
        proto.prototype = proto::object_array;
        proto.type += m.str(E_ARRAY);
    }

    return true;
}

/* check for function or function pointer prototype */
bool is_function_or_function_pointer(const std::string &line, proto_t &proto)
{
    const std::regex reg(
        "^(" REG_TYPE ") "

        "("
                       REG_SYMBOL        "|"  /* function */
            "\\( \\* " REG_SYMBOL " \\)" "|"  /* function pointer */
            "\\( "     REG_SYMBOL " \\)"      /* function with parentheses */
        ") "

        "\\( (" REG_PARAM ")\\)"
    );

    std::smatch m;

    if (!std::regex_match(line, m, reg) || m.size() != 4) {
        return false;
    }

    if (m.str(E_SYMBOL).starts_with("( *")) {
        /* funtion pointer */
        proto.prototype = proto::function_pointer;
        proto.type = m.str(E_TYPE) + " (*)(" + m.str(E_PARAM) + ")";
        proto.symbol = cut_lr(m.str(E_SYMBOL), 4, 2); /* "( * symbol )" */
    } else {
        /* function */
        proto.prototype = proto::function;
        proto.type = m.str(E_TYPE);
        proto.args = m.str(E_PARAM);

        if (m.str(E_SYMBOL).starts_with('(')) {
            proto.symbol = cut_lr(m.str(E_SYMBOL), 2, 2); /* "( symbol )" */
        } else {
            proto.symbol = m.str(E_SYMBOL);
        }
    }

    return true;
}

/* check for mismatching brackets */
bool brackets_are_okay(const std::string &line, std::string &msg)
{
    bool inside_brackets = false;
    size_t pos = std::string::npos;

    for (size_t i = 0; i < line.size(); i++) {
        switch (line[i])
        {
        case '[':
            if (inside_brackets) {
                msg.insert(0, i, ' ');
                msg += '^';
                return false;
            }
            pos = i;
            inside_brackets = true;
            break;
        case ']':
            if (!inside_brackets) {
                msg.insert(0, i, ' ');
                msg += '^';
                return false;
            }
            inside_brackets = false;
            break;
        default:
            break;
        }
    }

    if (inside_brackets) {
        /* trailing left bracket */
        if (pos != std::string::npos) {
            msg.insert(0, pos, ' ');
            msg += '^';
        }
        return false;
    }

    return true;
}

/* save tokens into prototypes */
bool get_prototypes_from_tokens(const std::vector<vstring_t> &vec_tokens, vproto_t &vproto)
{
    auto is_prototype = [] (const std::string &line, proto_t &proto) -> bool {
        return (is_object(line, proto) || is_function_or_function_pointer(line, proto));
    };

    for (auto &tokens : vec_tokens) {
        proto_t proto;
        std::string line, msg;

        /* write tokens back into a space separated line */
        for (const auto &e : tokens) {
            line += e + ' ';
        }

        utils::strip_spaces(line);

        /* get prototype from `line' and save into `proto' */
        if (tokens.size() < 2 || !is_prototype(line, proto)) {
            std::cerr << "error: " << line << std::endl;
            return false;
        }

        /* check for mismatching brackets */
        if (!brackets_are_okay(line, msg)) {
            std::cerr << "error: mismatching brackets `[]':" << std::endl;
            std::cerr << line << '\n' << msg << std::endl;
            return false;
        }

        utils::strip_spaces(proto.type);
        utils::strip_spaces(proto.args);

        vproto.push_back(proto);
    }

    return true;
}

/* add parameter names 'a' to 'z' */
bool add_parameter_names(const vstring_t &tokens, proto_t &proto)
{
    char name[] = " a";

    proto.args.clear();
    proto.notype_args.clear();

    for (auto e : tokens) {
        if (name[1] > 'z') {
            return false;
        }

        if (e == "...") {
            /* va_list */
            proto.args += "..., ";
            proto.notype_args += "...,";
            continue;
        }

        auto pos = e.find("( * ");

        if (pos != std::string::npos && e.find('(') == pos) {
            /* function pointer */
            e.insert(pos + 3, name);
        } else {
            /* object */
            e += name;
        }

        proto.args += e + ", ";
        proto.notype_args += name;
        proto.notype_args += ',';

        name[1]++;
    }

    return true;
}

/* extract argument names from args list */
bool read_parameter_names(proto_t &proto, param::names parameter_name, std::string &msg)
{
    vstring_t vec;
    std::string arg;
    int scope = 0;
    size_t pos = std::string::npos;
    bool mismatch = false;

    auto pretty_function = [] (const proto_t &p) {
        if (p.type.ends_with(" *")) {
            return p.type + p.symbol + '(' + p.args + ')';
        }
        return p.type + ' ' + p.symbol + '(' + p.args + ')';
    };

    /* split parameters, take care of parentheses */
    for (size_t i = 0; i < proto.args.size() && !mismatch; i++)
    {
        switch (proto.args[i])
        {
        case ',':
            if (scope == 0) {
                utils::strip_spaces(arg);
                vec.push_back(arg);
                arg.clear();
                continue;
            }
            break;
        case '(':
            scope++;
            pos = i;
            break;
        case ')':
            pos = i;
            if (--scope < 0) {
                mismatch = true;
                continue;
            }
            break;
        default:
            break;
        }

        arg += proto.args[i];
    }

    if (scope != 0) {
        /* mismatching parentheses! */
        msg = "mismatching parentheses in parameter list of function "
            "`" + proto.symbol + "':\n"
            "( " + proto.args + " )";

        if (pos != std::string::npos) {
            msg += '\n';
            msg.append(pos + 2, ' ');
            msg += '^';
        }

        return false;
    }

    if (!arg.empty()) {
        utils::strip_spaces(arg);
        vec.push_back(arg);
    }

    if (parameter_name == param::create) {
        if (!add_parameter_names(vec, proto)) {
            msg = proto.symbol + ": too many parameters to handle";
            return false;
        }
    } else {
        /* get parameter names */
        for (auto &e : vec) {
            if (e == "...") {
                /* va_list */
                proto.notype_args += "...";
            } else {
                std::string name = get_param_name(e);

                if (name.empty() || keyword_or_type(name)) {
                    msg = "a parameter name is missing: " + pretty_function(proto) + ";\n"
                        "hint: try again with `" OPT_SKIP_PARAM "' or `" OPT_CREATE_PARAM "'";
                    return false;
                }

                proto.notype_args += name;
            }

            proto.notype_args += ", ";
        }
    }

    utils::strip_spaces(proto.args);
    utils::strip_spaces(proto.notype_args);
    utils::delete_suffix(proto.args, ',');
    utils::delete_suffix(proto.notype_args, ',');

    return true;
}

/* format the text of the prototypes to make it look pretty */
void format_prototypes(std::string &s)
{
    utils::replace("* ", "*", s);
    utils::replace(" ,", ",", s);

    utils::replace("( ", "(", s);
    utils::replace(" )", ")", s);
    utils::replace(") (", ")(", s);

    utils::replace("[ ", "[", s);
    utils::replace("] ", "]", s);
    utils::replace(" [", "[", s);
    utils::replace(" ]", "]", s);

    utils::strip_spaces(s);
}

/* check for multiple definitions of a symbol */
std::string multi_def(const vproto_t &vproto)
{
    /* https://stackoverflow.com/a/72800146/5687704 */
    for (auto i = vproto.begin(); i != vproto.end(); ++i) {
        for (auto j = vproto.begin(); i != j; ++j) {
            if ((*i).symbol == (*j).symbol) {
                return (*i).symbol;
            }
        }
    }

    return {};
}

} /* end anonymous namespace */


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

    for (auto &p : m_prototypes) {
        format_prototypes(p.type);
        format_prototypes(p.args);
    }

    for (auto &p : m_objects) {
        format_prototypes(p.type);
        format_prototypes(p.args);
    }

    for (auto &e : m_typedefs) {
        format_prototypes(e);
    }
}

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

    std::string file_or_stdin = (m_input == "-")
        ? "<STDIN>"
        : "file: " + m_input;

    open_file file(m_input);

    if (!file.is_open()) {
        throw error(file_or_stdin + "\nfailed to open file for reading");
    }

    /* read and tokenize input */
    int ret = tokenize_stream(file.file_pointer(), vec_tokens, poptions);

    /* input is a clang AST file */
    if (ret == MYLEX_AST_BEGIN) {
        clang_ast(file.file_pointer());
        return;
    }

    file.close();

    /* lexer error */
    if (ret == MYLEX_ERROR) {
        const char *p = mylex_lasterror();

        if (p) {
            file_or_stdin += '\n';
            file_or_stdin += p;
        }

        throw error(file_or_stdin);
    }

    /* parse `%options' strings */
    parse_options(options);

    /* get prototypes from tokens */
    if (!get_prototypes_from_tokens(vec_tokens, vproto)) {
        throw error(file_or_stdin + "\nfailed to read prototypes");
    }

    /* nothing found? */
    if (vproto.empty()) {
        throw error("no function or object prototypes found in " + file_or_stdin);
    }

    /* check for multiple definitions of a symbol */
    auto dup = multi_def(vproto);

    if (!dup.empty()) {
        throw error("multiple definitions of symbol "
            "`" + dup + "' found in " + file_or_stdin);
    }

    /* get parameter names */
    if (m_parameter_names != param::skip) {
        std::string msg;

        for (auto &e : vproto) {
            if (e.prototype != proto::function ||
                e.args.empty() ||
                strcasecmp(e.args.c_str(), "void") == 0)
            {
                /* not a function or a function without parameters */
                continue;
            }

            if (!read_parameter_names(e, m_parameter_names, msg)) {
                throw error(msg);
            }
        }
    }

    /* copy */
    filter_and_copy_symbols(vproto);
}
