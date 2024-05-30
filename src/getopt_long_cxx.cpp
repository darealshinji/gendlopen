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
#include <sstream>
#include <vector>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt_long_cxx.hpp"


/* create option table and optstring */
void getopt_long_cxx::init_longopts()
{
    if (m_longopts) {
        /* already initialized */
        return;
    }

    m_longopts = new struct option[m_args.size() + 1];
    m_optstring.clear();

    for (size_t i = 0; i < m_args.size(); i++) {
        const auto &e = m_args[i];

        int has_arg = (e.val_arg && *e.val_arg) ? required_argument : no_argument;
        m_longopts[i] = { e.val_long, has_arg, NULL, e.val_short };

        /* append short option to optstring */
        m_optstring.push_back(e.val_short);

        if (has_arg == required_argument) {
            m_optstring.push_back(':');
        }
    }

    m_longopts[m_args.size()] = { NULL, 0, NULL, 0 };
}


/* c'tor */
getopt_long_cxx::getopt_long_cxx(int &argc, char **&argv)
: m_argc(argc), m_argv(argv)
{
}


/* d'tor */
getopt_long_cxx::~getopt_long_cxx()
{
    if (m_longopts) {
        delete[] m_longopts;
    }
}


/* add option */
void getopt_long_cxx::add(const char *val_long, char val_short, const char *val_arg,
                          const char *help, const char *more_help)
{
    auto throw_if_empty = [] (const char *val, const std::string &msg) {
        if (!val) {
            throw error(msg + " == NULL");
        } else if (*val == 0) {
            throw error(msg + " is empty string");
        }
    };

    throw_if_empty(val_long, "val_long");
    throw_if_empty(help, "help");
    throw_if_empty(more_help, "more_help");

    if (val_short == 0) {
        throw error("val_short == 0");
    }

    m_args.push_back( { val_long, val_short, val_arg, help, more_help } );
}


/* call getopt_long() and save return value */
bool getopt_long_cxx::getopt(int &c)
{
    init_longopts();

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
    for (const auto &arg : m_args) {
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
    for (const auto &arg : m_args) {
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

        std::cout << '\n' << std::endl;
    }
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

    const char *help_text = NULL;
    const char *arg = m_argv[2];
    const size_t arglen = strlen(arg);

    if (arglen == 2 && arg[0] == '-') {
        /* searching for short opt */
        for (const auto &e : m_args) {
            if (e.val_short == arg[1]) {
                help_text = e.more_help;
                break;
            }
        }
    } else if (arglen > 2 && arg[0] == '-' && arg[1] == '-') {
        /* searching for long opt */
        for (const auto &e : m_args) {
            if (strcmp(e.val_long, arg + 2) == 0) {
                help_text = e.more_help;
                break;
            }
        }
    }

    if (!help_text) {
        std::string msg = "unknown option: ";
        msg += arg;
        throw error(msg);
    }

    std::cout << help_text << '\n' << std::endl;

    return true;
}
