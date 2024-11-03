/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2023-2024 Carsten Janssen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

/**
 * Tokenize the input text files and save the function and object prototypes
 * into vectors.
 */

#include <algorithm>
#include <iostream>
#include <fstream>
#include <list>
#include <ranges>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include "gendlopen.hpp"


/* lex.yy.c */
extern "C" char *yytext;
extern "C" int mylex(FILE *fp);
extern "C" const char *mylex_lasterror();
extern "C" void yyset_lineno(int n);


namespace /* anonymous */
{

template<class C, typename T>
bool find_in_list(C &list, T &item)
{
    return std::find(list.begin(), list.end(), item) != list.end();
}

inline std::string concat_function_prototype(const proto_t &proto)
{
    if (proto.type.ends_with(" *")) {
        return proto.type + proto.symbol + '(' + proto.args + ')';
    }

    return proto.type + ' ' + proto.symbol + '(' + proto.args + ')';
}

/* compare s against a list of very basic types and keywords that
 * may appear in parameter lists to guess if it could be a name */
bool keyword_or_type(const std::string &s)
{
    const std::list<const char *> keywords =
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
        if (utils::eq_str_case(s, e)) {
            return true;
        }
    }

    return false;
}

/* tokenize stream */
bool tokenize_stream(FILE *fp, std::vector<vstring_t> &vec)
{
    int rv = -1;
    vstring_t tokens;

    while ((rv = mylex(fp)) == 1)
    {
        if (yytext[0] == ';' && !tokens.empty()) {
            vec.push_back(tokens);
            tokens.clear();
        } else if (strcmp(yytext, "extern") != 0) {
            /* don't add "extern" keyword */
            tokens.push_back(yytext);
        }
    }

    if (rv == -1) {
        return false;
    }

    /* push back if last prototype didn't end on semicolon */
    if (!tokens.empty()) {
        vec.push_back(tokens);
    }

    return true;
}

template <typename T, class C>
bool not_beg(T &it, C &vec) {
    return (it != vec.begin());
}

template <class C>
bool last_is(char c, C &vec) {
    return (vec.back()[0] == c);
}

template <typename T, class C>
bool incr_if(char c, T &it, C &vec) {
    return ((*it)[0] == c && ++it != vec.end());
}

template <typename T, class C>
bool incr_if_ident(T &it, C &vec)
{
    if (utils::range((*it)[0], 'A','Z') ||
        utils::range((*it)[0], 'a','z') ||
        (*it)[0] == '_')
    {
        return (++it != vec.end());
    }

    return false;
}

bool is_function_pointer(const vstring_t &v, proto_t &p)
{
    /* type ( * name ) ( ) */
    if (v.size() < 7) {
        return false;
    }

    for (auto i = v.begin(); i != v.end(); i++) {
        /* first open parentheses */
        if (not_beg(i, v) && incr_if('(', i, v)) {
            if (incr_if('*', i, v) && incr_if_ident(i, v))
            {
                auto ident = i - 1;

                if (incr_if(')', i, v) && incr_if('(', i, v) && last_is(')', v))
                {
                    p.prototype = proto::function_pointer;
                    p.symbol = *ident;

                    for (i = v.begin(); i != ident - 2; i++) {
                        p.type += *i + ' ';
                    }
                    p.type += "(*";

                    for (i = ident + 1; i != v.end(); i++) {
                        p.type += *i + ' ';
                    }
                    return true;
                }
            }
            return false;
        }
    }

    return false;
}

bool is_function(const vstring_t &v, proto_t &p)
{
    /* type name ( ) */
    if (v.size() < 4) {
        return false;
    }

    for (auto i = v.begin(); i != v.end(); i++) {
        /* type ... ident */
        if (not_beg(i, v) && incr_if_ident(i, v))
        {
            auto ident = i - 1;

            /* type ... ident lparen */
            if (incr_if('(', i, v)) {
                /* type ... ident lparen ... rparen */
                if (!incr_if('*', i, v) && last_is(')',v)) {
                    p.prototype = proto::function;
                    p.symbol = *ident;

                    /* type */
                    for (i = v.begin(); i != ident; i++) {
                        p.type += *i + ' ';
                    }

                    /* args */
                    for (i = ident + 1; i != v.end(); i++) {
                        p.args += *i + ' ';
                    }

                    utils::delete_prefix(p.args, "( ");
                    utils::delete_suffix(p.args, ") ");

                    if (p.args.empty()) {
                        p.args = "void";
                    }

                    return true;
                }
                return false;
            }
        }
    }

    return false;
}

