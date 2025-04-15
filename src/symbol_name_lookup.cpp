/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2023-2025 Carsten Janssen

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

#include <algorithm>
#include <string>
#include "cio_ofstream.hpp"
#include "gendlopen.hpp"
#include "types.hpp"
#include "utils.hpp"


/**
 * Look for a common symbol prefix.
 * Many APIs share a common prefix among their symbols.
 * If you want to load a specific symbol we can use this
 * later for a faster lookup.
 */
static size_t common_prefix_length(vstring_t &vec)
{
    /* need at least 2 symbols */
    if (vec.size() < 2) {
        return 0;
    }

    size_t shortest_sym_len = vec.at(0).size();

    /* get shortest symbol length */
    for (const auto &e : vec) {
        /* prevent `min()' macro expansion from Windows headers */
        shortest_sym_len = std::min<size_t>(shortest_sym_len, e.size());
    }

    if (shortest_sym_len == 0) {
        return 0;
    }

    size_t pfxlen = 0;
    const char *ptr = vec.front().c_str();

    for (size_t i = 0; i < shortest_sym_len; i++, ptr++) {
        for (const auto &e : vec) {
            if (e.empty()) {
                return 0;
            }

            if (e.at(i) != *ptr) {
                /* common prefix found (can be empty) */
                return pfxlen;
            }
        }

        pfxlen++;
    }

    /* shortest symbol name is prefix, i.e. if a symbol `foo'
     * and `foobar' exist the prefix is `foo' */
    return pfxlen;
}


/* create a macro that will do a slightly optimized lookup of a
 * given symbol name */
size_t save::symbol_name_lookup(const std::string &pfx_upper, const vproto_t &v_prototypes, const vproto_t &v_objects)
{
    vstring_t symbols, temp;
    std::vector<vstring_t> lists;
    std::string str;

    /* copy symbol names */
    for (const auto &e : v_prototypes) {
        symbols.push_back(e.symbol);
    }

    for (const auto &e : v_objects) {
        symbols.push_back(e.symbol);
    }

    if (symbols.empty()) {
        /* should not happen */
        return 0;
    }

    /* GDO_CHECK_SYMBOL_NAME */
    str = "#define " + pfx_upper + "_CHECK_SYMBOL_NAME(SYM) \\\n";

    /* only 1 symbol */
    if (symbols.size() == 1) {
        str += "  if (SYM != NULL && strcmp(SYM, \"" + symbols.at(0) + "\") == 0) { \\\n"
               "    goto " + pfx_upper + "_JUMP_" + symbols.at(0) + "; \\\n"
               "  }\n";

        save::ofs << str;

        return utils::count_linefeed(str);
    }

    /* multiple symbols */

    const size_t pfxlen = common_prefix_length(symbols); /* can be 0 */
    const std::string pfx = symbols.at(0).substr(0, pfxlen); /* can be empty string */
    const std::string str_pfxlen = std::to_string(pfxlen);
    const std::string off = std::to_string(pfxlen + 1);

    /* prefix check */
    if (pfxlen == 0) {
        str += "  if (SYM != NULL) \\\n";
    } else if (pfxlen == 1) {
        str += "  if (SYM != NULL && *SYM == '" + pfx + "') \\\n";
    } else {
        str += "  if (SYM != NULL && strncmp(SYM, \"" + pfx + "\", " + str_pfxlen + ") == 0) \\\n";
    }

    /* copy symbol names into alphabetically sorted lists */

    std::sort(symbols.begin(), symbols.end());

    for (const auto &e : symbols) {
        if (e.empty()) {
            /* unlikely */
            continue;
        }

        if (!temp.empty() && temp.front().at(pfxlen) != e.at(pfxlen)) {
            lists.push_back(temp);
            temp.clear();
        }

        temp.push_back(e);
    }

    if (!temp.empty()) {
        lists.push_back(temp);
    }

    if (lists.empty()) {
        /* unlikely */
        return 0;
    }

    /* switch begin */
    str += "  { \\\n"
           "    switch (*(SYM+" + str_pfxlen + ")) \\\n"
           "    { \\\n";

    /* switch entries */
    for (const auto &v : lists) {
        const char c = utils::str_at(v.front(), pfxlen);

        if (c == 0) {
            str += "    case 0: /* same as common symbol prefix */ \\\n"
                   "      goto " + pfx_upper + "_JUMP_" + pfx + "; \\\n";
        } else {
            str += "    case '"; str+=c; str+="': \\\n";

            for (const auto &symbol : v) {
                const char *short_name = symbol.c_str() + pfxlen + 1;

                str += "      if (strcmp(SYM+" + off + ", \"" + short_name + "\") == 0) /*" + symbol + "*/ \\\n"
                       "        { goto " + pfx_upper + "_JUMP_" + symbol + "; } \\\n";
            }

            str += "      break; \\\n";
        }
    }

    /* switch end */
    str += "    default: \\\n"
           "      break; \\\n"
           "    } \\\n"
           "  }\n";

    save::ofs << str;

    return utils::count_linefeed(str);
}
