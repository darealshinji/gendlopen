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
#include "getopt_long_cxx.hpp"


/* anonymous */
namespace
{
    /* short option values */
    enum {
        ARG_AST_ALL_SYM = 1,
        ARG_DEFINE      = 'D',
        ARG_FORCE       = 'f',
        ARG_FORMAT      = 'F',
        ARG_FULL_HELP   = 2,
        ARG_HELP        = 'h',
        ARG_INLCUDE     = 'I',
        ARG_INPUT       = 'i',
        ARG_LIBRARY     = 'l',
        ARG_NAME        = 'n',
        ARG_OUTPUT      = 'o',
        ARG_PREFIX      = 'P',
        ARG_SEPARATE    = 's',
        ARG_SKIP_PARAM  = 3,
        ARG_SYMBOL      = 'S',
        ARG_TEMPLATE    = 4
    };


    template<typename T=char>
    bool is_path_separator(T c)
    {
#ifdef _WIN32
        return (c == '\\' || c == '/');
#else
        return (c == '/');
#endif
    }


    /* create "#define" lines */
    std::string format_def(std::string def)
    {
        std::string name, value;
        std::stringstream out;

        size_t pos = def.find('=');

        if (pos == std::string::npos) {
            name = def;
        } else {
            name = def.substr(0, pos);
            value = " " + def.substr(pos + 1);
        }

        utils::strip_spaces(name);

        if (name.empty()) {
            std::cerr << "error: flag 'define' cannot be just white-spaces" << std::endl;
            std::exit(1);
        }

        out << "#ifndef " << name << '\n';
        out << "#define " << name << value << '\n';
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

        if (utils::prefixed_case(str, "nq:")) {
            /* no quotes */
            return str.substr(3);
        }
        else if (utils::prefixed_case(str, "ext:")) {
            /* quotes + extension macro */
            return quote_lib(str.substr(4), iswide) + libext;
        }
        else if (utils::prefixed_case(str, "api:")) {
            /* no quotes, API libname macro */
            const std::regex reg("(.*?):(.*)");
            std::smatch m;
            auto sub = str.substr(4);

            if (std::regex_match(sub, m, reg) && m.size() == 3) {
                std::stringstream out;
                out << libname << m[2] << ',' << m[1] << ')';
                return out.str();
            }
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
    void str_to_format_enum(const char *in, output::format &out)
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
            if (s == "minimal" || s == "minimal-c") {
                out = output::minimal;
                return;
            } else if (s == "minimal-cxx" || s == "minimal-c++" ||
                s == "minimal-cpp")
            {
                out = output::minimal_cxx;
                return;
            }
        }

        std::cerr << "error: unknown output format given: " << in << std::endl;
        std::exit(1);
    }


    /* usage: <PROG> ... */
    void print_usage(const char *prog)
    {
        /* get program name without full path */
#ifdef HAVE_PROGRAM_INVOCATION_NAME
        prog = program_invocation_short_name;
#elif defined(HAVE_GETPROGNAME)
        prog = getprogname();
#else
        const char *substr = NULL;

        for (auto p = prog; *p != 0; p++) {
            if (is_path_separator(*p) && *(p+1) != 0) {
                substr = p + 1;
            }
        }

        if (substr) {
            prog = substr;
        }
#endif

        printf(
            "usage: %s [OPTIONS..]\n"
            "       %s help <option>\n"
            "\n"
            "switches:\n"
            "\n"
            "  help <option>          print information about <option>\n"
            "\n"
            "\n"
            "options:\n"
            "\n",
            prog, prog
        );
    }