bool is_object(vstring_t &vec, proto_t &proto)
{
    /* type name */
    if (vec.size() < 2) {
        return false;
    }

    for (const auto &e : vec) {
        if (e[0] == '(' || e[0] == ')') {
            return false;
        }
    }

    proto.prototype = proto::object;
    proto.symbol = vec.back();
    vec.pop_back();

    for (const auto &e : vec) {
        proto.type += e + ' ';
    }

    return true;
}

/* save tokens into prototypes */
bool get_prototypes(std::vector<vstring_t> &vec_tokens, vproto_t &vproto)
{
    for (auto &tokens : vec_tokens) {
        proto_t proto;

        if (tokens.size() >= 2 &&
            (is_function(tokens, proto) ||
             is_object(tokens, proto) ||
             is_function_pointer(tokens, proto)))
        {
            utils::strip_spaces(proto.type);
            utils::strip_spaces(proto.args);
            vproto.push_back(proto);
        } else {
            std::cerr << "error: ";

            for (const auto &e : tokens) {
                std::cerr << e << ' ';
            }
            std::cerr << std::endl;

            return false;
        }
    }

    return true;
}

/* extract argument names from args list */
std::string get_parameter_names(proto_t &proto, param::names parameter_names)
{
    enum {
        E_DEFAULT,
        E_FUNC_PTR_NAME,
        E_FUNC_PTR_PARAM_LIST
    };

    int search = E_DEFAULT;
    int scope = 0;
    char letter[] = " a";
    std::string ok, args_notypes, token, args_new, name;
    vstring_t arg;

    /* nothing to do if empty (== object) or "void" */
    if (proto.args.empty() || utils::eq_str_case(proto.args, "void")) {
        return ok;
    }

    if (parameter_names == param::skip) {
        proto.notype_args = "/* disabled with -param=skip !! */";
        return ok;
    }

    /* trailing comma is needed for parsing */
    std::istringstream iss(proto.args + " ,");

    /* whether we create new parameter names or not */
    const bool create = (parameter_names == param::create);

    /* tokenize argument list */
    while (iss >> token) {
        if (token == "(") {
            scope++;
        } else if (token == ")") {
            scope--;
        }

        if (search == E_FUNC_PTR_NAME) {
            /* function pointer name */
            if (token == ")") {
                /* end of name sequence */
                if (create) {
                    args_new += ')';
                }
                search = E_FUNC_PTR_PARAM_LIST;
                continue;
            } else {
                /* add name */
                if (create) {
                    if (letter[1] > 'z') {
                        return proto.symbol + ": too many parameters";
                    }
                    name = letter;
                    args_new += token + name;
                    letter[1]++;
                } else {
                    arg.push_back(token);
                }
                continue;
            }
        } else if (search == E_FUNC_PTR_PARAM_LIST) {
            /* parameter list of a function pointer */
            if (scope == 0) {
                search = E_DEFAULT;
            }

            if (create) {
                args_new += ' ' + token;
            }
            continue;
        } else { /* search == E_DEFAULT */
            if (token == "(") {
                /* begin of a function pointer name */
                if (create) {
                    args_new += " (";
                }
                search = E_FUNC_PTR_NAME;
                continue;
            } else if (token == ",") {
                /* argument list separator */
                if (arg.size() == 1 && arg.back() == "...") {
                    /* handle variable arguments token */
                    if (create) {
                        args_new += " ...";
                    }
                    args_notypes += ", ...";
                    arg.clear();
                    continue;
                } else if (create) {
                    args_notypes += ',';

                    if (name.empty()) {
                        /* create new argument name */
                        if (letter[1] > 'z') {
                            return proto.symbol + ": too many parameters";
                        }
                        args_new += letter;
                        args_notypes += letter;
                        letter[1]++;
                    } else {
                        args_notypes += name;
                        name.clear();
                    }
                    args_new += ',';
                    continue;
                } else {
                    /* append existing argument name */
                    if (arg.size() < 2 ||  /* must be at least 2 to hold a type and parameter name */
                        arg.back().back() == '*' ||  /* pointer type without parameter name */
                        keyword_or_type(arg.back()))  /* reserved keyword or a very basic type (i.e. "int") */
                    {
                        return "a parameter name is missing: " + concat_function_prototype(proto) + ";\n"
                            "hint: try again with `-param=skip' or `-param=create'";
                    }

                    args_notypes += ", " + arg.back();
                    arg.clear();
                    continue;
                }
            } else {
                /* append token */
                if (create) {
                    args_new += ' ' + token;
                } else {
                    arg.push_back(token);
                }
                continue;
            }
        }
    }

    /* .args */
    if (create) {
        if (args_new.ends_with(',')) {
            args_new.pop_back();
        }

        /* overwrite old args */
        utils::strip_spaces(args_new);
        proto.args = args_new;
    }

    /* .notype_args */
    if (args_notypes.starts_with(',')) {
        args_notypes.erase(0, 1);
    }

    utils::strip_spaces(args_notypes);
    proto.notype_args = args_notypes;

    return ok;
}

} /* end anonymous namespace */


