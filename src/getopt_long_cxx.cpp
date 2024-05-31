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

#include <stdexcept>
#include <iostream>
#include <list>
#include <sstream>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt_long_cxx.hpp"


/* c'tor */
getopt_long_cxx::getopt_long_cxx(int &argc, char **&argv, const std::list<arg_t> &args)
    : m_argc(argc), m_argv(argv), m_args(&args)
{
}


/* d'tor */
getopt_long_cxx::~getopt_long_cxx()
{
    if (m_longopts) {
        delete[] m_longopts;
    }
}


/* initialize options for parsing with getopt_long() */
void getopt_long_cxx::init()
{
    auto throw_if_empty = [] (const char *val, const std::string &msg) {
        if (!val) {
            throw error(msg + " == NULL");
        } else if (*val == 0) {
            throw error(msg + " is empty string");
        }
    };

    /* already initialized? */
    if (m_longopts) {
        return;
    }

    /* check options */
    for (const auto &opt : *m_args) {
        throw_if_empty(opt.val_long, "val_long");
        throw_if_empty(opt.help, "help");
        throw_if_empty(opt.more_help, "more_help");

        if (opt.val_short == 0) {
            throw error("val_short == 0");
        }
    }

    /* create option table and optstring */
    auto ptr = m_longopts = new struct option[m_args->size() + 1];

    for (const auto &opt : *m_args) {
        int has_arg = (opt.val_arg && *opt.val_arg) ?
            required_argument : no_argument;

        *ptr++ = { opt.val_long, has_arg, NULL, opt.val_short };

        /* append short option to optstring */
        m_optstring.push_back(opt.val_short);

        if (has_arg == required_argument) {
            m_optstring.push_back(':');
        }
    }

    *ptr = { NULL, 0, NULL, 0 };
}


/* call getopt_long() and save return value */
bool getopt_long_cxx::getopt(int &c)
{
    init();

    /* returns -1 if there are no more option characters */
    c = getopt_long(m_argc, m_argv, m_optstring.c_str(), m_longopts, &m_longindex);

    if (c != -1) {
        /* not done yet */
        return true;
    }

    /* extra elements */
    if (optind < m_argc) {
        std::cerr << m_argv[0] << ": non-option elements:";

        while (optind < m_argc) {
            std::cerr << ' ' << m_argv[optind++];
        }
        std::cerr << std::endl;

        /* treat extra elements as an error */
        c = '?';

        return true;
    }

    /* finished */
    return false;
}


/* return optarg */
const char *getopt_long_cxx::arg() const {
    return optarg;
}


/* print help with automatic indentation and line breaks */
void getopt_long_cxx::print_help()
{
    for (const auto &arg : *m_args) {
        /* options */
        std::string line;

        if (arg.val_short > ' ') {
            /* short option is a printable character */
            line = "  -";
            line += arg.val_short;
        } else {
            line = "    ";
        }

        line += " --";
        line += arg.val_long;

        if (arg.val_arg && *arg.val_arg) {
            line += '=';
            line += arg.val_arg;
            line += " ";
        }

        /* append spaces */
        while (line.size() < 24) {
            line += ' ';
        }

        /* split lines and print with indentation of 25 spaces */
        std::istringstream iss(arg.help);
        std::string token;

        while (iss >> token) {
            if (line.size() + token.size() + 1 >= 80) {
                std::cout << line << std::endl;
                line.clear();
                line.append(24, ' ');
            }

            line += ' ';
            line += token;
        }

        std::cout << line << std::endl;
    }

    std::cout << std::endl;
}


/* print full help with automatic indentation and line breaks */
void getopt_long_cxx::print_full_help()
{
    for (const auto &arg : *m_args) {
        print_arg_full_help(arg);
        std::cout << std::endl;
    }
}


/* print formatted full help of argument "arg" */
void getopt_long_cxx::print_arg_full_help(const arg_t &arg)
{
    /* print first line with options */
    std::string line = "\n  ";

    if (arg.val_short > ' ') {
        /* short option is a printable character */
        line += '-';
        line += arg.val_short;

        if (arg.val_arg && *arg.val_arg) {
            line += ' ';
            line += arg.val_arg;
        }
        line += ", --";
    } else {
        line += "--";
    }

    line += arg.val_long;

    if (arg.val_arg && *arg.val_arg) {
        line += '=';
        line += arg.val_arg;
    }

    std::cout << line << "\n\n";
    line.clear();

    /* print help, split long lines, use indentation of 4 spaces */
    for (const char *p = arg.more_help; *p != 0; p++) {
        if (*p != '\n') {
            line += *p;

            if (*(p+1) != 0) {
                continue;
            }
        }

        if (line.empty()) {
            std::cout << std::endl;
        } else if (line.size() <= 76) {
            std::cout << "    " << line << std::endl;
            line.clear();
        } else {
            size_t pos = 0;

            while (line.size() > 76 && (pos = line.rfind(' ', 76)) != std::string::npos) {
                std::cout << "    " << line.substr(0, pos) << std::endl;
                line.erase(0, pos+1);
            }

            if (!line.empty()) {
                std::cout << "    " << line << std::endl;
                line.clear();
            }
        }
    }

    if (!line.empty()) {
        std::cout << "    " << line << std::endl;
    }

    std::cout << std::endl;
}


/* parse "help <opt>" switch */
bool getopt_long_cxx::parse_help_switch()
{
    if (m_argc < 2 || strcmp(m_argv[1], "help") != 0) {
        /* no "help" switch given */
        return false;
    }

    if (m_argc < 3) {
        throw error("switch 'help' expected an option");
    }

    const char *opt = m_argv[2];
    const size_t len = strlen(opt);
    char short_opt = 0;
    const char *long_opt = NULL;

    if (len == 1) {
        /* i */
        short_opt = *opt;
    } else if (len == 2 && *opt == '-') {
        /* -i */
        short_opt = opt[1];
    } else if (len > 2 && *opt == '-') {
        if (opt[1] == '-') {
            /* --input */
            long_opt = opt + 2;
        } else {
            /* -iFILE */
            short_opt = opt[1];
        }
    } else {
        /* input */
        long_opt = opt;
    }

    if (short_opt) {
        /* searching for short opt */
        for (const auto &e : *m_args) {
            if (e.val_short == short_opt) {
                print_arg_full_help(e);
                return true;
            }
        }
    } else if (long_opt) {
        /* if an argument was given remove it */
        std::string s = long_opt;
        size_t pos = s.find('=');

        if (pos != std::string::npos) {
            s.erase(pos);
        }

        /* searching for long opt */
        for (const auto &e : *m_args) {
            if (e.val_long == s) {
                print_arg_full_help(e);
                return true;
            }
        }
    }

    std::string msg = "unknown option: ";
    msg += opt;
    throw error(msg);
}
