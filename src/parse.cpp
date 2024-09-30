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
 * Parse the embedded template data and substitute the placeholders.
 */

#include <iostream>
#include <list>
#include <random>
#include <regex>
#include <cstdlib>
#include "gendlopen.hpp"


namespace /* anonymous */
{
    enum {
        NO_PARAM_SKIP_FOUND,
        PARAM_SKIP_COMMENT_OUT_BEGIN,
        PARAM_SKIP_USE_BEGIN,
        PARAM_SKIP_END
    };

    using list_t = std::list<const char *>;

    /* check for a "%PARAM_SKIP_*%" line */
    int check_skip_keyword(const std::string &line)
    {
        const std::regex reg(R"(^\s*?%PARAM_SKIP_(COMMENT_OUT_BEGIN|USE_BEGIN|END)%\s*?$)");
        std::smatch m;

        if (!std::regex_match(line, m, reg) || m.size() != 2) {
            return NO_PARAM_SKIP_FOUND;
        }

        if (m[1] == "COMMENT_OUT_BEGIN") {
            return PARAM_SKIP_COMMENT_OUT_BEGIN;
        } else if (m[1] == "USE_BEGIN") {
            return PARAM_SKIP_USE_BEGIN;
        } else if (m[1] == "END") {
            return PARAM_SKIP_END;
        }

        return NO_PARAM_SKIP_FOUND;
    }

    /* comment out lines in code */
    void comment_out_code(std::string &buffer, const std::string &code)
    {
        std::string line;

        auto append_to_buffer = [&buffer] (const std::string &str) {
            /* don't comment out if line has only ASCII spaces */
            if (str.find_first_not_of(" \t\n\r\v\f") != std::string::npos) {
                buffer += "//";
            }
            buffer += str;
        };

        for (const char &c : code) {
            line += c;

            if (c != '\n') {
                continue;
            }

            append_to_buffer(line);
            line.clear();
        }

        if (!line.empty()) {
            append_to_buffer(line);
        }
    }

    /* loop and replace function prototypes, append to buffer */
    void replace_function_prototypes(vproto_t &vec, const std::string &line, std::string &buffer)
    {
        std::string copy;

        for (const auto &e : vec) {
            bool comment_out = false;
            copy = line;

            /* we can't handle variable argument lists in wrapper functions */
            if (e.args.ends_with("...") && copy.find("%%return%%") != std::string::npos) {
                comment_out = true;
            }

            /* don't "return" on "void" functions */
            if (utils::eq_str_case(e.type, "void")) {
                /* keep the indentation pretty */
                utils::replace("%%return%% ", "", copy);
                utils::replace("%%return%%", "", copy);
            } else {
                utils::replace("%%return%%", "return", copy);
            }

            if (e.type.ends_with("*")) {
                /* »char * x«  -->  »char *x« */
                utils::replace("%%type%% ", e.type, copy);
            }
            utils::replace("%%type%%", e.type, copy);
            utils::replace("%%func_symbol%%", e.symbol, copy);
            utils::replace("%%args%%", e.args, copy);
            utils::replace("%%notype_args%%", e.notype_args, copy);

            if (comment_out) {
                comment_out_code(buffer, copy);
            } else {
                buffer += copy;
            }
        }
    }

    void replace_symbol_names(vproto_t &proto, vproto_t &obj, const std::string &line, std::string &buffer)
    {
        std::string copy, type;

        /* function pointer */
        for (const auto &e : proto) {
            copy = line;

            /* %%sym_type%% --> »%%type%% (*)(%%args%%)« */
            type = e.type + " (*)(" + e.args + ")";
            utils::replace("%%sym_type%%", type, copy);
            utils::replace("%%symbol%%", e.symbol, copy);
            buffer += copy;
        }

        /* object pointer */
        for (const auto &e : obj) {
            copy = line;

            /* %%sym_type%% --> »%%obj_type%% *« */
            type = e.type + " *";
            utils::replace("%%sym_type%%", type, copy);
            utils::replace("%%symbol%%", e.symbol, copy);
            buffer += copy;
        }
    }
}

