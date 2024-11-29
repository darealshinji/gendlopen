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

#include <iostream>
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
const char *parse_args::get_arg_len(const char *str, const size_t &len, const int &argc, char ** const &argv, int &it)
{
    auto err_noarg = [&] () {
        std::cerr << get_prog_name(argv[0]) << ": option requires an argument: "
            << argv[it] << std::endl;
        std::exit(1);
    };

    const char *cur = argv[it] + pfxlen;

    if (strncmp(cur, str, len) != 0) {
        /* not the argument we're looking for */
        return NULL;
    }

    /* "-foo bar" --> get next item */
    if (strcmp(cur, str) == 0) {
        int next = it + 1;

        if (next >= argc || !argv[next] || !*argv[next]) {
            err_noarg();
        }

        return argv[++it];
    }

    /* -foo=bar, -Dfoo --> get substring */
    if (strlen(cur) > len) {
        const char *opt = NULL;

        if (len == 1) {
            /* -Dfoo */
            opt = cur + 1;
        } else if (cur[len] == '=') {
            /* -foo=bar */
            opt = cur + len + 1;
        }

        if (!opt || *opt == 0) {
            err_noarg();
        }

        return opt;
    }

    return NULL;
}

/* option without argument */
bool parse_args::get_noarg_len(const char *str, const size_t &len, char ** const &argv, const int &it)
{
    const char *cur = argv[it] + pfxlen;

    /* -foo */
    if (strcmp(cur, str) == 0) {
        return true;
    }

    /* -foo=bar */
    if (strncmp(cur, str, len) == 0 && cur[len] == '=') {
        std::cerr << get_prog_name(argv[0]) << ": option does not take an argument: "
            << *argv[it] << str << std::endl;
        std::exit(1);
    }

    return false;
}
