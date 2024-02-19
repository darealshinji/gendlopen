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

#include "common.hpp"
#include "tokenize.hpp"

using common::replace_string;
using common::same_string_case;
using common::strip_spaces;


/* extract argument names from args list */
static bool get_argument_names(proto_t &proto)
{
    bool fptr_name = false;
    bool fptr_args = false;
    int scope = 0;
    std::string out, token;
    vstring_t arg;

    const char * const keywords[] = {
        "char",
        "int", "long", "short",
        "float", "double",
        "signed", "unsigned",
        "const", "volatile",
        "struct",
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

    auto in = proto.args;

    /* needed for parsing */
    in += " ,";

    std::istringstream iss(in);

    /* tokenize argument list */
    while (iss >> token) {
        if (fptr_args) {
            /* skip through function pointer arguments list */
            if (token == "(") {
                scope++;
            } else if (token == ")") {
                scope--;
            }

            if (scope < 1) {
                scope = 0;
                fptr_name = fptr_args = false;
            }
        } else if (fptr_name) {
            /* function pointer name */
            if (token == ")") {
                fptr_args = true;
            } else {
                arg.push_back(token);
            }
        } else if (token == "(") {
            /* begin of a function pointer name */
            fptr_name = true;
        } else if (token == ",") {
            /* argument list separator */
            if (arg.size() < 2 || arg.back().back() == '*' ||
                is_keyword(keywords, arg.back()))
            {
                std::cerr << "error: a parameter name is missing: "
                    << proto.symbol << '(' << proto.args << ");" << std::endl;
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
vstring_t tokenize::read_input()
{
    vstring_t vec;
    char c, comment = 0;
    std::string line;

    auto add_space = [] (std::string &line)
    {
        if (!line.empty() && line.back() != ' ') {
            line += ' ';
        }
    };

    auto save_line = [] (std::string &line, vstring_t &vec)
    {
        strip_spaces(line);

        if (!line.empty()) {
            vec.push_back(line);
            line.clear();
        }
    };

    /* read input into vector */
    while (m_ifs.get(c) && m_ifs.good())
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
            if (m_ifs.peek() == '*') {
                /* commentary begin */
                m_ifs.ignore();
                comment = '*';
            } else if (m_ifs.peek() == '/') {
                /* commentary begin */
                m_ifs.ignore();
                comment = '\n';
            } else {
                /* add character */
                add_space(line);
                line += c;
                line += ' ';
            }
            break;

        case '*':
            if (comment == '*' && m_ifs.peek() == '/') {
                /* commentary end */
                m_ifs.ignore();
                comment = 0;
            } else if (comment == 0) {
                /* add character */
                add_space(line);
                line += c;
                line += ' ';
            }
            break;

        case '\n':
            if (comment == '\n') {
                /* commentary end */
                comment = 0;
            }
            add_space(line);
            break;

        /* space */
        case ' ':
        case '\t':
        case '\r':
        case '\v':
        case '\f':
            add_space(line);
            break;

        /* add character */
        default:
            /* function name, argument, etc. */
            if ((c>='a' && c<='z') || (c>='A' && c<='Z') || isdigit(c) || c == '_') {
                line += c;
            } else {
                add_space(line);
                line += c;
                line += ' ';
            }
            break;
        }
    }

    /* in case the last prototype didn't end on semicolon */
    save_line(line, vec);

    m_ifs.close();

    return vec;
}

/* assume a function prototype and tokenize */
bool tokenize::tokenize_function(const std::string &s)
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

    if (m_skip_parameter_names) {
        proto.notype_args = "/* disabled !! */";
    } else if (!get_argument_names(proto)) {
        return false;
    }

    if (proto.args.empty()) {
        proto.args = "void";
    }

    m_prototypes.push_back(proto);

    return true;
}

/* assume an object prototype and tokenize */
bool tokenize::tokenize_object(const std::string &s)
{
    const std::regex reg(
        "(.*?[\\*|\\s])"  /* type */
        "([A-Za-z0-9_]*)" /* symbol */
    );

    std::smatch m;

    if (!std::regex_match(s, m, reg) || m.size() != 3) {
        return false;
    }

    obj_t obj = { m[1], m[2] };
    strip_spaces(obj.type);

    /* remove "extern" keyword */
    if (obj.type.starts_with("extern ")) {
        obj.type.erase(0, 7);
    }

    m_objects.push_back(obj);

    return true;
}

/* read input and tokenize */
bool tokenize::tokenize_file(const std::string &ifile, bool skip_parameter_names)
{
    m_skip_parameter_names = skip_parameter_names;

    /* open file for reading */
    m_ifs.open(ifile);

    if (!m_ifs.is_open()) {
        std::cerr << "error: failed to open file for reading: " << ifile << std::endl;
        return false;
    }

    /* read and tokenize input */
    vstring_t vec = read_input();

    /* process prototypes */
    for (const auto &s : vec) {
        if (!tokenize_function(s) && !tokenize_object(s)) {
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
