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


namespace /* anonymous */
{

template<class C, typename T>
bool find_in_list(C list, T item)
{
    return std::find(list.begin(), list.end(), item) != list.end();
}

/* compare s with a list of very basic types and keywords that
 * may appear in parameter list to guess if it could be a name */
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

void skip_comment_asterisk(cio::ifstream &ifs)
{
    char c;

    while (ifs.get(c) && ifs.good()) {
        if (c == '*' && ifs.peek() == '/') {
            ifs.ignore();
            return;
        }
    }
}

void skip_comment_newline(cio::ifstream &ifs)
{
    char c;

    while (ifs.get(c) && ifs.good() && c != '\n')
    {}
}

void pb_token(std::string &token, vstring_t &line)
{
    if (token.empty()) {
        return;
    }

    /* skip reserved "extern" keyword */
    if (!utils::eq_str_case(token, "extern")) {
        line.push_back(token);
    }

    token.clear();
}

void pb_line(vstring_t &line, std::vector<vstring_t> &vec)
{
    if (!line.empty()) {
        vec.push_back(line);
        line.clear();
    }
}

/* read and tokenize input */
void read_input(cio::ifstream &ifs, vproto_t &vproto)
{
    char c;
    int next;
    std::string token;
    vstring_t line;
    std::vector<vstring_t> vec;

    /* tokenize */
    while (ifs.get(c) && ifs.good())
    {
        switch (c)
        {
        /* possible comment */
        case '/':
            if ((next = ifs.peek()) == '*') {
                skip_comment_asterisk(ifs);
            } else if (next == '/') {
                skip_comment_newline(ifs);
            } else {
                /* old token */
                pb_token(token, line);
                /* new token */
                line.push_back("/");
            }
            break;

        /* space */
        case ' ':
        case '\t':
        case '\v':
        case '\f':
        case '\r':
        case '\n':
            pb_token(token, line);
            break;

        /* sequence end */
        case ';':
            pb_token(token, line);
            pb_line(line, vec);
            break;

        /* var, type, etc. */
#ifndef _MSC_VER
        case 'a' ... 'z':
        case 'A' ... 'Z':
        case '0' ... '9':
#endif
        case '_':
        case '.':
            token += c;
            break;

        /* other */
        default:
#ifdef _MSC_VER
            if (utils::range(c, 'a', 'z') ||
                utils::range(c, 'A', 'Z') ||
                utils::range(c, '0', '9'))
            {
                token += c;
                break;
            }
#endif
            /* old token */
            pb_token(token, line);
            /* new token */
            token = c;
            pb_token(token, line);
            break;
        }
    }

    /* just in case */
    pb_token(token, line);
    pb_line(line, vec);

    /* save tokens into prototypes */
    for (auto &l : vec) {
        proto_t proto;

        if (find_in_list(l, "(")) {
            /* function */
            bool param = false;

            for (auto it = l.begin(); it != l.end(); it++) {
                if (!param) {
                    if (it+1 != l.end() && *(it+1) == "(") {
                        /* next iterator is left parenthesis,
                         * marking begin of parameter list */
                        proto.symbol = *it;
                        it++;
                        param = true;
                    } else {
                        /* function return type */
                        proto.type += *it + ' ';
                    }
                } else if (!(*it == ")" && it+1 == l.end())) {
                    /* add to args list while we havent reached
                     * the parameter list's right parenthesis */
                    proto.args += *it + ' ';
                }
            }

            if (proto.args.empty()) {
                proto.args = "void";
            }
        } else {
            /* object */
            for (auto it = l.begin(); it != l.end(); it++) {
                if (it+1 == l.end()) {
                    proto.symbol = *it;
                } else {
                    proto.type += *it + ' ';
                }
            }
        }

        utils::strip_spaces(proto.type);
        utils::strip_spaces(proto.args);

        if (!proto.type.empty() && !proto.symbol.empty()) {
            vproto.push_back(proto);
        }
    }
}

/* check for mismatching parentheses in arguments list */
bool check_mismatching_parentheses(const std::string &param)
{
    int scope = 0;

    for (const char &c : param) {
        if (c == '(') {
            scope++;
        } else if (c == ')') {
            if (--scope < 0) {
                break;
            }
        }
    }

    if (scope != 0) {
        std::cerr << "error: mismatching parentheses in parameter list" << std::endl;
        return false;
    }

    return true;
}

/* extract argument names from args list */
bool get_parameter_names(proto_t &proto, param::names parameter_names)
{
    enum {
        E_DEFAULT,
        E_FUNC_PTR_NAME,
        E_FUNC_PTR_PARAM_LIST
    };

    int search = E_DEFAULT;
    int scope = 0;
    int arg_it = 0;
    std::string out, token, args_new, name;
    vstring_t arg;

    /* nothing to do if empty (= object) or "void" */
    if (proto.args.empty() || utils::eq_str_case(proto.args, "void")) {
        return true;
    }

    if (!check_mismatching_parentheses(proto.args)) {
        return false;
    }

    if (parameter_names == param::skip) {
        proto.notype_args = "/* disabled with -param=skip !! */";
        return true;
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
            } else {
                /* add name */
                if (create) {
                    name = " arg" + std::to_string(arg_it++);
                    args_new += token + name;
                } else {
                    arg.push_back(token);
                }
            }
        } else if (search == E_FUNC_PTR_PARAM_LIST) {
            /* parameter list of a function pointer */
            if (scope == 0) {
                search = E_DEFAULT;
            }

            if (create) {
                args_new += ' ' + token;
            }
        } else { /* search == E_DEFAULT */
            if (token == "(") {
                /* begin of a function pointer name */
                if (create) {
                    args_new += " (";
                }
                search = E_FUNC_PTR_NAME;
            } else if (token == ",") {
                /* argument list separator */
                if (arg.size() == 1 && arg.back() == "...") {
                    /* handle variable arguments token */
                    if (create) {
                        args_new += " ...";
                    }
                    out += ", ...";
                    arg.clear();
                    //break;
                } else if (create) {
                    out += ',';

                    if (name.empty()) {
                        /* create new argument name */
                        std::string s = " arg" + std::to_string(arg_it++);
                        args_new += s;
                        out += s;
                    } else {
                        out += name;
                        name.clear();
                    }
                    args_new += ',';
                } else {
                    /* append existing argument name */
                    if (arg.size() < 2 ||  /* must be at least 2 to hold a type and parameter name */
                        arg.back().back() == '*' ||  /* pointer type without parameter name */
                        keyword_or_type(arg.back()))  /* a reserved keyword or a very basic type (i.e. "int") */
                    {
                        std::cerr << "error: a parameter name is missing:\n"
                            << proto.type << ' ' << proto.symbol << '(' << proto.args << ");\n"
                            "hint: try again with `-param=skip' or `-param=create'" << std::endl;
                        return false;
                    }

                    out += ", " + arg.back();
                    arg.clear();
                }
            } else {
                /* append token */
                if (create) {
                    args_new += ' ' + token;
                } else {
                    arg.push_back(token);
                }
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
    if (out.starts_with(',')) {
        out.erase(0, 1);
    }

    utils::strip_spaces(out);
    proto.notype_args = out;

    return true;
}

} /* end anonymous namespace */


void gendlopen::filter_and_copy_symbols(vproto_t &vproto)
{
    auto pb_symbol = [this] (const proto_t &p)
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
            pb_symbol(e);
        }
    } else {
        /* copy prefixed symbols */
        if (!m_prefix.empty()) {
            for (const auto &e : vproto) {
                if (utils::is_prefixed(e.symbol, m_prefix)) {
                    pb_symbol(e);
                }
            }
        }

        /* copy whitelisted symbols */
        if (!m_symbols.empty()) {
            for (const auto &e : vproto) {
                if (find_in_list(m_symbols, e.symbol)) {
                    pb_symbol(e);
                }
            }
        }
    }
}

