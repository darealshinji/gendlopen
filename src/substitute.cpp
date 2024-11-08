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
 * Substitute placeholders in embedded template data.
 */

#include <iostream>
#include <list>
#include <random>
#include <regex>
#include <vector>
#include <cstdlib>
#include "gendlopen.hpp"


namespace /* anonymous */
{
    enum {
        NO_PARAM_SKIP_FOUND,
        PARAM_SKIP_REMOVE_BEGIN,
        PARAM_SKIP_USE_BEGIN,
        PARAM_SKIP_END
    };

    /* check for a "%PARAM_SKIP_*%" line */
    int check_skip_keyword(const char *ptr)
    {
        const char str[] = "%PARAM_SKIP_";
        const size_t len = sizeof(str) - 1;

        if (strncmp(ptr, str, len) != 0) {
            return NO_PARAM_SKIP_FOUND;
        }

        ptr += len;

        if (strcmp(ptr, "REMOVE_BEGIN%") == 0) {
            return PARAM_SKIP_REMOVE_BEGIN;
        } else if (strcmp(ptr, "USE_BEGIN%") == 0) {
            return PARAM_SKIP_USE_BEGIN;
        } else if (strcmp(ptr, "END%") == 0) {
            return PARAM_SKIP_END;
        }

        return NO_PARAM_SKIP_FOUND;
    }

    std::string get_leading_newlines(const std::string &s)
    {
        size_t n = 0;

        for ( ; n < s.size(); n++) {
            if (s[n] != '\n') {
                break;
            }
        }

        return std::string(n, '\n');
    }

    std::string get_trailing_newlines(const std::string &s)
    {
        size_t n = 0;

        for (auto it = s.rbegin(); it != s.rend(); it++, n++) {
            if (*it != '\n') {
                break;
            }
        }

        return std::string(n, '\n');
    }

    /* loop and replace function prototypes, append to buffer */
    void replace_function_prototypes(vproto_t &vec, const std::string &line, cio::ofstream &ofs)
    {
        std::string copy;

        for (const auto &e : vec) {
            /* we can't handle variable argument lists in wrapper functions */
            if (e.args.ends_with("...") && line.find("%%return%%") != std::string::npos) {
                /* print same amount of trailing and leading newlines */
                ofs << get_leading_newlines(line);
                ofs << "/* can't handle variable argument lists in wrapper functions */\n";
                ofs << "// " << e.type << ' ' << e.symbol << '(' << e.args << ");\n";
                ofs << get_trailing_newlines(line);
                continue;
            }

            copy = line;

            /* don't "return" on "void" functions */
            if (utils::eq_str_case(e.type, "void")) {
                /* keep the indentation pretty */
                utils::erase("%%return%% ", copy);
                utils::erase("%%return%%", copy);
            } else {
                utils::replace("%%return%%", "return", copy);
            }

            if (e.type.back() == '*') {
                /* »char * x«  -->  »char *x« */
                utils::replace("%%type%% ", e.type, copy);
            }
            utils::replace("%%type%%", e.type, copy);
            utils::replace("%%func_symbol%%", e.symbol, copy);
            utils::replace("%%args%%", e.args, copy);
            utils::replace("%%notype_args%%", e.notype_args, copy);

            ofs << copy << '\n';
        }
    }

    void replace_symbol_names(vproto_t &proto, vproto_t &obj, const std::string &line, cio::ofstream &ofs)
    {
        std::string copy, type;

        /* function pointer */
        for (const auto &e : proto) {
            copy = line;

            /* %%sym_type%% --> »%%type%% (*)(%%args%%)« */
            type = e.type + " (*)(" + e.args + ")";
            utils::replace("%%sym_type%%", type, copy);
            utils::replace("%%symbol%%", e.symbol, copy);
            ofs << copy << '\n';
        }

        /* object pointer */
        for (const auto &e : obj) {
            copy = line;

            /* %%sym_type%% --> »%%obj_type%% *« */
            type = e.type + " *";
            utils::replace("%%sym_type%%", type, copy);
            utils::replace("%%symbol%%", e.symbol, copy);
            ofs << copy << '\n';
        }
    }
}

