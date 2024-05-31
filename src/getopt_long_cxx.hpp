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

#ifndef GETOPT_LONG_CXX_HPP
#define GETOPT_LONG_CXX_HPP

#include <stdexcept>
#include <list>
#include <string>
#include <getopt.h>


/**
 * Convenience wrapper class around getopt_long()
 *
 * - create struct option array and optstring
 * - print formatted help output
 * - provide "help" switch
 * - check given option list for empty strings
 * - treat trailing non-option arguments as error (we want that)
 */
class getopt_long_cxx
{
public:

    typedef struct {
        const char *val_long;
        char val_short;
        const char *val_arg;
        const char *help;
        const char *more_help;
    } arg_t;

    class error : public std::runtime_error
    {
        public:
            error(const std::string &message) : std::runtime_error(message) {}
            virtual ~error() {}
    };

private:

    int m_argc;
    char **m_argv;
    const std::list<arg_t> *m_args;

    struct option *m_longopts = nullptr;
    std::string m_optstring;
    int m_longindex = 0;

    /* create option table and optstring */
    void init_longopts();

    /* print formatted full help of argument "arg" */
    void print_arg_full_help(const arg_t &arg);

public:

    /* c'tor */
    getopt_long_cxx(int &argc, char **&argv, const std::list<arg_t> &args);

    /* d'tor */
    ~getopt_long_cxx();

    /* initialize options for parsing;
     * will also be called by getopt() but manually calling it might
     * be desired to catch and exception */
    void init();

    /* call getopt_long() and save return value */
    bool getopt(int &c);

    /* return optarg */
    const char *arg() const;

    void print_help();
    void print_full_help();

    /* parse "help <opt>" switch */
    bool parse_help_switch();
};

#endif /* GETOPT_LONG_CXX_HPP */
