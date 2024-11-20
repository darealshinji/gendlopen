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
 *
 * The regular expressions used to identify the prototypes are far from perfect
 * but they are good enough for our purpose.
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

#include "global.hpp"

/* regex macros */
#define R_TYPE   "[a-zA-Z_][a-zA-Z0-9_ \\*]*?"
#define R_SYMBOL "[a-zA-Z_][a-zA-Z0-9_]*?"
#define R_PARAM  "[a-zA-Z0-9_ \\.\\*,\\(\\)\\[\\]]*?"
#define R_ARRAY  "[a-zA-Z0-9_\\[\\] ]*?"


namespace /* anonymous */
{

std::string concat_function_prototype(const proto_t &proto)
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
    const list_t keywords =
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

/* split `%option' line into strings */
void tokenize_options_len(const char *str, const size_t len, vstring_t *options)
{
    /* `options' is set to NULL if reading option
     * lines was disabled by the user */
    if (!options || strncmp(yytext, str, len) != 0) {
        return;
    }

    std::istringstream iss(yytext + len);
    std::string s;

    while (iss >> s) {
        options->push_back(s);
    }
}

template<size_t N>
constexpr void tokenize_options(char const (&str)[N], vstring_t *options)
{
    tokenize_options_len(str, N-1, options);
}

/* tokenize stream into prototype tokens and options */
int tokenize_stream(FILE *fp, std::vector<vstring_t> &vec, vstring_t *options)
{
    int rv = MYLEX_ERROR;
    vstring_t tokens;
    bool loop = true;

    while (loop)
    {
        rv = mylex(fp);

        switch (rv)
        {
        case MYLEX_TOKEN:
            /* don't add "extern" keyword */
            if (strcmp(yytext, "extern") != 0) {
                tokens.push_back(yytext);
            }
            break;

        case MYLEX_SEMICOLON:
            if (!tokens.empty()) {
                /* save declaration */
                vec.push_back(tokens);
                tokens.clear();
            }
            break;

        case MYLEX_OPTION:
            if (yytext[0] == '%') {
                tokenize_options("%option", options);
            } else {
                tokenize_options("\xEF\xBB\xBF%option", options);
            }
            tokens.clear();
            break;

        default:
            /* EOF, error, etc. */
            loop = false;
            break;
        }
    }

    /* push back if last prototype didn't end on semicolon */
    if (!tokens.empty()) {
        vec.push_back(tokens);
    }

    return rv;
}

/* get argument from an option string */
const char *get_argx(const std::string &str, const char *opt, const size_t optlen)
{
    if (strncmp(str.c_str(), opt, optlen) != 0) {
        /* not the argument we're looking for */
        return NULL;
    }

    if (str.size() > optlen) {
        return str.c_str() + optlen;
    }

    /* no argument */
    return NULL;
}

template<size_t N>
constexpr const char *get_arg(const std::string &str, char const (&opt)[N])
{
    return get_argx(str, opt, N-1);
}

/* extract a function parameter name */
std::string get_param_name(const std::string &element)
{
    /* regular parameter type */
    const std::regex reg_object(
        "^" R_TYPE " "

        "(" R_SYMBOL ")"

        "("
            " \\[ \\]"             "|"
            " \\[ " R_ARRAY " \\]" "|"
            /* empty */
        ")"
    );

    /* function pointer */
    const std::regex reg_fptr(
        "^" R_TYPE " "
        "\\( \\* (" R_SYMBOL ") \\) "
        "\\( " R_PARAM "\\)"
    );

    std::smatch m;

    if ((std::regex_match(element, m, reg_object) && m.size() == 3) ||
        (std::regex_match(element, m, reg_fptr) && m.size() == 2))
    {
        return m.str(1);
    }

    return {};
}

/* check for object prototype */
bool is_object(const std::string &line, proto_t &proto)
{
    const std::regex reg(
        "^(" R_TYPE ") "

        "(" R_SYMBOL ")"

        "("
            " \\[ \\]"             "|"
            " \\[ " R_ARRAY " \\]" "|"
            /* empty */
        ") "
    );

    std::smatch m;

    if (!std::regex_match(line, m, reg) || m.size() != 4) {
        return false;
    }

    proto.type = m.str(1);
    proto.symbol = m.str(2);

    if (m.str(3).empty()) {
        proto.prototype = proto::object;
    } else {
        proto.prototype = proto::object_array;
        proto.type += m.str(3);
    }

    return true;
}

/* check for function or function pointer prototype */
bool is_function_or_function_pointer(const std::string &line, proto_t &proto)
{
    const std::regex reg(
        "^(" R_TYPE ") "

        "("
                       R_SYMBOL        "|"  /* function */
            "\\( \\* " R_SYMBOL " \\)" "|"  /* function pointer */
            "\\( "     R_SYMBOL " \\)"      /* function with parentheses */
        ") "

        "\\( (" R_PARAM ")\\) "
    );

    std::smatch m;

    if (!std::regex_match(line, m, reg) || m.size() != 4) {
        return false;
    }

    if (m.str(2).starts_with("( *")) {
        /* funtion pointer */
        proto.prototype = proto::function_pointer;
        proto.type = m.str(1) + " (*)(" + m.str(3) + ")";
        proto.symbol = m.str(2).substr(4, m.str(2).size() - 6);  /* "( * symbol )" */
    } else {
        /* function */
        proto.prototype = proto::function;
        proto.type = m.str(1);
        proto.args = m.str(3);

        if (m.str(2).starts_with('(')) {
            proto.symbol = m.str(2).substr(2, m.str(2).size() - 4);  /* "( symbol )" */
        } else {
            proto.symbol = m.str(2);
        }
    }

    return true;
}

/* save tokens into prototypes */
bool get_prototypes_from_tokens(const std::vector<vstring_t> &vec_tokens, vproto_t &vproto)
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
        std::string name = " a";

