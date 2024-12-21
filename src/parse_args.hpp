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

#ifndef PARSE_ARGS_HPP
#define PARSE_ARGS_HPP

#include <stdexcept>
#include <string>


class parse_args
{
public:

    class error : public std::runtime_error
    {
        public:
            error(const std::string &message) : std::runtime_error(message) {}
            virtual ~error() {}
    };

private:

    int m_argc;
    char ** const m_argv;
    int m_it = 1; /* skip argv[0] */
    const char *m_opt = NULL;

public:

    parse_args(const int &argc, char ** const &argv);
    ~parse_args();

    const char *current() const;
    const char *next();
    const char *opt() const;
    bool has_prefix() const;

    bool get_arg(const char *str, const size_t &len);
    bool get_noarg(const char *str, const size_t &len);

    template<size_t N>
    constexpr bool get_arg(char const (&str)[N]) {
        return get_arg(str, N-1);
    }

    template<size_t N>
    constexpr bool get_noarg(char const (&str)[N]) {
        return get_noarg(str, N-1);
    }

    static const char *get_prog_name(const char *prog);
};

#endif /* PARSE_ARGS_HPP */
