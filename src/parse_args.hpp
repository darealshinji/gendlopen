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

/* Small helper to parse arguments. */

#ifndef PARSE_ARGS_HPP
#define PARSE_ARGS_HPP

/* hold infos to parse command line arguments */
struct parse_args
{
    int         argc = 0;     /* argument count */
    char      **argv = NULL;  /* argument vector */
    int         it   = 0;     /* current argc iterator */
    const char *cur  = NULL;  /* current argv[] element */
    const char *opt  = NULL;  /* current argument's option */

    static const char *get_prog_name(const char *prog);
    bool get_arg_len(const char *str, const size_t len);
    bool get_noarg_len(const char *str, const size_t len);

    template<size_t N>
    constexpr bool get_arg(char const (&str)[N])
    {
        return get_arg_len(str, N-1);
    }

    template<size_t N>
    constexpr bool get_noarg(char const (&str)[N])
    {
        return get_noarg_len(str, N-1);
    }
};

#endif /* PARSE_ARGS_HPP */
