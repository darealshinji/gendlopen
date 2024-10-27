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

void save_token(std::string &token, vstring_t &line)
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

inline void save_line(vstring_t &line, std::vector<vstring_t> &vec)
{
    if (!line.empty()) {
        vec.push_back(line);
        line.clear();
    }
}

void remove_comment(const std::string &beg, const std::string &end, std::string &s)
{
    size_t pos_beg = 0;
    size_t pos_end = 0;

    while ((pos_beg = s.find(beg, pos_beg)) != std::string::npos) {
        if ((pos_end = s.find(end, pos_beg)) != std::string::npos) {
            /* replace with spaces, keep newlines */
            for (size_t i = pos_beg; i < pos_end + end.size(); i++) {
                if (s[i] != '\n') {
                    s[i] = ' ';
                }
            }
        }
    }
}

std::string illegal_char(const char &c, int &lineno)
{
    char buf[128];
    const char *fmt = "illegal character `%c' at line %d";

    if (!isprint(c)) {
        fmt = "illegal character `\\%03u' at line %d";
    }

    sprintf(buf, fmt, c, lineno);

    return buf;
}

/* tokenize stream */
std::string tokenize_stream(cio::ifstream &ifs, std::vector<vstring_t> &vec, bool first_line_read)
{
    char ch;
    std::string token, text;
    vstring_t line;
    int lineno = 1;

    /* bump initial line number is input is stdin and we
     * had already read the first line */
    if (first_line_read && ifs.is_stdin()) {
        lineno++;
    }

    /* save file content to std::string first
     * which makes it much easier to parse */
    if (ifs.file_size() > 0) {
        text.reserve(ifs.file_size());
    }

    while (ifs.get(ch) && ifs.good()) {
        text += ch;
    }
    text += '\n';
    ifs.close();

    /* strip commentary */
    remove_comment("/*", "*/", text);
    remove_comment("//", "\n", text);

    for (const char &c : text)
    {
        switch (c)
        {
        /* newline */
        case '\n':
            lineno++;
            /* FALLTHROUGH */

        /* space */
        case ' ':
        case '\t':
        case '\v':
        case '\f':
        case '\r':
            save_token(token, line);
            break;

        /* var, type, etc. */
#ifndef _MSC_VER
        case 'a' ... 'z':
        case 'A' ... 'Z':
        case '0' ... '9':
#endif
        case '_':
            token += c;
            break;

        /* other legal characters */
        case '*':
        case ',':
        case '(': case ')':
        case '[': case ']':
            /* old token */
            save_token(token, line);
            /* new token */
            token = c;
            save_token(token, line);
            break;

        case '.':
            if (token.empty() || token == "." || token == "..") {
                token += c;
            } else {
                return illegal_char(c, lineno);
            }
            break;

        /* sequence end */
        case ';':
            save_token(token, line);
            save_line(line, vec);
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
            return illegal_char(c, lineno);
        }
    }

    return {};
}

/* check for mismatching parentheses */
bool has_mismatching_parentheses(const vstring_t &tokens)
{
    int scope = 0;

    for (const auto &e : tokens) {
        if (e == "(") {
            scope++;
        } else if (e == ")") {
            if (--scope < 0) {
                break;
            }
        }
    }

    return (scope != 0);
}

/* save tokens into prototypes */
std::string get_prototypes(std::vector<vstring_t> &vec_tokens, vproto_t &vproto)
{
    for (auto &l : vec_tokens) {
        proto_t proto;

        if (has_mismatching_parentheses(l)) {
            std::string s = "mismatching parentheses:";

            for (const auto &e : l) {
                s += ' ' + e;
            }
            return s;
        }

        if (l.back() == ")") {
            /* function */
            vstring_t vec_type;
            l.pop_back();

            auto it = l.begin();

            /* look for the first opening parentheses */
            for ( ; it != l.end(); it++) {
                if (*it == "(") {
                    it++;
                    break;
                }
                vec_type.push_back(*it);
            }

            /* symbol */
            proto.symbol = vec_type.back();
            vec_type.pop_back();

            /* type */
            for (const auto &e : vec_type) {
                proto.type += e + ' ';
            }

            /* args */
            for ( ; it != l.end(); it++) {
                proto.args += *it + ' ';
            }

            if (proto.args.empty()) {
                proto.args = "void";
            }

            utils::strip_spaces(proto.type);
            utils::strip_spaces(proto.args);

            if (proto.type.empty() || proto.symbol.empty()) {
                std::string s = concat_function_prototype(proto);
                utils::strip_spaces(s);
                return "failed to read prototype: " + s;
            }
        } else {
            /* object */
            proto.symbol = l.back();
            l.pop_back();

            for (const auto &e : l) {
                proto.type += e + ' ';
            }

            utils::strip_spaces(proto.type);

            if (proto.type.empty() || proto.symbol.empty()) {
                std::string s = proto.type + ' ' + proto.symbol;
                utils::strip_spaces(s);
                return "failed to read prototype: " + s;
            }
        }

        vproto.push_back(proto);
    }

    return {};
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
    std::string msg;
    std::vector<vstring_t> vec_tokens;
    vproto_t vproto;

    std::string file_or_stdin = (m_ifile == "-") ? "<STDIN>" : "file: " + m_ifile;

    /* read and tokenize input */
    msg = tokenize_stream(m_ifs, vec_tokens, m_second_attempt);

    if (!msg.empty()) {
        throw error(file_or_stdin + '\n' + msg);
    }

    /* get prototypes from tokens */
    msg = get_prototypes(vec_tokens, vproto);

    if (!msg.empty()) {
        if (msg.find("(*") != std::string::npos ||
            msg.find("( *") != std::string::npos)
        {
            msg += "\nhint: use typedefs for function pointers!";
        }

        throw error(file_or_stdin + '\n' + msg);
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
        msg = get_parameter_names(e, m_parameter_names);

        if (!msg.empty()) {
            throw error(msg);
        }
    }

    /* copy */
    filter_and_copy_symbols(vproto);
}
