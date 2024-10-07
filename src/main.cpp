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

#include <iostream>
#include <list>
#include <regex>
#include <stdlib.h>

#include "gendlopen.hpp"


struct parse_args
{
    int argc;
    char **argv;
    int it;
    const char *cur;
    const char *opt;
};


/* anonymous */
namespace
{
    inline bool is_path_separator(const char &c)
    {
#ifdef _WIN32
        return (c == '\\' || c == '/');
#else
        return (c == '/');
#endif
    }


    inline bool is_arg_prefix(const char &c)
    {
#ifdef _WIN32
        return (c == '-' || c == '/');
#else
        return (c == '-');
#endif
    }


    /* create "#define" lines */
    std::string format_def(std::string def)
    {
        std::string name, value, out;
        const size_t pos = def.find('=');

        if (pos == std::string::npos) {
            name = def;
        } else {
            name = def.substr(0, pos);
            value = ' ' + def.substr(pos + 1);
        }

        utils::strip_spaces(name);

        if (name.empty()) {
            /* empty string will be "appended" to code */
            return "";
        }

        out  = "#ifndef "  + (name + '\n');
        out += "# define " + (name + (value + '\n'));
        out += "#endif\n";

        return out;
    }


    /* quote library name */
    std::string quote_lib(const std::string &lib, bool wide)
    {
        if (wide) {
            if (lib.starts_with("L\"") && lib.back() == '"') {
                /* already quoted */
                return lib;
            } else if (lib.front() == '"' && lib.back() == '"') {
                /* prepend 'L' */
                return 'L' + lib;
            }

            return "L\"" + (lib + '"');
        }

        if (lib.front() == '"' && lib.back() == '"') {
            /* already quoted */
            return lib;
        }

        return '"' + (lib + '"');
    }


    /**
     * format library name
     * foo        ==>  "foo"
     * nq:foo     ==>  foo
     * ext:foo    ==>  "foo" LIBEXTA
     * api:2:foo  ==>  LIBNAMEA(foo,2)
     */
    void format_libname(const std::string &str, std::string &lib_a, std::string &lib_w)
    {
        switch(str.at(0))
        {
        case 'N':
        case 'n':
            /* no quotes */
            if (utils::prefixed_case(str, "nq:")) {
                lib_a = lib_w = str.substr(3);
                return;
            }
            break;

        case 'E':
        case 'e':
            /* quotes + file extension macro */
            if (utils::prefixed_case(str, "ext:")) {
                auto sub = str.substr(4);
                lib_a = quote_lib(sub, false) + " LIBEXTA";
                lib_w = quote_lib(sub, true) + " LIBEXTW";
                return;
            }
            break;

        case 'A':
        case 'a':
            /* no quotes, API libname macro */
            if (utils::prefixed_case(str, "api:")) {
                const std::regex reg("(.*?):(.*)");
                std::smatch m;
                auto sub = str.substr(4);

                if (std::regex_match(sub, m, reg) && m.size() == 3) {
                    /* LIBNAMEA(xxx,0) */
                    lib_w = lib_a = "LIBNAMEA(" + (m[2].str() + (',' + (m[1].str() + ')')));
                    lib_w[7] = 'W';
                    return;
                }
            }
            break;

        default:
            break;
        }

        /* quote string */
        lib_a = quote_lib(str, false);
        lib_w = quote_lib(str, true);
    }


    /* quote header name if needed */
    std::string format_inc(const std::string &inc)
    {
        if (utils::prefixed_case(inc, "nq:")) {
            /* no quotes */
            return inc.substr(3);
        }

        if ((inc.front() == '<' && inc.back() == '>') ||
            (inc.front() == '"' && inc.back() == '"'))
        {
            /* already quoted */
            return inc;
        }

        /* add quotes */
        return '"' + (inc + '"');
    }


    /* format */
    output::format format_enum(const char *in)
    {
        std::string s = utils::convert_to_lower(in, false);
        output::format out = output::error;

        if (s.front() == 'c') {
            if (s == "c") {
                out = output::c;
            } else if (s == "cxx" || s == "c++" || s == "cpp") {
                out = output::cxx;
            }
        } else if (s.starts_with("minimal")) {
            s.erase(0, 7);

            if (s.empty() || s == "-c") {
                out = output::minimal;
            } else if (s == "-cxx" || s == "-c++" || s == "-cpp") {
                out = output::minimal_cxx;
            }
        }

        if (out == output::error) {
            std::cerr << "error: unknown output format given: " << in << std::endl;
            std::exit(1);
        }

        return out;
    }


    const char *get_prog_name(const char *prog)
    {
        /* get program name without full path */
#ifdef HAVE_PROGRAM_INVOCATION_NAME
        prog = program_invocation_short_name;
#elif defined(HAVE_GETPROGNAME)
        prog = getprogname();
#else
        for (auto p = prog; *p != 0; p++) {
            if (is_path_separator(*p) && *(p+1) != 0) {
                prog = p + 1;
            }
        }
#endif

        return prog;
    }


