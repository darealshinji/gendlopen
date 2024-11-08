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
#include <regex>
#include <sstream>
#include <string>
#include <vector>
#include <cstdint>
#include "gendlopen.hpp"

#define REGEX_TYPE   "[A-Za-z_][A-Za-z0-9_ \\*]*?"
#define REGEX_SYMBOL "[A-Za-z_][A-Za-z0-9_]*?"
#define REGEX_ARGS   "[A-Za-z0-9_ \\.\\*,\\(\\)\\[\\]]*?"

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

std::string get_param_name(const std::string &line)
{
    const std::regex reg_object(
        "^" REGEX_TYPE " "
        "(" REGEX_SYMBOL ")"
    );

    const std::regex reg_fptr(
        "^" REGEX_TYPE " "
        "\\( \\* (" REGEX_SYMBOL ") \\) "
        "\\( " REGEX_ARGS "\\)"
    );

    std::smatch m;

    if ((std::regex_match(line, m, reg_object) && m.size() == 2) ||
        (std::regex_match(line, m, reg_fptr) && m.size() == 2))
    {
        return m[1].str();
    }

    return {};
}

bool is_object(const std::string &line, proto_t &proto)
{
    const std::regex reg(
        "^(" REGEX_TYPE ") "
        "(" REGEX_SYMBOL ") "
    );

    std::smatch m;

    if (!std::regex_match(line, m, reg) || m.size() != 3) {
        return false;
    }

    proto.prototype = proto::object;
    proto.type = m[1].str();
    proto.symbol = m[2].str();

    return true;
}

bool is_function_or_function_pointer(const std::string &line, proto_t &proto)
{
    const std::regex reg(
        "^(" REGEX_TYPE ") "

        /* symbol */
        "("
                       REGEX_SYMBOL        "|"  /* function */
            "\\( \\* " REGEX_SYMBOL " \\)" "|"  /* function pointer */
            "\\( "     REGEX_SYMBOL " \\)"      /* function with parentheses */
        ") "

        /* args */
        "\\( (" REGEX_ARGS ")\\) "
    );

    std::smatch m;

    if (!std::regex_match(line, m, reg) || m.size() != 4) {
        return false;
    }

    std::string f = m[2].str();

    if (f.starts_with("( *")) {
        /* funtion pointer */
        proto.prototype = proto::function_pointer;
        proto.type = m[1].str() + "(*)(" + m[3].str() + ")";
        proto.symbol = f.substr(4, f.size() - 6);
    } else {
        /* function */
        proto.prototype = proto::function;
        proto.type = m[1].str();
        proto.args = m[3].str();

        if (f.starts_with('(')) {
            /* "( symbol )" */
            proto.symbol = f.substr(2, f.size() - 4);
        } else {
            proto.symbol = f;
        }
    }

    return true;
}

/* save tokens into prototypes */
bool get_prototypes(const std::vector<vstring_t> &vec_tokens, vproto_t &vproto)
{
    for (auto &tokens : vec_tokens) {
        proto_t proto;
        std::string line;

        for (const auto &e : tokens) {
            line += e + ' ';
        }

        if (tokens.size() > 1 && (is_object(line, proto) ||
            is_function_or_function_pointer(line, proto)))
        {
            utils::strip_spaces(proto.type);
            utils::strip_spaces(proto.args);
            vproto.push_back(proto);
        } else {
            std::cerr << "error: " << line << std::endl;
            return false;
        }
    }

    return true;
}

/* extract argument names from args list */
std::string read_parameter_names(proto_t &proto, param::names parameter_names)
{
    vstring_t vec;
    std::string arg;
    int scope = 0;

    /* split parameters, take care of paretheses */
    for (const char &c : proto.args) {
        switch (c)
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
            break;
        case ')':
            scope--;
            break;
        default:
            break;
        }

        arg += c;
    }

    if (!arg.empty()) {
        utils::strip_spaces(arg);
        vec.push_back(arg);
    }

    if (parameter_names == param::create) {
        /* always add parameter names */
        std::string abc = " a";

        proto.args.clear();
        proto.notype_args.clear();

        for (auto &e : vec) {
            if (abc[1] > 'z') {
                return proto.symbol + ": too many parameters to handle";
            }

            if (e == "...") {
                /* va_list */
                proto.notype_args += "...";
            } else {
                /* add parameter name */
                const size_t pos = e.find('(');

                if (pos != std::string::npos && e.find("( * ") == pos) {
                    /* function pointer */
                    e.insert(pos + 3, abc);
                } else {
                    /* object */
                    e += abc;
                }

                proto.notype_args += abc;
                abc[1]++;
            }

            proto.args += e + ", ";
            proto.notype_args += ", ";
        }

        utils::delete_suffix(proto.args, ", ");
        utils::delete_suffix(proto.notype_args, ", ");
    } else {
        /* get parameter names */
        for (auto &e : vec) {
            if (e == "...") {
                /* va_list */
                proto.notype_args += "...";
            } else {
                std::string name = get_param_name(e);

                if (name.empty() || keyword_or_type(name)) {
                    return "a parameter name is missing: " + concat_function_prototype(proto) + ";\n"
                        "hint: try again with `-param=skip' or `-param=create'";
                }

                proto.notype_args += name;
            }

            proto.notype_args += ", ";
        }

        utils::delete_suffix(proto.notype_args, ", ");
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
        /* not a function or a function without parameters */
        if (e.prototype != proto::function || e.args.empty() || e.args == "void") {
            continue;
        }

        /* don't use parameter names */
        if (m_parameter_names == param::skip) {
            continue;
        }

        std::string msg = read_parameter_names(e, m_parameter_names);

        if (!msg.empty()) {
            throw error(msg);
        }
    }

    /* copy */
    filter_and_copy_symbols(vproto);
}
