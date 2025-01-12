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

#if defined(__linux__)
# ifndef HAVE_PROGRAM_INVOCATION_NAME
#  define HAVE_PROGRAM_INVOCATION_NAME
# endif
#elif defined(__FreeBSD__) || defined(__NetBSD__) || defined(__OpenBSD__) || \
    defined(__DragonFly__) || defined(__APPLE__)
# ifndef HAVE_GETPROGNAME
#  define HAVE_GETPROGNAME
# endif
#endif


parse_args::parse_args(const int &argc, char ** const &argv)
: m_argc(argc), m_argv(argv)
{}

parse_args::~parse_args()
{}

/* get current argument */
const char *parse_args::current() const {
    return (m_it < m_argc) ? m_argv[m_it] : NULL;
}

/* return option string (may be NULL) */
const char *parse_args::opt() const {
    return m_opt;
}

/* whether argument has a prefix */
bool parse_args::has_prefix() const
{
    if (m_it >= m_argc) {
        return false;
    }

    char c = m_argv[m_it][0];

#ifdef _WIN32
    return (c == '-' || c == '/');
#else
    return (c == '-');
#endif
}

/* iterate to next item and return pointer to it */
const char *parse_args::next()
{
    if (m_it < m_argc) {
        m_it++;
    }

    return current();
}

/* get program name without full path */
const char *parse_args::get_prog_name(const char *prog)
{
#ifdef HAVE_PROGRAM_INVOCATION_NAME
    prog = program_invocation_short_name;
#elif defined(HAVE_GETPROGNAME)
    prog = getprogname();
#else
    auto is_path_separator = [] (const char &c) -> bool
    {
# ifdef _WIN32
        return (c == '\\' || c == '/');
# else
        return (c == '/');
# endif
    };

    for (auto p = prog; *p != 0; p++) {
        if (is_path_separator(*p) && *(p+1) != 0) {
            prog = p + 1;
        }
    }
#endif

    return prog;
}

/* get argument from an option string */
bool parse_args::get_arg(const char *str, const size_t &len)
{
    std::string msg;
    const char *cur = m_argv[m_it] + 1; /* skip prefix */

    m_opt = NULL;

    if (strncmp(cur, str, len) != 0) {
        /* not the argument we're looking for */
        return false;
    }

    msg = "option requires an argument: ";
    msg += m_argv[m_it];

    /* "-foo bar" --> get next item */
    if (strcmp(cur, str) == 0) {
        int next = m_it + 1;

        if (next >= m_argc || !m_argv[next] || !*m_argv[next]) {
            throw error(msg);
        }

        m_opt = m_argv[++m_it];
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
bool parse_args::get_noarg(const char *str, const size_t &len)
{
    const char *cur = m_argv[m_it] + 1; /* skip prefix */

    m_opt = NULL;

    /* -foo */
    if (strcmp(cur, str) == 0) {
        return true;
    }

    /* -foo=bar */
    if (strncmp(cur, str, len) == 0 && cur[len] == '=') {
        std::string msg = "option does not take an argument: ";
        msg += *m_argv[m_it];
        msg += str;
        throw error(msg);
    }

    return false;
}
