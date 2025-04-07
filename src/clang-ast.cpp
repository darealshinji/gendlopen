/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2024-2025 Carsten Janssen

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
**/

/* read prototypes from a Clang AST file */

/***
For reference the AST from helloworld.h:

TranslationUnitDecl 0x5b4b21b885b8 <<invalid sloc>> <invalid sloc>
<...>
|-VarDecl 0x59d98537ea20 <line:31:1, col:35> col:35 helloworld_callback 'helloworld_cb_t':'void (*)(const char *)' extern
| `-VisibilityAttr 0x59d98537ea88 <line:21:44, col:65> Default
|-FunctionDecl 0x59d98537ec48 <col:28, line:34:40> col:24 helloworld_init 'helloworld *()'
| `-VisibilityAttr 0x59d98537ecf0 <line:21:44, col:65> Default
|-FunctionDecl 0x59d98537efa8 <col:28, line:35:67> col:24 helloworld_init_argv 'helloworld *(int, char **)'
| |-ParmVarDecl 0x59d98537ed80 <col:45, col:49> col:49 argc 'int'
| |-ParmVarDecl 0x59d98537ee80 <col:55, col:66> col:61 argv 'char **'
| `-VisibilityAttr 0x59d98537f060 <line:21:44, col:65> Default
|-FunctionDecl 0x59d98537f200 <col:28, line:38:48> col:17 helloworld_hello 'void (helloworld *)'
| |-ParmVarDecl 0x59d98537f0f0 <col:34, col:46> col:46 hw 'helloworld *'
| `-VisibilityAttr 0x59d98537f2b0 <line:21:44, col:65> Default
|-FunctionDecl 0x59d985361a30 <col:28, line:39:86> col:17 helloworld_hello2 'void (helloworld *, void (*)(const char *))'
| |-ParmVarDecl 0x59d98537f340 <col:35, col:47> col:47 hw 'helloworld *'
| |-ParmVarDecl 0x59d985361910 <col:51, col:85> col:58 helloworld_cb 'void (*)(const char *)'
| `-VisibilityAttr 0x59d985361ae8 <line:21:44, col:65> Default
|-FunctionDecl 0x59d985361c08 <col:28, line:42:50> col:17 helloworld_release 'void (helloworld *)'
| |-ParmVarDecl 0x59d985361b78 <col:36, col:48> col:48 hw 'helloworld *'
| `-VisibilityAttr 0x59d985361cb8 <line:21:44, col:65> Default
`-FunctionDecl 0x59d985361eb0 <col:28, line:45:72> col:16 helloworld_fprintf 'int (FILE *, const char *, ...)'
  |-ParmVarDecl 0x59d985361d48 <col:35, col:41> col:41 stream 'FILE *'
  |-ParmVarDecl 0x59d985361dc8 <col:49, col:61> col:61 format 'const char *'
  `-VisibilityAttr 0x59d985361f68 <line:21:44, col:65> Default


***/

#include <stdio.h>
#include <algorithm>
#include <regex>
#include <string>
#include <vector>
#include "gendlopen.hpp"
#include "lex.h"
#include "types.hpp"
#include "utils.hpp"


/* command line option strings used in error messages */
#define OPT_SYMBOL_NAME     "-S"
#define OPT_SYMBOL_PREFIX   "-P"
#define OPT_AST_ALL_SYMBOLS "-ast-all-symbols"
#define OPT_SKIP_PARAM      "-param=skip"
#define OPT_CREATE_PARAM    "-param=create"



namespace /* anonymous */
{

enum {
    M_DEFAULT,  /* no filter */
    M_PREFIX,   /* look for prefixed symbols */
    M_LIST,     /* look for whitelisted symbols */
    M_PFX_LIST  /* look for prefixed and/or whitelisted symbols */
};


/* strip ANSI colors from line */
std::string strip_line(const char *line)
{
    const std::regex reg(R"(\x1B\[[0-9;]*m)");

    std::string s = line;

    /* remains of Windows line endings */
    if (utils::ends_with(s, '\r')) {
        s.pop_back();
    }

    return std::regex_replace(s, reg, "");
}

/* get function parameter declaration */
bool get_parameters(std::string &args, std::string &notype_args, char letter)
{
    std::smatch m;

    const std::regex reg(
        "^.*?-ParmVarDecl 0x.*?"
        "'(.*?)'.*"  /* type */
    );

    if (*yytext == 0) {
        return false;
    }

    const std::string line = strip_line(yytext);

    if (!std::regex_match(line, m, reg) || m.size() != 2) {
        return false;
    }

    notype_args += letter;
    notype_args += ", ";

    /* search for function pointer */
    std::string str = m[1].str();
    size_t pos = str.find("(*)");

    if (pos == std::string::npos) {
        /* regular parameter */
        args += str;

        if (!utils::ends_with(args, '*')) {
            args += ' ';
        }
        args += letter;
        args += ", ";
    } else {
        /* function pointer */
        std::string ins = "(*x)";
        ins[2] = letter;

        str.replace(pos, 3, ins);
        args += str + ", ";
    }

    return true;
}

} /* end anonymous namespace */


