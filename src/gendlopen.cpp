/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Carsten Janssen
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
#include <string>
#include <vector>

#include "global.hpp"


namespace /* anonymous */
{
    /* 'void (*)()'  ==>  'void (*name)()' */
    std::string fptr_typedef(const std::string &type, const std::string &name)
    {
        auto pos = type.find("(*)");

        if (pos == std::string::npos || type.find('(') != pos) {
            return {};
        }

        std::string s = type;
        pos += 2; /* insert after '*' */

        return s.insert(pos, name);
    }

    /* 'char[32]'  ==>  'char name[32]' */
    std::string array_typedef(std::string &type, const std::string &name)
    {
        auto pos = type.find('[');

        if (pos == std::string::npos) {
            return {};
        }

        std::string s = type;

        /* insert space if needed */
        if (pos > 0 && type.at(pos-1) != ' ') {
            s.insert(pos, 1, ' ');
            pos++;
        }

        return s.insert(pos, name);
    }

} /* end anonymous namespace */


/* c'tor */
gendlopen::gendlopen()
{}


/* d'tor */
gendlopen::~gendlopen()
{}


/* set symbol prefix name */
void gendlopen::prefix(const std::string &s)
{
    /* set name */
    m_pfx = s;

    /* set uppercase/lowercase name */
    m_pfx_upper = utils::convert_to_upper(m_pfx);
    m_pfx_lower = utils::convert_to_lower(m_pfx);

    /* set regex format string (used in substitute.cpp) */
    m_fmt_upper = "$1" + m_pfx_upper + '_';
    m_fmt_lower = "$1" + m_pfx_lower + '_';
    m_fmt_namespace = "$1" + m_pfx_lower + "::";
}


/* set default library to load */
void gendlopen::default_lib(const std::string &lib_a, const std::string &lib_w)
{
    assert(!lib_a.empty() && !lib_w.empty());
    m_deflib_a = lib_a;
    m_deflib_w = lib_w;
}


/**
 * Look for a common symbol prefix.
 * Many APIs share a common prefix among their symbols.
 * If you want to load a specific symbol we can use this
 * later for a faster lookup.
 */
void gendlopen::get_common_prefix()
{
    std::string *symbol0, pfx;

    m_common_prefix.clear();

    /* need at least 2 symbols */
    if ((m_prototypes.size() + m_objects.size()) < 2) {
        return;
    }

    /* get first symbol */
    if (!m_prototypes.empty()) {
        symbol0 = &m_prototypes.at(0).symbol;
    } else {
        symbol0 = &m_objects.at(0).symbol;
    }

    /* get shortest symbol length */
    size_t len = symbol0->size();

    for (const auto &v : {m_prototypes, m_objects}) {
        for (const auto &e : v) {
            /* prevent `min()' macro expansion from Windows headers
             * https://stackoverflow.com/a/30924806/5687704 */
            len = std::min<size_t>(len, e.symbol.size());
        }
    }

    /* compare symbol names */
    for (size_t i = 0; i < len; i++) {
        const char c = symbol0->at(i);

        for (const auto &v : {m_prototypes, m_objects}) {
            for (const auto &e : v) {
                if (e.symbol.at(i) != c) {
                    /* common prefix found (can be empty) */
                    m_common_prefix = pfx;
                    return;
                }
            }
        }

        pfx.push_back(c);
    }

    /* shortest symbol name is prefix, i.e. if a symbol `foo'
     * and `foobar' exist the prefix is `foo' */
    m_common_prefix = pfx;
}


/* create typedefs for function pointers and arrays */
void gendlopen::create_typedefs()
{
    /* create typename */
    auto mk_name = [this] (const std::string &symbol) {
        return m_pfx_lower + '_' + symbol + "_t";
    };

    for (auto &p : m_objects) {
        std::string def, name;

        if (p.prototype == proto::function_pointer) {
            /* function pointer */
            name = mk_name(p.symbol);
            def = fptr_typedef(p.type, name);
        } else if (p.prototype == proto::object_array) {
            /* array type */
            name = mk_name(p.symbol);
            def = array_typedef(p.type, name);
        } else {
            continue;
        }

        if (!def.empty()) {
            m_typedefs.push_back(def); /* add to typedefs */
            p.type = name; /* replace old type */
        }
    }
}