/* read input and tokenize */
void gendlopen::tokenize()
{
    vproto_t vproto;

    /* read and tokenize input */
    read_input(m_ifs, vproto);
    m_ifs.close();

    /* nothing found? */
    if (vproto.empty()) {
        throw error("no function or object prototypes found in file: " + m_ifile);
    }

    /* check for duplicates (https://stackoverflow.com/a/72800146/5687704) */
    for (auto i = vproto.begin(); i != vproto.end(); ++i) {
        for (auto j = vproto.begin(); i != j; ++j) {
            if ((*i).symbol == (*j).symbol) {
                throw error("multiple definitions of symbol "
                    "`" + (*i).symbol + "' found in file: " + m_ifile);
            }
        }
    }

    /* get parameter names */
    for (auto &e : vproto) {
        if (!get_parameter_names(e, m_parameter_names)) {
            throw error("error reading function parameter list");
        }
    }

    /* copy */
    filter_and_copy_symbols(vproto);

    /* format args (cosmetics) */
    for (auto &p : m_prototypes) {
        auto rep = [&p] (const std::string &from, const std::string &to) {
            utils::replace(from, to, p.args);
        };

        rep("* ", "*");
        rep(" ,", ",");

        rep("( ", "(");
        rep(" )", ")");
        rep(") (", ")(");

        rep("[ ", "[");
        rep("] ", "]");
        rep(" [", "[");
        rep(" ]", "]");
    }
}
