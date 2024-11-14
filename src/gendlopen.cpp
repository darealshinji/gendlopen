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

#include "global.hpp"


/* c'tor */
gendlopen::gendlopen()
{}

/* d'tor */
gendlopen::~gendlopen()
{}

/* set symbol prefix name */
void gendlopen::name(const std::string &s)
{
    /* set name */
    m_name = s;

    /* set uppercase/lowercase name */
    m_name_upper = utils::convert_to_upper(m_name);
    m_name_lower = utils::convert_to_lower(m_name);

    /* set regex format string */
    m_fmt_upper = "$1" + m_name_upper + '_';
    m_fmt_lower = "$1" + m_name_lower + '_';
    m_fmt_namespace = "$1" + m_name_lower + "::";
}

/* set default library to load */
void gendlopen::default_lib(const std::string &lib_a, const std::string &lib_w)
{
    assert(!lib_a.empty() && !lib_w.empty());
    m_deflib_a = lib_a;
    m_deflib_w = lib_w;
}

