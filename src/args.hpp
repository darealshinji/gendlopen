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

#ifndef GDO_ARGS_HPP
#define GDO_ARGS_HPP


namespace args
{
    typedef struct {
        const char *long_opt;
        char short_opt;
        const char *help;
        const char *more_help;
    } arg_t;


    /* parse "help <cmd>" switch */
    int parse_help_switch(int &argc, char **&argv, const std::vector<arg_t> &help_args);

    /* vectorize arguments and check for --help */
    vstring_t parse_args(int &argc, char **&argv, const std::vector<arg_t> &help_args);

    /* find argument with value */
    std::string get_value(vstring_t &vec, const arg_t &arg);

    /* find argument(s) with value and add to list */
    vstring_t get_value_list(vstring_t &vec, const arg_t &arg);

    /* find boolean flag */
    bool get_flag(vstring_t &vec, const arg_t &arg);
}

#endif /* GDO_ARGS_HPP */