/* parse the template data */
std::string gendlopen::parse(std::string &data)
{
    std::string buf, line;
    std::string fmt_upper, fmt_lower, fmt_namespace;
    bool comment_out = false;

    const list_t function_keywords = {
        "%%return%%",
        "%%type%%",
        "%%func_symbol%%",
        "%%args%%",
        "%%notype_args%%"
    };

    const list_t object_keywords = {
        "%%obj_type%%",
        "%%obj_symbol%%"
    };

    const list_t symbol_keywords = {
        "%%sym_type%%",
        "%%symbol%%"
    };

    if (data.empty()) {
        return {};
    }

    if (m_prototypes.empty() && m_objects.empty()) {
        throw error("no function or object prototypes");
    }

    /* regex for prefixes: [_]GDO_ / [_]gdo_ / gdo:: */
    const std::regex reg_upper("([^A-Za-z0-9_]?[_]?)(GDO_)");
    const std::regex reg_lower("([^A-Za-z0-9_]?[_]?)(gdo_)");
    const std::regex reg_nmspc("([^A-Za-z0-9_]?)(gdo::)");

    const bool custom_prefix = (m_name_upper != "GDO");

    if (custom_prefix) {
        fmt_upper = "$1" + (m_name_upper + '_');
        fmt_lower = "$1" + (m_name_lower + '_');
        fmt_namespace = "$1" + (m_name_lower + "::");
    }

    /* Change all line endings from \r\n to \n.
     * This is a "radical" solution but it makes parsing the code
     * a lot easier. */
    utils::replace("\r\n", "\n", data);

    /* read data character by character */
    for (const char *p = data.c_str(); *p != 0; p++)
    {
        /* will be NUL if EOL is reached */
        const char next = *(p+1);

        /* treat lines ending on '@' like single lines
         * but keep the '\n' character */
        if (*p == '@' && next == '\n') {
            p++;

            if (comment_out) {
                /* keep trailing `@' symbol in commented out section
                 * and continue to parse the line */
                line += '@';
            } else {
                line += '\n';
                continue;
            }
        }

        line += *p;

        /* end of line or data not yet reached */
        if (*p != '\n' && next != 0) {
            continue;
        }

        /* optimize search for potential keywords */
        bool maybe_keyword = (line.find('%') != std::string::npos);

        if (maybe_keyword) {
            /* skip the whole line if it has the %DNL% (Do Not Lex) keyword */
            if (line.find("%DNL%") != std::string::npos) {
                line.clear();
                continue;
            }

            /* check if we have to comment out lines between
             * "%PARAM_SKIP_*_BEGIN%" and "%PARAM_SKIP_END%" */
            switch (check_skip_keyword(line))
            {
            case PARAM_SKIP_COMMENT_OUT_BEGIN:
                comment_out = (m_parameter_names == param::skip);
                line.clear();
                continue;

            case PARAM_SKIP_USE_BEGIN:
                comment_out = (m_parameter_names != param::skip);
                line.clear();
                continue;

            case PARAM_SKIP_END:
                comment_out = false;
                line.clear();
                continue;

            default:
                break;
            }
        }

        /* replace prefixes */
        if (custom_prefix) {
            line = std::regex_replace(line, reg_upper, fmt_upper);
            line = std::regex_replace(line, reg_lower, fmt_lower);
            line = std::regex_replace(line, reg_nmspc, fmt_namespace);
        }

        if (comment_out) {
            buf += "//" + line;
            line.clear();
            continue;
        }

        if (maybe_keyword) {
            /* insert common symbol prefix string */
            utils::replace("%COMMON_PREFIX%", m_common_prefix, line);

            /* update value */
            maybe_keyword = line.find('%') != std::string::npos;
        }

        /* nothing to loop, just append */
        if (!maybe_keyword) {
            buf += line;
            line.clear();
            continue;
        }

        /* check if the line needs to be processed in a loop */
        auto find_keyword = [&line] (const list_t &list) -> int
        {
            for (const auto &e : list) {
                if (line.find(e) != std::string::npos) {
                    return 1;
                }
            }
            return 0;
        };

        int has_func, has_obj, has_sym;

        if (line.find("%%") == std::string::npos) {
            has_func = has_obj = has_sym = 0;
        } else {
            has_func = find_keyword(function_keywords);
            has_obj = find_keyword(object_keywords);
            has_sym = find_keyword(symbol_keywords);
        }

        if ((has_func + has_obj + has_sym) > 1) {
            /* error */
            std::string msg = "cannot mix function, object and regular symbol placeholders:\n";
            msg += line;
            throw error(msg);
        } else if (has_func == 1) {
            /* function prototypes */

            if (m_prototypes.empty()) {
                line.clear();
                continue;
            }

            replace_function_prototypes(m_prototypes, line, buf);
        } else if (has_obj == 1) {
            /* object prototypes */

            if (m_objects.empty()) {
                line.clear();
                continue;
            }

            for (const auto &e : m_objects) {
                std::string copy = line;
                utils::replace("%%obj_type%%", e.type, copy);
                utils::replace("%%obj_symbol%%", e.symbol, copy);
                buf += copy;
            }
        } else if (has_sym == 1) {
            /* any symbol */
            replace_symbol_names(m_prototypes, m_objects, line, buf);
        } else {
            /* nothing to loop, just append */
            buf += line;
        }

        line.clear();
    }

    return buf;
}
