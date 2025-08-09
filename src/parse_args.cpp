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

#include <errno.h>
#include <string.h>
#include "parse_args.hpp"


/* prefix length */
int parse_args::pfxlen()
{
    if (m_pfxlen != -1) {
        /* prefix length was already set */
        return m_pfxlen;
    }

    /* initialize prefix length as 0 */
    m_pfxlen = 0;

    if (m_it >= m_argc) {
        return m_pfxlen;
    }

    const char *arg = m_argv[m_it];

    if (!arg) {
        return m_pfxlen;
    }

    switch (*arg)
    {
#ifdef _WIN32
    case '/':
        /* don't count string "/" as prefixed */
        if (strcmp(arg, "/") != 0) {
            m_pfxlen = 1;
        }
        break;
#endif

    case '-':
        /* don't count strings "-" and "--" as prefixed */
        if (strcmp(arg, "-") != 0 && strcmp(arg, "--") != 0) {
            m_pfxlen = (strncmp(arg, "--", 2) == 0) ? 2 : 1;
        }
        break;

    default:
        break;
    }

    return m_pfxlen;
}

/* return prefix */
std::string parse_args::prefix()
{
    std::string pfx;
    const char *arg = m_argv[m_it];

    if (pfxlen() < 1 || !arg) {
        return {};
    }

    for (int i = 0; i < pfxlen(); i++) {
        pfx.push_back(arg[i]);
    }

    return pfx;
}

/* get first argument (argv[1]) */
const char *parse_args::begin()
{
    /* reset */
    m_pfxlen = -1;
    m_it = 1;

    return (m_argc > 1) ? m_argv[1] : NULL;
}

/* iterate to next item and return pointer to it */
const char *parse_args::next()
{
    m_pfxlen = -1; /* reset */

    if (m_it < m_argc) {
        m_it++;
    }

    return (m_it < m_argc) ? m_argv[m_it] : NULL;
}

/* get argument from an option string */
bool parse_args::get_arg(const char *str, size_t len)
{
    std::string msg;
    const char *cur = m_argv[m_it] + pfxlen(); /* skip prefix */

    m_opt = NULL;

    if (strncmp(cur, str, len) != 0) {
        /* not the argument we're looking for */
        return false;
    }

    msg = "option requires an argument: ";
    msg += m_argv[m_it];

    /* "-foo bar" --> get next item */
    if (strcmp(cur, str) == 0) {
        m_opt = next();

        if (!m_opt || *m_opt == 0) {
            throw error(msg);
        }

        return true;
    }

    /* -foo=bar, -Dfoo --> get substring */
    if (strlen(cur) > len) {
        if (len == 1) {
            /* -Dfoo */
            m_opt = cur + 1;
        } else if (cur[len] == '=') {
            /* -foo=bar */
            m_opt = cur + len + 1;
        } else {
            /* could be -foobar instead of -foo */
            return false;
        }

        if (!m_opt || *m_opt == 0) {
            throw error(msg);
        }

        return true;
    }

    return false;
}

/* option without argument */
bool parse_args::get_noarg(const char *str, size_t len)
{
    const char *cur = m_argv[m_it] + pfxlen(); /* skip prefix */

    m_opt = NULL;

    /* -foo */
    if (strcmp(cur, str) == 0) {
        return true;
    }

    /* -foo=bar */
    if (strncmp(cur, str, len) == 0 && cur[len] == '=') {
        std::string msg = "option does not take an argument: ";
        msg += prefix();
        msg += str;
        throw error(msg);
    }

    return false;
}
