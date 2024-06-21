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
#include <algorithm>
#include <fstream>
#include <iostream>
#include <list>
#include <ranges>
#include <sstream>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "getopt_long_cxx.hpp"
#include "types.hpp"


/* c'tor */
getopt_long_cxx::getopt_long_cxx(int &argc, char **&argv, const std::list<arg_t> &args)
    : m_args(&args)
{
    for (const auto &opt : *m_args) {
        if (opt.val_short == '@') {
            parse_atfile_args(argc, argv);
        }
        break;
    }
}


/* d'tor */
getopt_long_cxx::~getopt_long_cxx()
{
}


/* check for @file argument */
void getopt_long_cxx::check_atfile_arg(const char *arg)
{
    std::string line;
    std::ifstream ifs;

    /* try to open arg as a file */
    if (*arg == '@') {
        //std::cerr << "DEBUG: open file: " << (arg + 1) << std::endl;
        ifs.open(arg + 1);
    }

    if (ifs.is_open()) {
        /* read lines, parse @file args recursively */
        while (std::getline(ifs, line)) {
            if (line[0] == '@' && line != arg) {
                check_atfile_arg(line.c_str());
            } else {
                m_argv_strs.push_back(line);
            }
        }
    } else {
        m_argv_strs.push_back(arg);
    }
}


/* recursive checks for @file argument */
void getopt_long_cxx::parse_atfile_args(int &argc, char **&argv)
{
    /* add argv[0] first */
    m_argv_strs.push_back(argv[0]);

    /* check arguments for @file and parse if needed */
    for (int i = 1; i < argc; i++) {
        check_atfile_arg(argv[i]);
    }

    /* create new arg vector */
    for (size_t i = 0; i < m_argv_strs.size(); i++) {
        char *ptr = const_cast<char *>(m_argv_strs.at(i).c_str());
        m_argv_ptrs.push_back(ptr);
    }

    m_argc = static_cast<int>(m_argv_ptrs.size());
    m_argv_ptrs.push_back(nullptr);
    m_argv = m_argv_ptrs.data();
}


/* initialize options for parsing with getopt_long() */
void getopt_long_cxx::init()
{
    /* already initialized? */
    if (!m_longopts.empty()) {
        return;
    }

#if (defined(__GNUC__) && !defined(__OPTIMIZE__)) || \
    (defined(_MSC_VER) && defined(_DEBUG))

    /* GCC/Clang: optimization disabled
     * MSVC: debug build enabled */

    auto throw_if_empty = [] (const char *val, const std::string &msg) {
        if (!val) {
            throw error(msg + " == NULL");
        } else if (*val == 0) {
            throw error(msg + " is empty string");
        }
    };

    auto check_dup = [] (vstring_t &list, const std::string &str)
    {
        std::sort(list.begin(), list.end());
        auto it = std::ranges::adjacent_find(list);

        if (it != list.end()) {
            std::string msg = "getopt_long() " + str;
            msg += " option `";
            msg += *it;
            msg += "' is not unique";
            throw error(msg);
        }
    };

    vstring_t list_long, list_short;

    /* check for empty options */
    for (const auto &opt : *m_args) {
        throw_if_empty(opt.val_long, "val_long");
        throw_if_empty(opt.help, "help");
        throw_if_empty(opt.more_help, "more_help");

        if (opt.val_short == 0) {
            throw error("val_short == 0");
        }

        list_long.push_back(opt.val_long);
        list_short.push_back( std::string{ 1, opt.val_short } );
    }

    /* check for duplicates */
    check_dup(list_long, "long");
    check_dup(list_short, "short");

#endif

    /* create option table and optstring */
    for (const auto &opt : *m_args) {
        int val_arg = has_arg(opt) ? required_argument : no_argument;

        struct option lopt = { opt.val_long, val_arg, NULL, opt.val_short };
        m_longopts.push_back(lopt);

        /* append short option to optstring */
        m_optstring.push_back(opt.val_short);

        if (val_arg == required_argument) {
            m_optstring.push_back(':');
        }
    }

    m_longopts.push_back( { NULL, 0, NULL, 0 } );
}


/* call getopt_long() and save return value */
bool getopt_long_cxx::getopt(int &c)
{
    init();

    /* returns -1 if there are no more option characters */
    c = getopt_long(m_argc, m_argv, m_optstring.c_str(), m_longopts.data(), &m_longindex);

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

        if (arg.val_short == '@') {
            /* @file */
            if (has_arg(arg)) {
                line = "  @";
                line += arg.val_arg;
            } else {
                line = "  @file";
            }
        } else {
            if (arg.val_short > ' ') {
                /* short option is a printable character */
                line = "  -";
                line += arg.val_short;
            } else {
                line = "    ";
            }

            line += " --";
            line += arg.val_long;

            if (has_arg(arg)) {
                line += '=';
                line += arg.val_arg;
                line += " ";
            }
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

    if (arg.val_short == '@') {
        if (has_arg(arg)) {
            line += '@';
            line += arg.val_arg;
        } else {
            line += "@file";
        }
    } else {
        if (arg.val_short > ' ') {
            /* short option is a printable character */
            line += '-';
            line += arg.val_short;

            if (has_arg(arg)) {
                line += ' ';
                line += arg.val_arg;
            }
            line += ", --";
        } else {
            line += "--";
        }

        line += arg.val_long;

        if (has_arg(arg)) {
            line += '=';
            line += arg.val_arg;
        }
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

    if (*opt == '@') {
        opt = "@";
    }

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
