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

        /* parameter list */
        case E_FUNC_PTR_PARAM_LIST:
            if (token == "(") {
                scope++;
            } else if (token == ")") {
                scope--;
            }

            if (scope < 1) {
                scope = 0;
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
                    std::cerr << "error: a parameter name is missing: "
                        << proto.symbol << '(' << proto.args << ");" << std::endl;
                    std::cerr << "hint: try again with `--skip-parameter-names'" << std::endl;
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
bool read_input(cio::ifstream &ifs, vstring_t &vec)
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

    return true;
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

    if (skip_parameter_names) {
        /* just in case */
        proto.notype_args = "/* disabled !! */";
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

    /* remove "extern" keyword */
    if (obj.type.starts_with("extern ")) {
        obj.type.erase(0, 7);
    }

    objects.push_back(obj);

    return true;
}

/* check for duplicates */
bool has_duplicate_symbols(const vproto_t &proto, const vproto_t &objs, const std::string &ifile)
{
    vstring_t list;

    for (const auto &s : proto) {
        list.push_back(s.symbol);
    }

    for (const auto &s : objs) {
        list.push_back(s.symbol);
    }

    std::sort(list.begin(), list.end());
    auto it = std::ranges::adjacent_find(list);

    if (it != list.end()) {
        std::cerr << "error: multiple definitions of symbol `" << *it << "' found in file: ";
        utils::print_filename(ifile, true);
        return true;
    }

    return false;
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
bool gendlopen::tokenize(cio::ifstream &ifs)
{
    vstring_t vec;
    vproto_t tmp_proto, tmp_objs;

    /* read and tokenize input */
    if (!read_input(ifs, vec)) {
        return false;
    }

    ifs.close();

    /* process prototypes */
    for (const auto &str : vec) {
        if (!tokenize_function(str, tmp_proto, m_skip_parameter_names) &&
            !tokenize_object(str, tmp_objs))
        {
            std::cerr << "error: malformed prototype:\n" << str << std::endl;
            return false;
        }
    }

    /* nothing found? */
    if (tmp_proto.empty() && tmp_objs.empty()) {
        std::cerr << "error: no function or object prototypes found in file: ";
        utils::print_filename(m_ifile, true);
        return false;
    }

    /* check for duplicates */
    if (has_duplicate_symbols(tmp_proto, tmp_objs, m_ifile)) {
        return false;
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

    return true;
}