    const std::list<getopt_long_cxx::arg_t> args_list =
    {
        { "help", ARG_HELP, NULL,

    /* help */
    "display this information",

    /* more help */
    "Show a brief description of all command line arguments."
        },

        { "full-help", ARG_FULL_HELP, NULL,

    /* help */
    "show more detailed information",

    /* more help */
    "Show more detailed information."
        },

        { "input", ARG_INPUT, "<file>",

    /* help */
    "input file, use `-' to read from STDIN",

    /* more help */
    "Specify an input file. Use `-' to read data from STDIN.\n"
    "This flag is required unless `--help' was passed.\n"
    "\n"
    "The input text must contain all symbols that should be loaded. "
    "They must be listed as modern C-style prototypes, ending on semi-colon (;). "
    "Comments are ignored, line-breaks are treated as spaces. "
    "Any other code will throw an error.\n"
    "\n"
    "Alternatively the input may be an Abstract Syntax Tree (AST) generated by Clang. "
    "To dump the AST created from `foo.h' you can run the following command:\n"
    "clang -Xclang -ast-dump foo.h > ast.txt"
        },

        { "output", ARG_OUTPUT, "<file>",

    /* help */
    "output file (defaults to STDOUT if not set)",

    /* more help */
    "Specify an output file. "
    "If this flag isn't set or if FILE is `-' output will be printed to STDOUT."
        },

        { "name", ARG_NAME, "<string>",

    /* help */
    "use <string> in names of functions, macros and C++ namespaces (default: gdo)",

    /* more help */
    "Use <string> as a prefix in names of functions and macros or as C++ "
    "namespace when generating output to avoid symbol clashes. "
    "The default string is `gdo'. "
    "Upper/lower case and underscores will be set accordingly."
        },

        { "format", ARG_FORMAT, "<fmt>",

    /* help */
    "output format: c (default), c++, minimal, minimal-c++",

    /* more help */
    "Use one of the following output formats:\n"
    "C            -  many features, this is the default\n"
    "C++          -  many features but no exception handling (due to the design)\n"
    "minimal      -  small C header\n"
    "minimal-C++  -  small C++ header with exception handling\n"
    "\n"
    "More information can be found in the comments of the header files."
        },

        { "template", ARG_TEMPLATE, "<file>",

    /* help */
    "use a custom template (`--format' will be ignored)",

    /* more help */
    "Use a custom template file to generate output from. "
    "The flag `--format' will be ignored in this case.\n"
    "\n"
    "Text substitution in the template file is done as following:\n"
    "\n"
    "If a prefix was set with `--name' then any instances of `GDO_' and `gdo_' "
    "will be substituted with it, converted to uppercase and lowercase.\n"
    "\n"
    "Any lines containing one or more of the following will be replaced multiple "
    "times with code from the input (used to generate typedefs, prototyes, etc.):\n"
    "\n"
    "%%type%%: function return type\n"
    "%%func_symbol%%: function symbol name\n"
    "%%args%%: function arguments\n"
    "%%notype_args%%: function argument names without type\n"
    "%%return%%: empty if function doesn't return anything (void), else `return'\n"
    "\n"
    "%%obj_type%%: object type\n"
    "%%obj_symbol%%: object symbol name\n"
    "\n"
    "%%sym_type%%: function or object symbol type\n"
    "%%symbol%: function or object symbol name\n"
    "\n"
    "If a line ends on `@' it will be processed together with the next line as if "
    "there was no line break, but the line break will still appear in the output.\n"
    "\n"
    "All lines between a line `%SKIP_PARAM_UNUSED_BEGIN%' and a line `%SKIP_PARAM_UNUSED_END%' "
    "will be commented out if `--skip-param' was passed. "
    "This will skip code that would otherwise require parameter names.\n"
    "\n"
    "Any line that contains `%DNL%' (\"Do Not Lex\") will be skipped entirely. "
    "This can be used for meta comments. "
    "This is done AFTER glueing lines that end on `@'."
        },

        { "library", ARG_LIBRARY, "[<mode>:]<lib>",

    /* help */
    "set a default library name to load; "
    "if <mode> is 'nq' no quotes are added, "
    "'ext' will append a file extension to the library name and "
    "'api:#' will create a library filename with API number",

    /* more help */
    "Set a default library name to load. "
    "If no mode was set quotation marks are put around the filename as needed.\n"
    "\n"
    "Available modes:\n"
    "nq    - no quotes are added, the string will be used as is\n"
    "ext   - filename extension will be added automatically through a macro\n"
    "api:# - library filename with API number will be created through a macro\n"
    "\n"
    "--library=foo        ==>  \"foo\"\n"
    "--library=nq:foo     ==>  foo\n"
    "--library=ext:foo    ==>  \"foo\" LIBEXTA    ==>  i.e. \"foo.dll\"\n"
    "--library=api:2:foo  ==>  LIBNAMEA(foo,2)  ==>  i.e. \"libfoo.so.2\""
        },

        { "include", ARG_INLCUDE, "[nq:]<file>",

    /* help */
    "include a header file (this flag may be passed multiple times); "
    "nq:<file> - no quotes are added, the string will be used as is",

    /* more help */
    "Set a header file name to be included at the top of the output code. "
    "Quotation marks are put around the filename if it's not enclosed in "
    "brackets or quotation marks or if it's not prefixed with \"nq:\".\n"
    "\n"
    "--include=foo.h      ==>  #include \"foo.h\"\n"
    "--include='\"foo.h\"'  ==>  #include \"foo.h\"\n"
    "--include=<foo.h>    ==>  #include <foo.h>\n"
    "--include=nq:foo     ==>  #include foo\n"
    "\n"
    "This flag may be passed multiple times."
        },

        { "define", ARG_DEFINE, "<string>",

    /* help */
    "define a preprocessor macro to be used (this flag may be passed multiple times)",

    /* more help */
    "Set a preprocessor definition macro to be added at the top of the output code. "
    "This macro may include a value in the form of `FOO=1'.\n"
    "\n"
    "This flag may be passed multiple times."
        },

        { "separate", ARG_SEPARATE, NULL,

    /* help */
    "save output into separate body and header files",

    /* more help */
    "Save output into separate body and header files. "
    "The filename extensions will be set to .c/.h or .cpp/.hpp accordingly. "
    "This flag is ignored if the output is printed to STDOUT or if the "
    "output format is not \"C\" or \"C++\"."
        },

        { "force", ARG_FORCE, NULL,

    /* help */
    "always overwrite existing output files",

    /* more help */
    "Always overwrite existing output files. Use with care."
        },

        { "skip-param", ARG_SKIP_PARAM, NULL,

    /* help */
    "don't look for parameter names in function prototypes",

    /* more help */
    "Don't try to look for parameter names in function prototypes when "
    "the input is being processed. "
    "This will disable any kind of wrapped functions in the output (if "
    "the selected output format makes use of them)."
        },

        { "prefix", ARG_PREFIX, "<string>",

    /* help */
    "look for symbols prefixed with <string>",

    /* more help */
    "Look for symbols that begin with <string> when parsing the input. "
    "This is most useful if the input is a Clang AST to ignore unwanted "
    "declarations coming from i.e. standard C headers."
        },

        { "symbol", ARG_SYMBOL, "<string>",

    /* help */
    "look for symbol name <string> (this flag may be passed multiple times)",

    /* more help */
    "Look for the symbol name <string> when parsing the input. "
    "This is most useful if the input is a Clang AST, "
    "to ignore unwanted declarations coming from i.e. standard C headers.\n"
    "\n"
    "This flag may be passed multiple times."
        },

        { "ast-all-symbols", ARG_AST_ALL_SYM, NULL,

    /* help */
    "use all symbols from a Clang AST",

    /* more help */
    "Pass this flag if you really want to use all symbols found in a Clang AST. "
    "Be careful as this might include unwanted prototypes from other headers. "
    "It's recommended to use `--prefix' and/or `--symbol' instead. "
    "This flag cannot be combined with `--prefix' or `--symbol'.\n"
    "\n"
    "This flag is ignored if the input isn't a Clang AST."
        }
    };


