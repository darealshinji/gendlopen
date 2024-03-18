/**
 * The MIT License (MIT)
 *
 * Copyright (C) 2023-2024 djcj@gmx.de
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
#include <ranges>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "cin_ifstream.hpp"
#include "common.hpp"
#include "tokenize.hpp"

using common::replace_string;
using common::same_string_case;
using common::strip_spaces;
using common::range;


namespace /* anonymous */
{

/* extract argument names from args list */
bool get_parameter_names(proto_t &proto)
{
    typedef enum {
        e_normal = 0,
        e_fptr_name = 1,
        e_fptr_args = 2
    } e_type_t;

    e_type_t search = e_normal;
    int scope = 0;
    std::string out, token;
    vstring_t arg;

    const char * const keywords[] = {
        "char",
        "int", "long", "short",
        "float", "double",
        "signed", "unsigned",
        "const", "volatile",
        "struct", "union", "enum",
        "restrict",
        "void",
        NULL
    };

    /* compare s with a list of very basic types and keywords
     * to guess if it could be a parameter name */
    auto is_keyword = [] (const char* const *list, const std::string &s) -> bool
    {
        for (auto p = list; *p != NULL; p++) {
            if (same_string_case(s, *p)) {
                return true;
            }
        }
        return false;
    };

    /* void or empty: nothing to do */
    if (proto.args.empty() || same_string_case(proto.args, "void")) {
        return true;
    }

    /* trailing comma is needed for parsing */
    std::istringstream iss(proto.args + " ,");

    /* tokenize argument list */
    while (iss >> token) {
        if (search == e_fptr_args) {
            /* skip through function pointer arguments list */
            if (token == "(") {
                scope++;
            } else if (token == ")") {
                scope--;
            }

            if (scope < 1) {
                scope = 0;
                search = e_normal;
            }
        } else if (search == e_fptr_name) {
            /* function pointer name */
            if (token == ")") {
                search = e_fptr_args;
            } else {
                arg.push_back(token);
            }
        } else if (token == "(") {
            /* begin of a function pointer name */
            search = e_fptr_name;
        } else if (token == ",") {
            /* argument list separator */
            if (arg.size() < 2 || arg.back().back() == '*' ||
                is_keyword(keywords, arg.back()))
            {
                std::cerr << "error: a parameter name is missing: "
                    << proto.symbol << '(' << proto.args << ");" << std::endl;
                std::cerr << "maybe try again with `--skip-parameter-names'" << std::endl;
                return false;
            }

            out += ", ";
            out += arg.back();
            arg.clear();
        } else {
            arg.push_back(token);
        }
    }

    if (out.starts_with(", ")) {
        out.erase(0, 2);
    }

    proto.notype_args = out;

    return true;
}

/* read input and strip comments */
bool read_input(cin_ifstream &ifs, vstring_t &vec)
{
    std::string line;
    char c, comment = 0;
    uint8_t nullbytes = 0;

    auto add_space = [] (std::string &line) {
        if (!line.empty() && line.back() != ' ') {
            line += ' ';
        }
    };

    auto add_element = [add_space] (std::string &line, char c) {
        add_space(line);
        line += c;
        line += ' ';
    };

    auto save_line = [] (std::string &line, vstring_t &vec) {
        strip_spaces(line);

        if (!line.empty()) {
            vec.push_back(line);
            line.clear();
        }
    };

    /* read input into vector */
    while (ifs.get(c) && ifs.good())
    {
        if (comment != 0 && comment != c) {
            continue;
        }

        switch (c)
        {
        /* end of sequence -> save line buffer */
        case ';':
            save_line(line, vec);
            break;

        case '/':
            if (ifs.peek() == '*') {
                /* commentary begin */
                ifs.ignore();
                comment = '*';
            } else if (ifs.peek() == '/') {
                /* commentary begin */
                ifs.ignore();
                comment = '\n';
            } else {
                add_element(line, c);
            }
            break;

        case '*':
            if (comment == '*' && ifs.peek() == '/') {
                /* commentary end */
                ifs.ignore();
                comment = 0;
            } else if (comment == 0) {
                add_element(line, c);
            }
            break;

        case '\n':
            if (comment == '\n') {
                comment = 0; /* commentary end */
            }
            [[fallthrough]];

        /* space */
        case ' ':
        case '\t':
        case '\r':
        case '\v':
        case '\f':
            add_space(line);
            break;

        /* null byte */
        case 0x00:
            /* ignore a couple of them, otherwise we should assume it's
             * something like UTF-16 or a binary file */
            if (++nullbytes > 8) {
                std::cerr << "error: too many null bytes (\\0) found in input!" << std::endl;
                std::cerr << "input text must be ASCII or UTF-8 formatted" << std::endl;
                return false;
            }
            break;

        /* add character */
        default:
            /* function name, argument, etc. */
            if (range(c, 'a','z') ||
                range(c, 'A','Z') ||
                range(c, '0','9') || c == '_')
            {
                line += c;
            } else {
                add_element(line, c);
            }
            break;
        }

        /* stop if the sequence gets unrealistically long */
        if (line.size() > 1000) {
            line.erase(80, std::string::npos);
            std::cerr << "error: the following sequence is exceeding 1000 bytes:" << std::endl;
            std::cerr << line << " <...>" << std::endl;
            return false;
        }
    }

    /* in case the last prototype didn't end on semicolon */
    save_line(line, vec);

    return true;
}

/* assume a function prototype and tokenize */
bool tokenize_function(const std::string &s, vproto_t &prototypes, bool skip_parameter_names)
{
    const std::regex reg(
        "(.*?[\\*|\\s])"  /* type */
        "([A-Za-z0-9_]*)" /* symbol */
        "[?|\\s]*\\("
        "(.*?)\\)"        /* args */
    );

    std::smatch m;

    if (!std::regex_match(s, m, reg) || m.size() != 4) {
        return false;
    }

    proto_t proto = { m[1], m[2], m[3], "" };
    strip_spaces(proto.type);
    strip_spaces(proto.args);

    if (skip_parameter_names) {
        /* normally unused, but just in case */
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
        "(.*?[\\*|\\s])"  /* type */
        "([A-Za-z0-9_]*)" /* symbol */
    );

    std::smatch m;

    if (!std::regex_match(s, m, reg) || m.size() != 3) {
        return false;
    }

    proto_t obj = { m[1], m[2], {}, {} };
    strip_spaces(obj.type);

    /* remove "extern" keyword */
    if (obj.type.starts_with("extern ")) {
        obj.type.erase(0, 7);
    }

    objects.push_back(obj);

    return true;
}

} /* end anonymous namespace */