    /* get argument from an option string */
    bool get_argx(struct parse_args &args, const char *opt, const size_t optlen)
    {
        auto err_noarg = [&] () {
            std::cerr << get_prog_name(args.argv[0]) << ": option requires an argument: "
                << args.argv[args.it] << std::endl;
            std::exit(1);
        };

        args.opt = NULL;

        if (strncmp(args.cur, opt, optlen) != 0) {
            /* not the argument we're looking for */
            return false;
        }

        /* "-foo bar" --> get next item */
        if (strcmp(args.cur, opt) == 0) {
            if ((args.it + 1) >= args.argc) {
                err_noarg();
            }

            args.opt = args.argv[++args.it];

            return true;
        }

        /* -foo=bar, -Dfoo --> get substring */
        if (strlen(args.cur) > optlen) {
            if (optlen == 1) {
                /* -Dfoo */
                args.opt = args.cur + 1;
            } else if (args.cur[optlen] == '=') {
                /* -foo=bar */
                args.opt = args.cur + optlen + 1;
            }

            if (!args.opt || *args.opt == 0) {
                err_noarg();
            }

            return true;
        }

        return false;
    }

    template<size_t N>
    constexpr bool get_arg(struct parse_args &args, char const (&opt)[N])
    {
        return get_argx(args, opt, N-1);
    }


    /* option without argument */
    bool get_noargx(struct parse_args &args, const char *opt, const size_t optlen)
    {
        /* -foo */
        if (strcmp(args.cur, opt) == 0) {
            return true;
        }

        /* -foo=bar */
        if (strncmp(args.cur, opt, optlen) == 0 && args.cur[optlen] == '=') {
            std::cerr << get_prog_name(args.argv[0]) << ": option does not take an argument: "
                << *args.argv[args.it] << opt << std::endl;
            std::exit(1);
        }

        return false;
    }

    template<size_t N>
    constexpr bool get_noarg(struct parse_args &args, char const (&opt)[N])
    {
        return get_noargx(args, opt, N-1);
    }

} /* anonymous namespace */


int main(int argc, char **argv)
{
    auto try_help = [&argv] () {
        std::cerr << "Try `" << argv[0] << " -help' for more information." << std::endl;
    };

    auto prog = [&argv] () -> const char* {
        return get_prog_name(argv[0]);
    };

    struct parse_args args;
    const char *input = NULL;

    /* default settings */
    const char *output = "-";
    const char *name = "gdo";

    /* initialize class */
    gendlopen gdo(argc, argv);

    args.argc = argc;
    args.argv = argv;

    /* parse arguments */
    for (args.it = 1; args.it < argc; args.it++) {
        args.cur = argv[args.it];

        /* non-option argument --> input */
        if (!is_arg_prefix(args.cur[0]) || strcmp(args.cur, "-") == 0) {
            if (!input) {
                input = args.cur;
            } else {
                std::cerr << "warning: non-option argument ignored: " << args.cur << std::endl;
            }
            continue;
        }

        auto cur = args.cur;

        /* skip prefix */
        args.cur++;

        switch(*args.cur)
        {
        case '?':
            if ( get_noarg(args, "?") ) {
                help::print(prog());
                return 0;
            }
            break;

        case 'h':
            if ( get_noarg(args, "help") ) {
                help::print(prog());
                return 0;
            }
            break;

        case 'f':
            if ( get_arg(args, "format") ) {
                gdo.format(format_enum(args.opt));
                continue;
            } else if ( get_noarg(args, "force") ) {
                gdo.force(true);
                continue;
            } else if ( get_noarg(args, "full-help") ) {
                help::print_full(prog());
                return 0;
            }
            break;

        case 'o':
            if ( get_arg(args, "o") ) {
                output = args.opt;
                continue;
            }
            break;

        case 'n':
            if ( get_arg(args, "name") ) {
                name = args.opt;
                continue;
            }
            break;

        case 't':
            if ( get_arg(args, "template") ) {
                gdo.custom_template(args.opt);
                continue;
            }
            break;

        case 'l':
            if ( get_arg(args, "library") ) {
                std::string lib_a, lib_w;
                format_libname(args.opt, lib_a, lib_w);
                gdo.default_lib(lib_a, lib_w);
                continue;
            }
            break;

        case 'i':
            if ( get_arg(args, "include") ) {
                gdo.add_inc(format_inc(args.opt));
                continue;
            }
            break;

        case 'D':
            if ( get_arg(args, "D") ) {
                gdo.add_def(format_def(args.opt));
                continue;
            }
            break;

        case 'P':
            if ( get_arg(args, "P") ) {
                gdo.add_pfx(args.opt);
                continue;
            }
            break;

        case 'p':
            if ( get_arg(args, "param") ) {
                if (utils::eq_str_case(args.opt, "skip")) {
                    gdo.parameter_names(param::skip);
                } else if (utils::eq_str_case(args.opt, "create")) {
                    gdo.parameter_names(param::create);
                } else {
                    std::cerr << prog() << ": unknown argument for option '"
                        << *cur << "param': " << args.opt << std::endl;
                    try_help();
                    return 1;
                }
                continue;
            }
            break;

        case 'S':
            if ( get_arg(args, "S") ) {
                gdo.add_sym(args.opt);
                continue;
            }
            break;

        case 's':
            if ( get_noarg(args, "separate") ) {
                gdo.separate(true);
                continue;
            }
            break;

        case 'a':
            if ( get_noarg(args, "ast-all-symbols") ) {
                gdo.ast_all_symbols(true);
                continue;
            }
            break;

        default:
            break;
        }

        std::cerr << prog() << ": unknown option: " << cur << std::endl;
        try_help();
        return 1;
    }

    /* input is required */
    if (!input) {
        std::cerr << prog() << ": input file required" << std::endl;
        try_help();
        return 1;
    }

    /* generate output */
    try {
        gdo.generate(input, output, name);
    }
    catch (const gendlopen::error &e) {
        std::cerr << prog() << ": " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
