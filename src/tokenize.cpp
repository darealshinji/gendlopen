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


namespace /* anonymous */
{

/* compare s with a list of very basic types and keywords
 * to guess if it could be a parameter name */
bool keyword_or_type(const std::string &s)
{
    const std::list<const char *> keywords =
    {
        "char",
        "int", "long", "short",
        "float", "double",
        "signed", "unsigned",
        "const", "volatile",
        "struct", "union", "enum",
        "restrict",
        "void"
    };

    for (const auto &e : keywords) {
        if (utils::eq_str_case(s, e)) {
            return true;
        }
    }

    return false;
}

/* extract argument names from args list */
bool get_parameter_names(proto_t &proto)
{
    typedef enum {
        E_DEFAULT,
        E_FUNC_PTR_NAME,
        E_FUNC_PTR_PARAM_LIST
    } e_type_t;

    e_type_t search = E_DEFAULT;
    int scope = 0;
    std::string out, token;
    vstring_t arg;

    /* nothing to do if "void" or empty*/
    if (proto.args.empty() || utils::eq_str_case(proto.args, "void")) {
        return true;
    }

    /* trailing comma is needed for parsing */
    std::istringstream iss(proto.args + " ,");

    /* tokenize argument list */
    while (iss >> token) {
        /* note: a full check for mismatching
         * parenthesis was already done */
        if (token == "(") {
            scope++;
        } else if (token == ")") {
            scope--;
        }

        switch (search)
        {
        /* function pointer name */
        case E_FUNC_PTR_NAME:
            if (token == ")") {
                /* end of name sequence */
                search = E_FUNC_PTR_PARAM_LIST;
            } else {
                /* add name */
                arg.push_back(token);
            }
            break;

        /* parameter list of a function pointer */
        case E_FUNC_PTR_PARAM_LIST:
            if (scope == 0) {
                search = E_DEFAULT;
            }
            break;

        case E_DEFAULT:
            if (token == "(") {
                /* begin of a function pointer name */
                search = E_FUNC_PTR_NAME;
            } else if (token == ",") {
                /* argument list separator */
                if (arg.size() < 2 ||  /* must be at least 2 to hold a type and parameter name */
                    arg.back().back() == '*' ||  /* pointer type without parameter name */
                    keyword_or_type(arg.back()))  /* a reserved keyword or a very basic type (i.e. "int") */
                {
                    std::cerr << "error: a parameter name is missing:" << std::endl;
                    std::cerr << proto.type << ' ' << proto.symbol << '(' << proto.args << ");" << std::endl;
                    std::cerr << "hint: try again with `-skip-param'" << std::endl;
                    return false;
                }

                out += ", ";
                out += arg.back();
                arg.clear();
            } else {
                arg.push_back(token);
            }
            break;
        }
    }

    if (out.starts_with(',')) {
        out.erase(0, 1);
    }

    utils::strip_spaces(out);
    proto.notype_args = out;

    return true;
}

/* decide what to do with the current character */
bool handle_character(cio::ifstream &ifs, std::string &line, char &c, char &comment)
{
    auto add_element = [] (std::string &line, char &c) {
        if (!line.empty() && line.back() != ' ') {
            line += ' ';
        }
        line += c;
        line += ' ';
    };

    switch (c)
    {
    /* end of sequence -> save line buffer */
    case ';':
        return true;

    case '/':
        if (ifs.peek() == '*') {
            /* asterisk commentary begin */
            ifs.ignore();
            comment = '*';
        } else if (ifs.peek() == '/') {
            /* double forward slash commentary begin */
            ifs.ignore();
            comment = '\n';
        } else {
            add_element(line, c);
        }
        break;

    case '*':
        if (comment == '*' && ifs.peek() == '/') {
            /* asterisk commentary end */
            ifs.ignore();
            comment = 0;
        } else if (comment == 0) {
            add_element(line, c);
        }
        break;

    case '\n':
        if (comment == '\n') {
            /* double forward slash commentary end */
            comment = 0;
        }
        /* treat newline as space */
        [[fallthrough]];

    /* space */
    case ' ':
    case '\t':
    case '\r':
    case '\v':
    case '\f':
        if (!line.empty() && line.back() != ' ') {
            line += ' ';
        }
        break;

    case '_':
        line += c;
        break;

    /* add character */
    default:
        /* function name, argument, etc. */
        if (utils::range(c, 'a','z') ||
            utils::range(c, 'A','Z') ||
            utils::range(c, '0','9'))
        {
            line += c;
        } else {
            add_element(line, c);
        }
        break;
    }

    return false;
}

/* read input and strip comments */
void read_input(cio::ifstream &ifs, vstring_t &vec)
{
    std::string line;
    char c, comment = 0;

    /* read input into vector */
    while (ifs.get(c) && ifs.good())
    {
        /* we're skipping through a comment section */
        if (comment != 0 && comment != c) {
            continue;
        }

        if (handle_character(ifs, line, c, comment)) {
            /* the end of a sequence was reached */
            utils::strip_spaces(line);

            if (!line.empty()) {
                vec.push_back(line);
                line.clear();
            }
        }
    }

    /* append line in case the last prototype
     * didn't end on semicolon */

    utils::strip_spaces(line);

    if (!line.empty()) {
        vec.push_back(line);
    }
}

/* assume a function prototype and tokenize */
bool tokenize_function(const std::string &s, vproto_t &prototypes, bool skip_parameter_names)
{
    const std::regex reg(
        R"((.*?[\*|\s]))"  /* type */
         "([A-Za-z0-9_]*)" /* symbol */
        R"([?|\s]*\()"
        R"((.*?)\))"       /* args */
    );

    std::smatch m;

    if (!std::regex_match(s, m, reg) || m.size() != 4) {
        return false;
    }

    proto_t proto = { m[1], m[2], m[3], "" };

    utils::strip_spaces(proto.type);
    utils::strip_spaces(proto.args);
    utils::delete_prefix(proto.type, "extern ");

    if (proto.type.empty() || proto.symbol.empty()) {
        return false;
    }

    /* check for mismatching parentheses */
    int scope = 0;

    for (const char &c : proto.args) {
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

    /* set proto.notype_args */
    if (skip_parameter_names) {
        /* just in case */
        proto.notype_args = "/* disabled with -skip-param !! */";
    } else if (!get_parameter_names(proto)) {
        return false;
    }

    if (proto.args.empty()) {
        proto.args = "void";
    }

    prototypes.push_back(proto);

    return true;
}

/* assume an object prototype and tokenize */
bool tokenize_object(const std::string &s, vproto_t &objects)
{
    const std::regex reg(
        R"((.*?[\*|\s]))"  /* type */
         "([A-Za-z0-9_]*)" /* symbol */
    );

    std::smatch m;

    if (!std::regex_match(s, m, reg) || m.size() != 3) {
        return false;
    }

    proto_t obj = { m[1], m[2], {}, {} };

    utils::strip_spaces(obj.type);
    utils::delete_prefix(obj.type, "extern ");

    if (obj.type.empty() || obj.symbol.empty()) {
        return false;
    }

    objects.push_back(obj);

    return true;
}

/* add to vector only if the symbol wasn't already saved */
void push_back_if_unique(vproto_t &vec, const proto_t &proto)
{
    for (const auto &e : vec) {
        if (e.symbol == proto.symbol) {
            return;
        }
    }

    vec.push_back(proto);
}

} /* end anonymous namespace */


void gendlopen::filter_and_copy_symbols(vproto_t &tmp_proto, vproto_t &tmp_objs)
{
    /* copy symbols beginning with prefix */
    auto copy_if_prefixed = [this] (const vproto_t &from, vproto_t &to) {
        for (const auto &e : from) {
            if (utils::is_prefixed(e.symbol, m_prefix)) {
                push_back_if_unique(to, e);
            }
        }
    };

    /* copy symbols whose names are on the symbols vector list */
    auto copy_if_whitelisted = [this] (const vproto_t &from, vproto_t &to) {
        for (const auto &e : from) {
            if (std::find(m_symbols.begin(), m_symbols.end(), e.symbol) != m_symbols.end()) {
                push_back_if_unique(to, e);
            }
        }
    };

    if (m_prefix.empty() && m_symbols.empty()) {
        /* copy all symbols */
        m_prototypes = tmp_proto;
        m_objects = tmp_objs;
    } else {
        if (!m_prefix.empty()) {
            copy_if_prefixed(tmp_proto, m_prototypes);
            copy_if_prefixed(tmp_objs, m_objects);
        }

        if (!m_symbols.empty()) {
            copy_if_whitelisted(tmp_proto, m_prototypes);
            copy_if_whitelisted(tmp_objs, m_objects);
        }
    }
}

/* read input and tokenize */
void gendlopen::tokenize(cio::ifstream &ifs)
{
    vstring_t vec, duplist;
    vproto_t tmp_proto, tmp_objs;

    /* read and tokenize input */
    read_input(ifs, vec);
    ifs.close();

    /* process prototypes */
    for (const auto &str : vec) {
        if (!tokenize_function(str, tmp_proto, m_skip_parameter_names) &&
            !tokenize_object(str, tmp_objs))
        {
            std::string msg = "malformed prototype:\n";
            msg += str;
            throw error(msg);
        }
    }

    /* nothing found? */
    if (tmp_proto.empty() && tmp_objs.empty()) {
        std::string msg = "no function or object prototypes found in file: ";
        msg += m_ifile;
        throw error(msg);
    }

    /* check for duplicates */
    for (const auto &s : tmp_proto) {
        duplist.push_back(s.symbol);
    }

    for (const auto &s : tmp_objs) {
        duplist.push_back(s.symbol);
    }

    std::sort(duplist.begin(), duplist.end());
    auto it = std::ranges::adjacent_find(duplist);

    if (it != duplist.end()) {
        std::string msg = "multiple definitions of symbol `";
        msg += *it;
        msg += "' found in file: ";
        msg += m_ifile;
        throw error(msg);
    }

    /* copy */
    filter_and_copy_symbols(tmp_proto, tmp_objs);

    /* format args */
    for (auto &p : m_prototypes) {
        utils::replace("* ", "*", p.args);
        utils::replace(" ,", ",", p.args);

        utils::replace("( ", "(", p.args);
        utils::replace(" )", ")", p.args);
        utils::replace(") (", ")(", p.args);

        utils::replace("[ ", "[", p.args);
        utils::replace("] ", "]", p.args);
        utils::replace(" [", "[", p.args);
        utils::replace(" ]", "]", p.args);
    }
}