    /* @file */
    const getopt_long_cxx::arg_t atfile =
    {
        NULL, 0, "<file>",

        /* help */
        "read command-line options from <file>",

        /* more help */
        "Read command-line options from <file>. The options read are inserted in place of the original @<file> option. "
        "If <file> does not exist, or cannot be read, then the option will be treated literally, and not removed.\n"
        "\n"
        "Options in <file> are separated by newline. "
        "The file may itself contain additional @<file> options; any such options will be processed recursively."
    };

} /* anonymous namespace */


int main(int argc, char **argv)
{
    /* default settings */
    const char *input = NULL;
    const char *output = "-";
    const char *name = "gdo";
    output::format fmt = output::c;

    /* initialize command line options */
    getopt_long_cxx opt(argc, argv, args_list, atfile);

    try {
        opt.init();

        if (opt.parse_help_switch()) {
            /* "help <opt>" switch was invoked and
             * didn't throw an exception */
            return 0;
        }
    }
    catch (const getopt_long_cxx::error &e) {
        std::cerr << argv[0] << ": error: " << e.what() << std::endl;
        return 1;
    }

    /* initialize class */
    gendlopen gdo(argc, argv);

    int ret, help = 0;

    while (opt.getopt(ret))
    {
        switch (ret)
        {
        case ARG_HELP:
            help = ARG_HELP;
            break;

        case ARG_FULL_HELP:
            help = ARG_FULL_HELP;
            break;

        case ARG_INPUT:
            input = opt.arg();
            break;

        case ARG_OUTPUT:
            output = opt.arg();
            break;

        case ARG_NAME:
            name = opt.arg();
            break;

        case ARG_FORMAT:
            str_to_format_enum(opt.arg(), fmt);
            gdo.format(fmt);
            break;

        case ARG_TEMPLATE:
            gdo.custom_template(opt.arg());
            break;

        case ARG_LIBRARY:
            gdo.default_lib(
                format_libname(opt.arg(), false),
                format_libname(opt.arg(), true)
            );
            break;

        case ARG_INLCUDE:
            gdo.add_inc(format_inc(opt.arg()));
            break;

        case ARG_DEFINE:
            gdo.add_def(format_def(opt.arg()));
            break;

        case ARG_PREFIX:
            gdo.add_pfx(opt.arg());
            break;

        case ARG_SYMBOL:
            gdo.add_sym(opt.arg());
            break;

        case ARG_SEPARATE:
            gdo.separate(true);
            break;

        case ARG_FORCE:
            gdo.force(true);
            break;

        case ARG_SKIP_PARAM:
            gdo.skip_parameter_names(true);
            break;

        case ARG_AST_ALL_SYM:
            gdo.ast_all_symbols(true);
            break;

        case '?':
            std::cerr << "Try '" << argv[0] << " --help' for more information." << std::endl;
            return 1;

        default:
            fprintf(stderr, "error: getopt_long() returned character code 0x%x\n", ret);
            return 1;
        }
    }

    /* print help */
    if (help == ARG_HELP) {
        print_usage(argv[0]);
        opt.print_help();
        return 0;
    } else if (help == ARG_FULL_HELP) {
        print_usage(argv[0]);
        opt.print_full_help();
        return 0;
    }

    /* input is required */
    if (!input) {
        std::cerr << argv[0] << ": option '--input' is required" << std::endl;
        return 1;
    }

    /* generate output */
    try {
        gdo.generate(input, output, name);
    }
    catch (const gendlopen::error &e) {
        std::cerr << argv[0] << ": " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