void tokenize::copy_symbols(
    const std::string &prefix,
    const vstring_t &symbols,
    vproto_t &prototypes,
    vproto_t &objects)
{
    /* add to vector only if the symbol wasn't already saved */
    auto pb_if_unique = [] (vproto_t &vec, const proto_t &proto) {
        for (const auto &e : vec) {
            if (e.symbol == proto.symbol) {
                return;
            }
        }
        vec.push_back(proto);
    };

    /* copy symbols beginning with prefix */
    auto copy_if_prefixed = [pb_if_unique] (const std::string &pfx, const vproto_t &from, vproto_t &to) {
        for (const auto &e : from) {
            if (e.symbol.starts_with(pfx)) {
                pb_if_unique(to, e);
            }
        }
    };

    /* copy symbols whose names are on the symbols vector list */
    auto copy_if_whitelisted = [pb_if_unique] (const vstring_t &list, const vproto_t &from, vproto_t &to) {
        for (const auto &e : from) {
            if (std::find(list.begin(), list.end(), e.symbol) != list.end()) {
                pb_if_unique(to, e);
            }
        }
    };

    /* copy all symbols */
    if (prefix.empty() && symbols.empty()) {
        prototypes = m_prototypes;
        objects = m_objects;
        return;
    }

    if (!prefix.empty()) {
        copy_if_prefixed(prefix, m_prototypes, prototypes);
        copy_if_prefixed(prefix, m_objects, objects);
    }

    if (!symbols.empty()) {
        copy_if_whitelisted(symbols, m_prototypes, prototypes);
        copy_if_whitelisted(symbols, m_objects, objects);
    }
}

/* read input and tokenize */
bool tokenize::tokenize_file(const std::string &ifile, bool skip_parameter_names)
{
    cin_ifstream ifs;
    vstring_t vec;

    /* open file for reading */
    if (!ifs.open(ifile)) {
        std::cerr << "error: failed to open file for reading: " << ifile << std::endl;
        return false;
    }

    /* read and tokenize input */
    if (!read_input(ifs, vec)) {
        return false;
    }

    ifs.close();

    /* process prototypes */
    for (const auto &s : vec) {
        if (!tokenize_function(s, m_prototypes, skip_parameter_names) &&
            !tokenize_object(s, m_objects))
        {
            std::cerr << "error: malformed prototype:\n" << s << std::endl;
            return false;
        }
    }

    /* nothing found? */
    if (m_prototypes.size() == 0 && m_objects.size() == 0) {
        std::cerr << "error: no function or object prototypes found in file: " << ifile << std::endl;
        return false;
    }

    /* check for duplicates */
    vstring_t list;

    for (const auto &s : m_prototypes) {
        list.push_back(s.symbol);
    }

    for (const auto &s : m_objects) {
        list.push_back(s.symbol);
    }

    std::sort(list.begin(), list.end());
    auto it = std::ranges::adjacent_find(list);

    if (it != list.end()) {
        std::cerr << "error: multiple definitions of symbol `" << *it
            << "' found in file: " << ifile << std::endl;
        return false;
    }

    /* format args */
    for (auto &s : m_prototypes) {
        replace_string("* ", "*", s.args);
        replace_string(" ,", ",", s.args);

        replace_string("( ", "(", s.args);
        replace_string(" )", ")", s.args);
        replace_string(") (", ")(", s.args);

        replace_string("[ ", "[", s.args);
        replace_string("] ", "]", s.args);
        replace_string(" [", "[", s.args);
        replace_string(" ]", "]", s.args);
    }

    return true;
}