        proto.args.clear();
        proto.notype_args.clear();

        for (auto &e : vec) {
            if (name[1] > 'z') {
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
                    e.insert(pos + 3, name);
                } else {
                    /* object */
                    e += name;
                }

                proto.notype_args += name;
                name[1]++;
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
                    return "a parameter name is missing: " +
                        concat_function_prototype(proto) + ";\n"
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

/* format the text of the prototypes to make it look pretty */
void format_prototypes(std::string &s)
{
    utils::replace("* ", "*", s);
    utils::replace(" ,", ",", s);

    utils::replace("( ", "(", s);
    utils::replace(" )", ")", s);
    utils::replace(") (", ")(", s);

    utils::replace("[ ", "[", s);
    utils::replace("] ", "]", s);
    utils::replace(" [", "[", s);
    utils::replace(" ]", "]", s);

    utils::strip_spaces(s);
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
            auto it_beg = m_symbols.begin();
            auto it_end = m_symbols.end();

            for (const auto &e : vproto) {
                /* look for item e.symbol in list m_symbols */
                if (std::find(it_beg, it_end, e.symbol) != it_end) {
                    save_symbol(e);
                }
            }
        }
    }

    /* create typedefs for function pointers and arrays */
    for (auto &p : m_objects) {
        auto pos = std::string::npos;

        if (p.prototype == proto::function_pointer) {
            pos = p.type.find("(*)") + 2;
        } else if (p.prototype == proto::object_array) {
            pos = p.type.find('[');
        } else {
            continue;
        }

        if (pos != std::string::npos) {
            std::string new_type = m_name_lower + "_" + p.symbol + "_t";

            /* typedef */
            std::string tmp = p.type;
            tmp.insert(pos, new_type);
            m_typedefs.push_back(tmp);

            /* replace old type */
            p.type = new_type;
        }
    }

    /* cosmetics */

