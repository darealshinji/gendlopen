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

#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include "gendlopen.hpp"


/* helper functions to parse command line arguments */
namespace args
{

/* search vector for boolean flag, delete from vector if found */
bool parse_vector_for_flag(
    vstring_t &vec,
    const std::string &long_opt,
    const std::string &short_opt)
{
    auto error_value = [] (const std::string &opt, const std::string &val) {
        std::cerr << "error: flag `" << opt << "' provided a value but none was expected: "
            << val << std::endl;
        std::exit(1);
    };

    for (auto it = vec.begin(); it != vec.end(); it++) {
        auto arg = *it;

        /* short option */
        if (!short_opt.empty()) {
            if (arg == short_opt) {
                /* -x */
                vec.erase(it);
                return true;
            } else if (arg.size() > 2 && arg.starts_with(short_opt)) {
                /* -xvalue */
                error_value(short_opt, arg.substr(2));
            }
        }

        /* long option */
        if (!long_opt.empty()) {
            if (arg == long_opt) {
                /* --arg */
                vec.erase(it);
                return true;
            } else if (arg.size() > long_opt.size() &&
                arg.starts_with(long_opt) && arg.at(long_opt.size()) == '=')
            {
                /* --arg=value */
                arg.erase(0, long_opt.size() + 1);
                error_value(long_opt, arg);
            }
        }
    }

    return false;
}

/* search vector for argument with value, delete from vector if found */
std::string parse_vector_for_value(
    vstring_t &vec,
    const std::string &long_opt,
    const std::string &short_opt)
{
    auto error_no_value = [] (const std::string &opt) {
        std::cerr << "error: argument `" << opt << "' expected a value but none was provided" << std::endl;
        std::exit(1);
    };

    auto error_empty_value = [] (const std::string &opt) {
        std::cerr << "error: argument `" << opt << "' expected a non-empty value" << std::endl;
        std::exit(1);
    };

    std::string value;
    auto it = vec.begin();
    auto next = vec.begin() + 1;

    /* get value from next iterator */
    auto get_next_iter = [&] (const std::string &opt) {
        if (next == vec.end()) {
            error_no_value(opt);
        }

        value = *next;

        if (value.empty()) {
            error_empty_value(opt);
        }

        vec.erase(it, next+1);
    };

    for ( ; it != vec.end(); ++it, ++next) {
        auto arg = *it;

        /* short option */
        if (!short_opt.empty()) {
            if (arg == short_opt) {
                /* -x value */
                get_next_iter(short_opt);
                return value;
            } else if (arg.size() > 2 && arg.starts_with(short_opt)) {
                /* -xvalue */
                vec.erase(it);
                return arg.substr(2);
            }
        }

        /* long option */
        if (!long_opt.empty()) {
            if (arg == long_opt) {
                /* --arg value */
                get_next_iter(long_opt);
                return value;
            } else if (arg.size() > long_opt.size() &&
                arg.starts_with(long_opt) && arg.at(long_opt.size()) == '=')
            {
                /* --arg=value */
                arg.erase(0, long_opt.size() + 1);

                if (arg.empty()) {
                    error_empty_value(long_opt);
                }

                vec.erase(it);
                return arg;
            }
        }
    }

    return {};
}

/* find boolean flag */
bool get_flag(vstring_t &vec, const arg_t &arg)
{
    std::string long_opt, short_opt;

    if (arg.long_opt != NULL) {
        long_opt = "--";
        long_opt += arg.long_opt;
    }

    if (arg.short_opt != 0) {
        short_opt = "-";
        short_opt += arg.short_opt;
    }

    if (!parse_vector_for_flag(vec, long_opt, short_opt)) {
        return false;
    }

    /* parse the rest to clear redundant flags from vector */
    while (parse_vector_for_flag(vec, long_opt, short_opt))
    {}

    return true;
}

/* find argument with value */
std::string get_value(vstring_t &vec, const arg_t &arg)
{
    std::string long_opt, short_opt;

    if (arg.long_opt != NULL) {
        long_opt = "--";
        long_opt += arg.long_opt;
    }

    if (arg.short_opt != 0) {
        short_opt = "-";
        short_opt += arg.short_opt;
    }

    std::string value = parse_vector_for_value(vec, long_opt, short_opt);

    if (value.empty()) {
        return "";
    }

    /* we don't want the argument to be used multiple times */
    std::string temp = parse_vector_for_value(vec, long_opt, short_opt);

    if (!temp.empty()) {
        std::cerr << "error: argument `" << long_opt << "' was provided more than once" << std::endl;
        std::exit(1);
    }

    return value;
}

/* find argument(s) with value and add to list */
vstring_t get_value_list(vstring_t &vec, const arg_t &arg)
{
    vstring_t list;
    std::string long_opt, short_opt;

    if (arg.long_opt != NULL) {
        long_opt = "--";
        long_opt += arg.long_opt;
    }

    if (arg.short_opt != 0) {
        short_opt = "-";
        short_opt += arg.short_opt;
    }

    std::string value = parse_vector_for_value(vec, long_opt, short_opt);

    while (!value.empty()) {
        list.push_back(value);
        value = parse_vector_for_value(vec, long_opt, short_opt);
    }

    return list;
}

/* parse "help <cmd>" switch */
int parse_help_switch(int &argc, char **&argv, const std::vector<arg_t> &help_args)
{
    if (argc != 3) {
        std::cerr << "error: switch `help' expected an option but none was provided" << std::endl;
        return 1;
    }

    std::string arg = argv[2];
    const char *help_text = nullptr;

    if (arg.size() == 2 && arg.front() == '-') {
        /* searching short opts */
        char opt = arg.at(1);

        for (const auto &e : help_args) {
            if (opt == e.short_opt) {
                help_text = e.more_help;
                break;
            }
        }
    } else if (arg.size() > 3 && arg.starts_with("--")) {
        /* searching long opts */
        arg.erase(0, 2);

        for (const auto &e : help_args) {
            if (arg == e.long_opt) {
                help_text = e.more_help;
                break;
            }
        }
    }

    /* print help for option */
    if (help_text) {
        std::cout << "Option `" << argv[2] << "':\n" << help_text << '\n' << std::endl;
        return 0;
    }

    /* nothing found */
    std::cerr << "error: option not found: " << arg << std::endl;

    return 1;
}

/* vectorize arguments and check for --help */
vstring_t parse_args(int &argc, char **&argv, const std::vector<arg_t> &help_args)
{
    auto print_usage = [] (const char *argv0) {
        std::cout << "usage: " << argv0 << " [OPTIONS..]\n";
        std::cout << "       " << argv0 << " help <option>\n\n";
        std::cout << "Options:\n";
        std::cout << std::endl;
    };

    vstring_t vec;

    for (int i = 1; i < argc; i++) {
        /* quick help check */
        if (*argv[i] == '-') {
            if (strcmp(argv[i], "-h") == 0 ||
                strcmp(argv[i], "--help") == 0 ||
                strcmp(argv[i], "-?") == 0)
            {
                print_usage(*argv);

                for (const auto &e : help_args) {
                    std::cout << e.help << std::endl;
                }
                std::cout << std::endl;
                std::exit(0);
            } else if (strcmp(argv[i], "--full-help") == 0) {
                print_usage(*argv);

                for (const auto &e : help_args) {
                    std::cout << e.more_help << '\n' << std::endl;
                }
                std::exit(0);
            }
        }

        /* copy args into vector */
        vec.push_back(argv[i]);
    }

    return vec;
}

} /* namespace args end */