void gendlopen::filter_and_copy_symbols(vproto_t &vproto)
{
    auto save_symbol = [this] (const proto_t &p)
    {
        /* no arguments means object, else function prototype */
        if (p.args.empty()) {
            m_objects.push_back(p);
        } else {
            m_prototypes.push_back(p);
        }
    };

    if (m_prefix.empty() && m_symbols.empty()) {
        /* copy all symbols */
        for (const auto &e : vproto) {
            save_symbol(e);
        }
    } else {
        /* copy prefixed symbols */
        if (!m_prefix.empty()) {
            for (const auto &e : vproto) {
                if (utils::is_prefixed(e.symbol, m_prefix)) {
                    save_symbol(e);
                }
            }
        }

        /* copy whitelisted symbols */
        if (!m_symbols.empty()) {
            for (const auto &e : vproto) {
                if (find_in_list(m_symbols, e.symbol)) {
                    save_symbol(e);
                }
            }
        }
    }
}

/* read input and tokenize */
void gendlopen::tokenize()
{
    FILE *fp;
    std::vector<vstring_t> vec_tokens;
    vproto_t vproto;

    std::string file_or_stdin = (m_ifile == "-") ? "<STDIN>" : "file: " + m_ifile;

    m_ifs.close();

    if (m_ifile == "-") {
        fp = stdin;
    } else if ((fp = fopen(m_ifile.c_str(), "rb")) == NULL) {
        perror("fopen()");
        throw error(file_or_stdin);
    }

    if (m_second_attempt) {
        yyset_lineno(2);
    }

    /* read and tokenize input */
    bool ret = tokenize_stream(fp, vec_tokens);

    if (fp != stdin) {
        fclose(fp);
    }

    if (!ret) {
        const char *p = mylex_lasterror();

        if (p) {
            file_or_stdin += '\n';
            file_or_stdin += p;
        }

        throw error(file_or_stdin);
    }

    /* get prototypes from tokens */
    if (!get_prototypes(vec_tokens, vproto)) {
        file_or_stdin += '\n';
        file_or_stdin += "failed to read prototypes";
        throw error(file_or_stdin);
    }

    /* nothing found? */
    if (vproto.empty()) {
        throw error("no function or object prototypes found in " + file_or_stdin);
    }

    /* check for duplicates (https://stackoverflow.com/a/72800146/5687704) */
    for (auto i = vproto.begin(); i != vproto.end(); ++i) {
        for (auto j = vproto.begin(); i != j; ++j) {
            if ((*i).symbol == (*j).symbol) {
                throw error("multiple definitions of symbol "
                    "`" + (*i).symbol + "' found in " + file_or_stdin);
            }
        }
    }

    /* get parameter names */
    for (auto &e : vproto) {
        std::string msg = get_parameter_names(e, m_parameter_names);

        if (!msg.empty()) {
            throw error(msg);
        }
    }

    /* copy */
    filter_and_copy_symbols(vproto);
}