    for (auto &p : m_prototypes) {
        format_prototypes(p.type);
        format_prototypes(p.args);
    }

    for (auto &p : m_objects) {
        format_prototypes(p.type);
        format_prototypes(p.args);
    }

    for (auto &e : m_typedefs) {
        format_prototypes(e);
    }
}

/* parse `%option' strings */
void gendlopen::parse_options(const vstring_t &options)
{
    const char *p;

    for (const auto &e : options)
    {
        switch (e[0])
        {
        case 'f':
            if ( (p = get_arg(e, "format=")) != NULL ) {
                output::format out = utils::format_enum(p);

                if (out == output::error) {
                    throw error("unknown output format: " + std::string(p));
                }
                format(out);
                continue;
            }
            break;

        case 'n':
            if ( (p = get_arg(e, "name=")) != NULL ) {
                name(p);
                continue;
            }
            break;

        case 'l':
            if ( (p = get_arg(e, "library=")) != NULL ) {
                std::string lib_a, lib_w;
                utils::format_libname(p, lib_a, lib_w);
                default_lib(lib_a, lib_w);
                continue;
            }
            break;

        case 'i':
            if ( (p = get_arg(e, "include=")) != NULL ) {
                add_inc(utils::format_inc(p));
                continue;
            }
            break;

        case 'd':
            if ( (p = get_arg(e, "define=")) != NULL ) {
                add_def(utils::format_def(p));
                continue;
            }
            break;

        case 'p':
            if ( (p = get_arg(e, "param=")) != NULL ) {
                if (utils::eq_str_case(p, "skip")) {
                    parameter_names(param::skip);
                } else if (utils::eq_str_case(p, "create")) {
                    parameter_names(param::create);
                } else {
                    throw error("unknown argument for option 'param': " + std::string(p));
                }
                continue;
            }
            break;

        default:
            break;
        }

        throw error("unknown %option string: " + e);
    }
}

/* read input and tokenize */
void gendlopen::tokenize()
{
    std::vector<vstring_t> vec_tokens;
    vstring_t options;
    vstring_t *poptions = m_read_options ? &options : NULL;
    vproto_t vproto;

    /* open input file */

    if (m_input.empty()) {
        throw error("input file required");
    }

    if (m_input == "-" && m_custom_template == "-") {
        throw error("cannot read input file and custom template both from STDIN");
    }

    std::string file_or_stdin = (m_input == "-") ? "<STDIN>"
        : "file: " + m_input;

    open_file file(m_input);

    if (!file.is_open()) {
        throw error(file_or_stdin + "\nfailed to open file for reading");
    }

    /* read and tokenize input */
    int ret = tokenize_stream(file.file_pointer(), vec_tokens, poptions);

    /* input is a clang AST file */
    if (ret == MYLEX_AST_BEGIN) {
        clang_ast(file.file_pointer());
        return;
    }

    file.close();

    /* lexer error */
    if (ret == MYLEX_ERROR) {
        const char *p = mylex_lasterror();

        if (p) {
            file_or_stdin += '\n';
            file_or_stdin += p;
        }

        throw error(file_or_stdin);
    }

    /* parse `%options' strings */
    if (m_read_options) {
        parse_options(options);
    }

    /* get prototypes from tokens */
    if (!get_prototypes_from_tokens(vec_tokens, vproto)) {
        throw error(file_or_stdin + "\nfailed to read prototypes");
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
    if (m_parameter_names != param::skip) {
        for (auto &e : vproto) {
            if (e.prototype != proto::function ||
                e.args.empty() ||
                utils::eq_str_case(e.args, "void"))
            {
                /* not a function or a function without parameters */
                continue;
            }

            auto msg = read_parameter_names(e, m_parameter_names);

            if (!msg.empty()) {
                throw error(msg);
            }
        }
    }

    /* copy */
    filter_and_copy_symbols(vproto);
}
