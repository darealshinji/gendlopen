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

#if defined(__linux__) || defined(__GNU__)
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
#include <sstream>
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
        std::string name, value;
        const size_t pos = def.find('=');

        if (pos == std::string::npos) {
            name = def;
        } else {
            name = def.substr(0, pos);
            value = " " + def.substr(pos + 1);
        }

        utils::strip_spaces(name);

        if (name.empty()) {
            /* empty string will be "appended" to code */
            return {};
        }

        std::stringstream out;
        out << "#ifndef " << name << '\n';
        out << "# define " << name << value << '\n';
        out << "#endif\n";

        return out.str();
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
                return "L" + lib;
            }

            return "L\"" + (lib + "\"");
        }

        if (lib.front() == '"' && lib.back() == '"') {
            /* already quoted */
            return lib;
        }

        return "\"" + (lib + "\"");
    }


    /**
     * format library name
     * foo        ==>  "foo"
     * nq:foo     ==>  foo
     * ext:foo    ==>  "foo" LIBEXTA
     * api:2:foo  ==>  LIBNAMEA(foo,2)
     */
    std::string format_libname(const std::string &str, bool iswide)
    {
        const char *libext = iswide ? " LIBEXTW" : " LIBEXTA";
        const char *libname = iswide ? "LIBNAMEW(" : "LIBNAMEA(";

        switch(str.at(0))
        {
        case 'N':
        case 'n':
            /* no quotes */
            if (utils::prefixed_case(str, "nq:")) {
                return str.substr(3);
            }
            break;

        case 'E':
        case 'e':
            /* quotes + file extension macro */
            if (utils::prefixed_case(str, "ext:")) {
                return quote_lib(str.substr(4), iswide) + libext;
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
                    std::stringstream out;
                    out << libname << m[2] << ',' << m[1] << ')';
                    return out.str();
                }
            }
            break;

        default:
            break;
        }

        /* quote string */
        return quote_lib(str, iswide);
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
        return "\"" + (inc + "\"");
    }


    /* format */
    void str_to_format_enum(const std::string &in, output::format &out)
    {
        std::string s = utils::convert_to_lower(in, false);

        if (s.front() == 'c') {
            if (s == "c") {
                out = output::c;
                return;
            } else if (s == "cxx" || s == "c++" || s == "cpp") {
                out = output::cxx;
                return;
            }
        } else if (s.starts_with("minimal")) {
            s.erase(0, 7);

            if (s.empty() || s == "-c") {
                out = output::minimal;
                return;
            } else if (s == "-cxx" || s == "-c++" || s == "-cpp") {
                out = output::minimal_cxx;
                return;
            }
        }

        std::cerr << "error: unknown output format given: " << in << std::endl;
        std::exit(1);
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


    void print_help(const char *prog)
    {
        std::cout << "usage: " << prog << " [OPTIONS..] <file>\n"
            "\n"
#ifdef _WIN32
            "options may be prefixed with `-' or `/'\n"
            "\n"
#endif
            "  <file>            input file, use `-' to read from stdin\n"
            "\n"
            "options:\n"
            "  -help             display this information\n"
            "  -full-help        show more detailed information\n"
            "  -o<file>          save to file instead of stdout\n"
            "  -name=<string>    use <string> in names of functions, macros and C++ namespaces (default: gdo)\n"
            "  -format=<string>  set output format: c (default), c++, minimal, minimal-c++\n"
            "  -template=<file>  use a custom template (`-format' will be ignored)\n"
            "  -library=[<mode>:]<lib>    set a default library name to load; if <mode> is 'nq' no quotes are\n"
            "                    added, 'ext' will append a file extension to the library name and 'api:#'\n"
            "                    will create a library filename with API number\n"
            "  -include=[nq:]<file>    include a header file *;\n"
            "                    nq:<file> - no quotes are added, the string will be used as is\n"
            "  -D<string>        define a preprocessor macro *\n"
            "  -P<string>        look for symbols prefixed with <string> *\n"
            "  -S<string>        look for symbol name <string> *\n"
            "  -separate         save output into separate body and header files\n"
            "  -force            always overwrite existing output files\n"
            "  -skip-param       don't look for parameter names in function prototypes\n"
            "  -ast-all-symbols  use all symbols from a Clang AST\n"
            "\n"
            "  * option may be passed multiple times" << std::endl;
    }


    void print_full_help(const char *prog)
    {
        std::cout << "usage: " << prog << " [OPTIONS..] <file>\n"
            "\n"
#ifdef _WIN32
            "options may be prefixed with `-' or `/'\n"
            "\n"
#endif

            "  <file>\n"
            "    Specify an input file. Use `-' to read data from stdin.\n"
            "\n"
            "    The input text must contain all symbols that should be loaded.\n"
            "    They must be listed as modern C-style prototypes, ending on semi-colon (;).\n"
            "    Comments are ignored, line-breaks are treated as spaces.\n"
            "    Any other code will throw an error.\n"
            "\n"
            "    Alternatively the input may be an Abstract Syntax Tree (AST) generated by Clang.\n"
            "    To dump the AST created from `foo.h' you can run the following command:\n"
            "    clang -Xclang -ast-dump foo.h > ast.txt\n"
            "\n"


            "Options:\n"
            "\n"

            "  -help\n"
            "    Show a brief description of all command line arguments.\n"
            "\n"

            "  -full-help\n"
            "    Show more detailed information.\n"
            "\n"


            "  -o<file>\n"
            "    Specify an output file.\n"
            "    If this flag isn't set or if <file> is `-' output will be printed to stdout.\n"
            "\n"


            "  -name=<string>\n"
            "    Use <string> as a prefix in names of functions and macros or as C++\n"
            "    namespace when generating output to avoid symbol clashes.\n"
            "    The default string is `gdo'.\n"
            "    Upper/lower case and underscores will be set accordingly.\n"
            "\n"


            "  -format=<string>\n"
            "    Use one of the following output formats:\n"
            "    C            -  many features, this is the default\n"
            "    C++          -  many features but no exception handling (due to the design)\n"
            "    minimal      -  small C header\n"
            "    minimal-C++  -  small C++ header with exception handling\n"
            "\n"
            "    More information can be found in the comments of the header files.\n"
            "\n"


            "  -template=<file>\n"
            "    Use a custom template file to generate output from.\n"
            "    The flag `-format' will be ignored in this case.\n"
            "\n"
            "    Text substitution in the template file is done as following:\n"
            "\n"
            "    If a prefix was set with `-name' then any instances of `GDO_' and `gdo_'\n"
            "    will be substituted with it, converted to uppercase and lowercase.\n"
            "\n"
            "    Any lines containing one or more of the following will be replaced multiple\n"
            "    times with code from the input (used to generate typedefs, prototyes, etc.):\n"
            "\n"
            "    %%type%%: function return type\n"
            "    %%func_symbol%%: function symbol name\n"
            "    %%args%%: function arguments\n"
            "    %%notype_args%%: function argument names without type\n"
            "    %%return%%: empty if function doesn't return anything (void), else `return'\n"
            "\n"
            "    %%obj_type%%: object type\n"
            "    %%obj_symbol%%: object symbol name\n"
            "\n"
            "    %%sym_type%%: function or object symbol type\n"
            "    %%symbol%: function or object symbol name\n"
            "\n"
            "    If a line ends on `@' it will be processed together with the next line as if\n"
            "    there was no line break, but the line break will still appear in the output.\n"
            "\n"
            "    All lines between a line `%SKIP_PARAM_UNUSED_BEGIN%' and a line `%SKIP_PARAM_UNUSED_END%'\n"
            "    will be commented out if `-skip-param' was passed.\n"
            "    This will skip code that would otherwise require parameter names.\n"
            "\n"
            "    Any line that contains `%DNL%' (\"Do Not Lex\") will be skipped entirely.\n"
            "    This can be used for meta comments. This is done AFTER glueing lines that end on `@'.\n"
            "\n"


            "  -library=[<mode>:]<lib>\n"
            "    Set a default library name to load.\n"
            "    If no mode was set quotation marks are put around the filename as needed.\n"
            "\n"
            "    Available modes:\n"
            "    nq    - no quotes are added, the string will be used as is\n"
            "    ext   - filename extension will be added automatically through a macro\n"
            "    api:# - library filename with API number will be created through a macro\n"
            "\n"
            "    -library=foo        ==>  \"foo\"\n"
            "    -library=nq:foo     ==>  foo\n"
            "    -library=ext:foo    ==>  \"foo\" LIBEXTA    ==>  i.e. \"foo.dll\"\n"
            "    -library=api:2:foo  ==>  LIBNAMEA(foo,2)  ==>  i.e. \"libfoo.so.2\"\n"
            "\n"


            "  -include=[nq:]<file>\n"
            "    Set a header file name to be included at the top of the output code.\n"
            "    Quotation marks are put around the filename if it's not enclosed in\n"
            "    brackets or quotation marks or if it's not prefixed with \"nq:\".\n"
            "\n"
            "    -include=foo.h      ==>  #include \"foo.h\"\n"
            "    -include='\"foo.h\"'  ==>  #include \"foo.h\"\n"
            "    -include=<foo.h>    ==>  #include <foo.h>\n"
            "    -include=nq:foo     ==>  #include foo\n"
            "\n"
            "    This flag may be passed multiple times.\n"
            "\n"


            "  -D<string>\n"
            "    Set a preprocessor definition macro to be added at the top of the output code.\n"
            "    This macro may include a value in the form of `FOO=1'.\n"
            "\n"
            "    This flag may be passed multiple times.\n"
            "\n"


            "  -P<string>\n"
            "    Look for symbols that begin with <string> when parsing the input.\n"
            "    This is most useful if the input is a Clang AST to ignore unwanted\n"
            "    declarations coming from i.e. standard C headers.\n"
            "\n"
            "    This flag may be passed multiple times.\n"
            "\n"


            "  -S<string>\n"
            "    Look for the symbol name <string> when parsing the input.\n"
            "    This is most useful if the input is a Clang AST,\n"
            "    to ignore unwanted declarations coming from i.e. standard C headers.\n"
            "\n"
            "    This flag may be passed multiple times.\n"
            "\n"


            "  -separate\n"
            "    Save output into separate body and header files.\n"
            "    The filename extensions will be set to .c/.h or .cpp/.hpp accordingly.\n"
            "    This flag is ignored if the output is printed to stdout or if the\n"
            "    output format is \"minimal-C\" or \"minimal-C++\".\n"
            "\n"


            "  -force\n"
            "    Always overwrite existing output files. Use with care.\n"
            "\n"


            "  -skip-param\n"
            "    Don't try to look for parameter names in function prototypes when\n"
            "    the input is being processed.\n"
            "    This will disable any kind of wrapped functions in the output (if\n"
            "    the selected output format makes use of them).\n"
            "\n"


            "  -ast-all-symbols\n"
            "    Pass this flag if you really want to use all symbols found in a Clang AST.\n"
            "    Be careful as this might include unwanted prototypes from other headers.\n"
            "    It's recommended to use `-P' and/or `-S' instead.\n"
            "    This flag cannot be combined with `-P' or `-S'.\n"
            "\n"
            "    This flag is ignored if the input is not a Clang AST.\n"

            << std::endl;
    }


    /* get argument from an option string */
    bool get_argx(struct parse_args &args, const char *opt, size_t optlen)
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
    bool get_noargx(struct parse_args &args, const char *opt, size_t optlen)
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
    std::string input;

    /* default settings */
    std::string output = "-";
    std::string name = "gdo";
    output::format fmt = output::c;

    /* initialize class */
    gendlopen gdo(argc, argv);

    args.argc = argc;
    args.argv = argv;

    /* parse arguments */
    for (args.it = 1; args.it < argc; args.it++) {
        args.cur = argv[args.it];

        /* non-option argument --> input */
        if (!is_arg_prefix(args.cur[0]) || strcmp(args.cur, "-") == 0) {
            if (input.empty()) {
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
        case 'h':
            if ( get_noarg(args, "help") || get_noarg(args, "?") ) {
                print_help(prog());
                return 0;
            }
            break;

        case 'f':
            if ( get_arg(args, "format") ) {
                str_to_format_enum(args.opt, fmt);
                gdo.format(fmt);
                continue;
            } else if ( get_noarg(args, "force") ) {
                gdo.force(true);
                continue;
            } else if ( get_noarg(args, "full-help") ) {
                print_full_help(prog());
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
                gdo.default_lib(
                    format_libname(args.opt, false),
                    format_libname(args.opt, true)
                );
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
            } else if ( get_noarg(args, "skip-param") ) {
                gdo.skip_parameter_names(true);
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
    if (input.empty()) {
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
