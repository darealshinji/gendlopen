/*
Licensed under the MIT License <http://opensource.org/licenses/MIT>.
SPDX-License-Identifier: MIT
Copyright (c) 2023 djcj@gmx.de

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
*/

#include <algorithm>
#include <iostream>
#include <fstream>
#include <sstream>
#include <regex>
#include <string>
#include <vector>
#include <cstdlib>
#include <ctype.h>
#include <string.h>

#include "cin_ifstream.hpp"
#include "gendlopen.hpp"


/* strip leading and trailing spaces */
inline static void strip_spaces(std::string &in)
{
    while (isspace(in.back())) in.pop_back();
    while (isspace(in.front())) in.erase(0, 1);
}

/* compare s with a list of very basic types and keywords
 * to guess if it could be a parameter name */
static bool is_keyword(const std::string &s)
{
    const char *list[] = {
        "*",
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

    for (const char **p = list; *p != NULL; p++) {
        if (strcasecmp(s.c_str(), *p) == 0) {
            return true;
        }
    }

    return false;
}

/* extract argument names from args list */
static bool get_argument_names(gendlopen::proto_t &proto)
{
    std::string out, token;
    std::vector<std::string> arg;

    /* void; nothing to do */
    if (proto.args.empty() || strcasecmp(proto.args.c_str(), "void") == 0) {
        return true;
    }

    /* needed for parsing */
    auto in = proto.args;
    gendlopen::replace_string(",", " , ", in);
    gendlopen::replace_string("*", " * ", in);
    in += " ,";

    std::istringstream iss(in);

    /* tokenize argument list */
    while (iss >> token) {
        if (token != ",") {
            arg.push_back(token);
            continue;
        }

        if (arg.size() < 2 || is_keyword(arg.back())) {
            std::cerr << "error: a parameter name is missing: "
                << proto.symbol << '(' << proto.args << ");" << std::endl;
            return false;
        }

        out += ", ";
        out += arg.back();
        arg.clear();
    }

    if (out.starts_with(", ")) {
        out.erase(0, 2);
    }

    proto.notype_args = out;

    return true;
}

/* read input and strip comments */
static std::vector<std::string> read_input(
    cin_ifstream &ifs,
    std::string &s_typedef)
{
    std::vector<std::string> vproto;
    char c, comment = 0;
    std::string line;

    /* read input into vector */
    while (ifs.get(c) && ifs.good())
    {
        if (comment != 0 && comment != c) {
            continue;
        }

        switch (c) {
            /* end of sequence -> tokenize and save into vector */
            case ';': {
                std::istringstream iss(line);
                std::string token;
                line.clear();

                while (iss >> token) {
                    line += token + " ";
                }

                strip_spaces(line);

                if (line.starts_with("typedef")) {
                    s_typedef += line + ";\n";
                } else {
                    vproto.push_back(line);
                }

                line.clear();
                break;
            }

            /* commentary begin */
            case '/': {
                if (ifs.peek() == '*') {
                    ifs.ignore();
                    comment = '*';
                    continue;
                } else if (ifs.peek() == '/') {
                    ifs.ignore();
                    comment = '\n';
                    continue;
                }
                line += c;
                break;
            }

            /* commentary end */
            case '\n':
                if (comment == '\n') {
                    comment = 0;
                }
                line += c;
                break;
            case '*': {
                if (comment == '*' && ifs.peek() == '/') {
                    ifs.ignore();
                    comment = 0;
                    continue;
                } else if (comment == 0) {
                    line += c;
                }
                break;
            }

            /* append character to line */
            default:
                line += c;
                break;
        }
    }

    ifs.close();

    return vproto;
}

bool gendlopen::tokenize_function(const std::string &s)
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

    /* format args */
    if (proto.args.empty()) {
        proto.args = "void";
    } else {
        replace_string(" ,", ",", proto.args);
    }

    m_prototypes.push_back(proto);

    return true;
}

bool gendlopen::tokenize_object(const std::string &s)
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

bool gendlopen::tokenize(const std::string &ifile)
{
    /* open file for reading */
    cin_ifstream ifs(ifile);

    if (!ifs.is_open()) {
        std::cerr << "error: failed to open file for reading: " << ifile << std::endl;
        return false;
    }

    /* read and tokenize input */
    auto vproto_s = read_input(ifs, m_typedefs);

    if (!m_typedefs.empty()) {
        m_typedefs.insert(0, "\n/* typedefs */\n");
    }

    /* process prototypes */
    for (const auto &s : vproto_s) {
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
    std::vector<std::string> list;

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