/* substitute placeholders */
void gendlopen::substitute(const cstrList_t &data, cio::ofstream &ofs)
{
    std::string fmt_upper, fmt_lower, fmt_namespace;
    bool skip_code = false;

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
        return;
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
        fmt_upper = "$1" + m_name_upper + '_';
        fmt_lower = "$1" + m_name_lower + '_';
        fmt_namespace = "$1" + m_name_lower + "::";
    }

    for (auto &e : data)
    {
        const char *line = e[0];

        for (int i = 0; line != NULL; i++, line = e[i])
        {
            /* empty line */
            if (line[0] == 0) {
                if (!skip_code) {
                    ofs << '\n';
                }
                continue;
            }

            /* optimize search for potential keywords */
            bool maybe_keyword = (strchr(line, '%') != NULL);

            if (maybe_keyword) {
                /* skip the whole line if it has the %DNL% (Do Not Lex) keyword */
                if (strstr(line, "%DNL%") != NULL) {
                    continue;
                }

                /* check if we have to comment out lines between
                * "%PARAM_SKIP_*_BEGIN%" and "%PARAM_SKIP_END%" */
                switch (check_skip_keyword(line))
                {
                case PARAM_SKIP_REMOVE_BEGIN:
                    skip_code = (m_parameter_names == param::skip);
                    continue;
                case PARAM_SKIP_USE_BEGIN:
                    skip_code = (m_parameter_names != param::skip);
                    continue;
                case PARAM_SKIP_END:
                    skip_code = false;
                    continue;
                default:
                    break;
                }
            }

            if (skip_code) {
                continue;
            }

            std::string buf = line;

            /* replace prefixes */
            if (custom_prefix) {
                buf = std::regex_replace(buf, reg_upper, fmt_upper);
                buf = std::regex_replace(buf, reg_lower, fmt_lower);
                buf = std::regex_replace(buf, reg_nmspc, fmt_namespace);
            }

            if (maybe_keyword) {
                /* insert common symbol prefix string */
                utils::replace("%COMMON_PREFIX%", m_common_prefix, buf);

                /* update value */
                maybe_keyword = (buf.find('%') != std::string::npos);
            }

            /* nothing to loop, just append */
            if (!maybe_keyword) {
                ofs << buf << '\n';
                continue;
            }

            /* check if the line needs to be processed in a loop */
            auto find_keyword = [&buf] (const list_t &list) -> int
            {
                for (const auto &e : list) {
                    if (buf.find(e) != std::string::npos) {
                        return 1;
                    }
                }
                return 0;
            };

            int has_func, has_obj, has_sym;

            if (buf.find("%%") == std::string::npos) {
                has_func = has_obj = has_sym = 0;
            } else {
                has_func = find_keyword(function_keywords);
                has_obj = find_keyword(object_keywords);
                has_sym = find_keyword(symbol_keywords);
            }

            if ((has_func + has_obj + has_sym) > 1) {
                /* error */
                throw error("cannot mix function, object and regular symbol"
                            " placeholders:\n" + std::string{line});
            } else if (has_func == 1) {
                /* function prototypes */
                if (m_prototypes.empty()) {
                    continue;
                }
                replace_function_prototypes(m_prototypes, buf, ofs);
            } else if (has_obj == 1) {
                /* object prototypes */
                if (m_objects.empty()) {
                    continue;
                }

                for (const auto &e : m_objects) {
                    std::string copy = buf;
                    utils::replace("%%obj_type%%", e.type, copy);
                    utils::replace("%%obj_symbol%%", e.symbol, copy);
                    ofs << copy << '\n';
                }
            } else if (has_sym == 1) {
                /* any symbol */
                replace_symbol_names(m_prototypes, m_objects, buf, ofs);
            } else {
                /* nothing to loop, just append */
                ofs << buf << '\n';
            }
        }
    }
}
