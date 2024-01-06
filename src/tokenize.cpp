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

#include <algorithm>
#include <iostream>
#include <fstream>
#include <ranges>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

#include "strcasecmp.h"
#include "tokenize.hpp"


/* extract argument names from args list */
static bool get_argument_names(proto_t &proto)
{
    bool fptr_name = false;
    bool fptr_args = false;
    int scope = 0;
    std::string out, token;
    vstring_t arg;

    const char *keywords[] = {
        "char",
        "int", "long", "short",
        "float", "double",
        "signed", "unsigned",
        "const", "volatile",
        "struct",
        "void",
        "extern",
        NULL
    };

    /* compare s with a list of very basic types and keywords
     * to guess if it could be a parameter name */
    auto is_keyword = [] (const char **list, const std::string &s) -> bool
    {
        for (const char **p = list; *p != NULL; p++) {
            if (strcasecmp(s.c_str(), *p) == 0) {
                return true;
            }
        }
        return false;
    };

    /* void; nothing to do */
    if (proto.args.empty() || strcasecmp(proto.args.c_str(), "void") == 0) {
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

    auto add_space = [] (std::string &line) {
        if (!line.empty() && line.back() != ' ') {
            line += ' ';
        }
    };

    auto save_line = [] (std::string &input, vstring_t &vec)
    {
        strip_spaces(input);
        if (input.empty()) return; /* had only whitespaces */
        auto line = input;
        input.clear(); /* empty variable */
        strip_spaces(line);
        vec.push_back(line);
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

        case '\n':
            if (comment == '\n') {
                /* commentary end */
                comment = 0;
            }
            add_space(line);
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

#ifdef __GNUC__
        /* function name, argument, etc. */
        case 'a'...'z':
        case 'A'...'Z':
        case '0'...'9':
        case '_':
            line += c;
            break;
#endif //__GNUC__

        /* add character */
        default:
#ifndef __GNUC__
            /* function name, argument, etc. */
            if (c == '_' || (c>='a' && c<='z') || (c>='A' && c<='Z') || (c>='0' && c<='9')) {
                line += c;
                break;
            }
#endif //!__GNUC__
            add_space(line);
            if (isspace(c)) break;
            line += c;
            line += ' ';
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

    if (!get_argument_names(proto)) {
        return false;
    }

    /* replace string in proto.args (macro) */
#define REPL(a, b) \
    for (size_t pos = 0; (pos = proto.args.find(a, pos)) != std::string::npos; \
        pos += sizeof(b) - 1) \
    { \
        proto.args.replace(pos, sizeof(a) - 1, b); \
    }

    /* format args */
    if (proto.args.empty()) {
        proto.args = "void";
    } else {
        REPL("* ", "*");
        REPL(" ,", ",");
        REPL("( ", "(");
        REPL(" )", ")");
        REPL(") (", ")(");
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
bool tokenize::tokenize_file(const std::string &ifile)
{
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

    return true;
}