/* get function or variable declaration */
bool gendlopen::get_declarations(int mode)
{
    decl_t decl;
    proto_t proto;
    std::smatch m;

    auto find_and_erase = [] (vstring_t &v, const std::string &str)
    {
#ifdef __cpp_lib_erase_if
        return std::erase(v, str);
#else
        const auto it = std::remove(v.begin(), v.end(), str);
        const auto rv = v.end() - it;
        v.erase(it, v.end());
        return rv;
#endif
    };

    const std::regex reg_func(
        "^[|`]-FunctionDecl 0x.*?"
        " ([A-Za-z0-9_]*?) "  /* symbol */
        "'(.*?)'.*"           /* type */
    );

    const std::regex reg_var(
        "^[|`]-VarDecl 0x.*?"
        " ([A-Za-z0-9_]*?) "  /* symbol */
        "'(.*?)'.*"           /* type */
    );

    if (*yytext == 0) {
        return false;
    }

    const std::string line = strip_line(yytext);
    const bool function = (line.size() >= 3 && line.at(2) == 'F');

    if (!std::regex_match(line, m, function ? reg_func : reg_var) || m.size() != 3) {
        return false;
    }

    /* check symbol prefix or list */
    decl.symbol = m.str(1);

    switch (mode)
    {
    case M_PREFIX:
        if (!utils::is_prefixed(decl.symbol, m_prefix_list)) {
            return false; /* not prefixed */
        }
        break;

    case M_PFX_LIST:
        if (utils::is_prefixed(decl.symbol, m_prefix_list)) {
            break; /* prefixed */
        }
        /* not prefixed */
        [[fallthrough]];

    case M_LIST:
        /* erase from list if found */
        if (find_and_erase(m_symbol_list, decl.symbol) == 0) {
            return false; /* not in list */
        }
        break;

    default:
        break;
    }

    if (function) {
        /* function declaration */
        std::string args, notype_args;
        char letter = 'a';
        int rv;

        size_t pos = m.str(2).find('(');

        if (pos == std::string::npos) {
            return false;
        }

        decl.prototype = proto::function;
        decl.type = m.str(2).substr(0, pos);
        utils::strip_spaces(decl.type);

        /* read next lines for parameters */
        while ((rv = yylex()) == LEX_AST_PARMVAR) {
            if (letter > 'z') {
                throw error(decl.symbol + ": too many parameters");
            }

            if (!get_parameters(args, notype_args, letter)) {
                break;
            }
            letter++;
        }

        utils::delete_suffix(args, ", ");
        utils::delete_suffix(notype_args, ", ");

        proto.prototype   = proto::function;
        proto.type        = decl.type;
        proto.symbol      = decl.symbol,
        proto.args        = args;
        proto.notype_args = notype_args;

        m_prototypes.push_back(proto);

        /* continue to analyze the current line stored in yytext buffer */
        return true;
    } else {
        /* variable declaration */
        decl.type = m.str(2);
        utils::strip_spaces(decl.type);

        if (m.str(2).find("(*)") != std::string::npos) {
            decl.prototype = proto::function_pointer;
        } else if (m.str(2).find('[') != std::string::npos) {
            decl.prototype = proto::object_array;
        } else {
            decl.prototype = proto::object;
        }

        proto.prototype = decl.prototype;
        proto.type      = decl.type;
        proto.symbol    = decl.symbol;

        m_objects.push_back(proto);
    }

    return false;
}

/* read Clang AST */
void gendlopen::clang_ast()
{
    int mode = M_DEFAULT;
    int rv;

    /* ignore symbol lists if m_ast_all_symbols was set */
    if (!m_ast_all_symbols) {
        if (m_symbol_list.empty() && m_prefix_list.empty()) {
            throw error("Clang AST: no symbols provided to look for\n"
                        "use `" OPT_SYMBOL_NAME "', `" OPT_SYMBOL_PREFIX "' or `" OPT_AST_ALL_SYMBOLS "'");
        }

        if (m_symbol_list.size() > 0 && m_prefix_list.size() > 0) {
            mode = M_PFX_LIST;
        } else if (m_prefix_list.size() > 0) {
            mode = M_PREFIX;
        } else if (m_symbol_list.size() > 0) {
            mode = M_LIST;
        }
    }

    /* read lines */
    while ((rv = yylex()) == LEX_AST_FUNCVAR || rv == LEX_AST_PARMVAR) {
        if (rv == LEX_AST_PARMVAR) {
            /* ignore */
            continue;
        }

        /* uses an inner loop to read parameters */
        while (get_declarations(mode))
        {}

        if (mode == M_LIST && m_symbol_list.empty()) {
            /* nothing left to look for */
            break;
        }
    }

    /* throw an error if not all symbols on the list were found */
    if ((mode == M_LIST || mode == M_PFX_LIST) && !m_symbol_list.empty()) {
        std::string s;

        /* sort list and remove duplicates */
        std::sort(m_symbol_list.begin(), m_symbol_list.end());
        auto last = std::unique(m_symbol_list.begin(), m_symbol_list.end());
        m_symbol_list.erase(last, m_symbol_list.end());

        for (const auto &e : m_symbol_list) {
            s += " " + e;
        }

        throw error("the following symbols were not found:" + s);
    }

    if (m_prototypes.empty() && m_objects.empty()) {
        throw error("no function or object prototypes found in file: " + m_input);
    }

    create_typedefs();
}

