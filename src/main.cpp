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

#include <iostream>
#include <regex>
#include <sstream>
#include <vector>
#include <cstdlib>

#ifdef _MSC_VER
#include "getopt.h"
#else
#include <getopt.h>
#endif

#include "gendlopen.hpp"

typedef struct {
    const char *l;     /* long_opt */
    char        s;     /* short_opt */
    const char *help;
    const char *more_help;
} arg_t;


/* anonymous */
namespace
{
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
            std::cerr << "error: Flag 'define' cannot be just white-spaces" << std::endl;
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
				return lib;
			}
			return "L\"" + (lib + "\"");
		}

        if (lib.front() == '"' && lib.back() == '"') {
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
        auto pfx_case = [] (const std::string &str, const std::string &pfx) -> bool {
            return (str.size() > pfx.size() &&
                utils::eq_str_case(str.substr(0, pfx.size()), pfx));
        };

		const char *libext = iswide ? " LIBEXTW" : " LIBEXTA";
		const char *libname = iswide ? "LIBNAMEW(" : "LIBNAMEA(";

        if (pfx_case(str, "nq:")) {
            /* no quotes */
            return str.substr(3);
        } else if (pfx_case(str, "ext:")) {
            /* quotes + extension macro */
            return quote_lib(str.substr(4), iswide) + libext;
        } else if (pfx_case(str, "api:")) {
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
        if (inc.starts_with("nq:")) {
            return inc.substr(3);
        }

        if ((inc.front() == '<' && inc.back() == '>') ||
            (inc.front() == '"' && inc.back() == '"'))
        {
            return inc;
        }

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


    /* print help */
    void print_help(char **&argv, const std::vector<arg_t> &help_args, bool full_help)
    {
        std::cout << "usage: " << argv[0] << " [OPTIONS..]\n";
        std::cout << "       " << argv[0] << " help <option>\n\n";
        std::cout << "Options:\n";
        std::cout << std::endl;

        if (full_help) {
            for (const auto &e : help_args) {
                std::cout << e.more_help << '\n' << std::endl;
            }
        } else {
            for (const auto &e : help_args) {
                std::cout << e.help << std::endl;
            }
            std::cout << std::endl;
        }
    };


    /* parse "help <cmd>" switch */
    int parse_help_switch(int &argc, char **&argv, const std::vector<arg_t> &help_args)
    {
        if (argc != 3) {
            std::cerr << "error: switch `help' expected an option but none was provided" << std::endl;
            return 1;
        }

        bool opt_found = false;
        const char *help_text = NULL;
        const char *arg = argv[2];
        size_t arglen = strlen(arg);

        if (arglen == 2 && arg[0] == '-') {
            /* searching for short opt */
            for (const auto &e : help_args) {
                if (e.s == arg[1]) {
                    help_text = e.more_help;
                    opt_found = true;
                    break;
                }
            }
        } else if (arglen > 3 && arg[0] == '-' && arg[1] == '-') {
            /* searching for long opt */
            for (const auto &e : help_args) {
                if (strcmp(e.l, arg+2) == 0) {
                    help_text = e.more_help;
                    opt_found = true;
                    break;
                }
            }
        }

        if (!opt_found) {
            std::cerr << "error: unknown option: " << arg << std::endl;
            return 1;
        }

        if (help_text) {
            std::cout << "Option `" << arg << "':" << std::endl;
            std::cout << help_text << '\n' << std::endl;
        } else {
            std::cerr << "warning: no entry found for option: " << arg << std::endl;
        }

        return 0;
    }

} /* anonymous namespace */


int main(int argc, char **argv)
{
    std::vector<arg_t> help_args;


    /********************* declare command line arguments *********************/

    enum {
        LONGONLY_FULL_HELP,  /* --full-help */
        LONGONLY_SKIP_PARAM  /* --skip-param */
    };


    /**************************************************************************/
    /* --help */
    constexpr arg_t a_help = {
        /* long */
        "help",

        /* short */
        'h',

        /* help */
    ////"..########################.#####################################################"
        "  -h --help                Display this information",

        /* more help */
        R"(
  -h, --help

    Show a brief description of all command line arguments.)"
    };

    help_args.push_back(a_help);


    /**************************************************************************/
    /* --full-help */
    constexpr arg_t a_full_help =
    {
        /* long */
        "full-help",

        /* short */
        LONGONLY_FULL_HELP,

        /* help */
        "  --full-help              Show more detailed information",

        /* more help */
        R"(
  --full-help

    Show more detailed information.)"
    };

    help_args.push_back(a_full_help);


    /**************************************************************************/
    /* --input */
    constexpr arg_t a_input =
    {
        /* long */
        "input",

        /* short */
        'i',

        /* help */
        "  -i --input=<file>        Input file, use `-' to read from STDIN",

        /* more help */
        R"(
  -i <file>, --input=<file>

    Specify an input file. Use `-' to read data from STDIN.
    This flag is required unless `--help' was passed.

    The input text must contain all symbols that should be loaded. They must be
    listed as modern C-style prototypes, ending on semi-colon (;). Comments are
    ignored, line-breaks are treated as spaces. Any other code will throw an
    error.

    Alternatively the input may be an Abstract Syntax Tree (AST) generated by
    Clang. To dump the AST created from `foo.h' you can run the following
    command: clang -Xclang -ast-dump foo.h > ast.txt)"
    };

    help_args.push_back(a_input);


    /**************************************************************************/
    /* --output */
    constexpr arg_t a_output =
    {
        /* long */
        "output",

        /* short */
        'o',

        /* help */
        "  -o --output=<file>       Output file (defaults to STDOUT if not set)",

        /* more help */
        R"(
  -o <file>, --output=<file>
    Specify an output file. If this flag isn't set or if FILE is `-' output will
    be printed to STDOUT.)"
    };

    help_args.push_back(a_output);


    /**************************************************************************/
    /* --name */
    constexpr arg_t a_name =
    {
        /* long */
        "name",

        /* short */
        'n',

        /* help */
        "  -n --name=<string>       Use STRING in names of functions, macros and C++\n"
        "                           namespaces (default: gdo)",

        /* more help */
        R"(
  -n <string>, --name=<string>

    Use <string> as a prefix in names of functions and macros or as C++
    namespace when generating output to avoid symbol clashes. The default
    string is `gdo'. Upper/lower case and underscores will be set accordingly.)"
    };

    help_args.push_back(a_name);


    /**************************************************************************/
    /* --format */
    constexpr arg_t a_format =
    {
        /* long */
        "format",

        /* short */
        'F',

        /* help */
        "  -F --format=<fmt>        Output format: c (default), c++, minimal, minimal-c++",

        /* more help */
        R"(
  -F <fmt>, --format=<fmt>

    Use one of the following output formats:
    C            -  many features, this is the default
    C++          -  many features but no exception handling (due to the design)
    minimal      -  small C header
    minimal-C++  -  small C++ header with exception handling

    More information can be found in the comments of the header files.)"
    };

    help_args.push_back(a_format);


    /**************************************************************************/
    /* --template */
    constexpr arg_t a_template =
    {
        /* long */
        "template",

        /* short */
        't',

        /* help */
        "  -t --template=<file>     Use a custom template (`--format' will be ignored)",

        /* more help */
        R"(
  -t <file>, --template=<file>

    Use a custom template file to generate output from. The flag `--format' will
    be ignored in this case.

    Text substitution in the template file is done as following:

    If a prefix was set with `--name' then any instances of `GDO_' and `gdo_'
    will be substituted with it, converted to uppercase and lowercase.

    Any lines containing one or more of the following will be replaced multiple
    times with code from the input (used to generate typedefs, prototyes, etc.):

    %%type%%: function return type
    %%func_symbol%%: function symbol name
    %%args%%: function arguments
    %%notype_args%%: function argument names without type
    %%return%%: empty if function doesn't return anything (void), else `return'

    %%obj_type%%: object type
    %%obj_symbol%%: object symbol name

    %%sym_type%%: function or object symbol type
    %%symbol%: function or object symbol name

    If a line ends on `@' it will be processed together with the next line as if
    there was no line break, but the line break will still appear in the output.

    All lines between a line `%SKIP_BEGIN%' and a line `%SKIP_END%' will be
    commented out if `--skip-param' was passed. This is used to skip
    code that would otherwise require parameter names.

    Any line that contains `%DNL%' ("Do Not Lex") will be skipped entirely.
    This can be used for meta comments. This is done AFTER glueing lines that
    end on `@'.)"
    };

    help_args.push_back(a_template);


    /**************************************************************************/
    /* --library */
    constexpr arg_t a_library =
    {
        /* long */
        "library",

        /* short */
        'l',

        /* help */
        "  -l --library=<lib>       Set a default library name to load\n"
        "  -l --library=nq:<lib>    No quotes are added, the string will be used as is\n"
        "  -l --library=ext:<lib>   Filename extension will be added as macro\n"
        "  -l --library=api:#:<lib> Filename with API number will be created as macro",

        /* more help */
        R"(
  -l <lib>,       --library=<lib>
  -l nq:<lib>,    --library=nq:<lib>
  -l ext:<lib>,   --library=ext:<lib>
  -l api:#:<lib>, --library=api:#:<lib>

    Set a default library name to load.
    If no mode was set quotation marks are put around the filename as needed.

    Available modes:
    nq     - no quotes are added, the string will be used as is
    ext    - filename extension will be added automatically through a macro
    api:#  - library filename with API number will be created through a macro

    --library=foo        ==>  "foo"
    --library=nq:foo     ==>  foo
    --library=ext:foo    ==>  "foo" LIBEXTA      ==>    i.e. "foo.dll"
    --library=api:2:foo  ==>  LIBNAMEA(foo,2)    ==>    i.e. "libfoo.so.2")"
    };

    help_args.push_back(a_library);


    /**************************************************************************/
    /* --include */
    constexpr arg_t a_include =
    {
        /* long */
        "include",

        /* short */
        'I',

        /* help */
        "  -I --include=<file>      Include a header file (this flag may be passed\n"
        "                           multiple times\n"
        "  -I --include=nq:<file>   No quotes are added, the string will be used as is",

        /* more help */
        R"(
  -I <file>,    --include=<file>
  -I nq:<file>, --include=nq:<file>

    Set a header file name to be included at the top of the output code.
    Quotation marks are put around the filename if it's not enclosed in brackets
    or quotation marks or if it's not prefixed with "nq:".

    --include=foo.h      ==>  #include "foo.h"
    --include='"foo.h"'  ==>  #include "foo.h"
    --include=<foo.h>    ==>  #include <foo.h>
    --include=nq:foo     ==>  #include foo

    This flag may be passed multiple times.)"
    };

    help_args.push_back(a_include);


    /**************************************************************************/
    /* --define */
    constexpr arg_t a_define =
    {
        /* long */
        "define",

        /* short */
        'D',

        /* help */
        "  -D --define=<string>     Define a preprocessor macro to be used (this flag\n"
        "                           may be passed multiple times)",

        /* more help */
        R"(
  -D <string>, --define=<string>

    Set a preprocessor definition macro to be added at the top of the output
    code. This macro may include a value in the form of `FOO=1'.

    This flag may be passed multiple times.)"
    };

    help_args.push_back(a_define);


    /**************************************************************************/
    /* --separate */
    constexpr arg_t a_separate =
    {
        /* long */
        "separate",

        /* short */
        's',

        /* help */
        "  -s --separate            Save output into separate body and header files",

        /* more help */
        R"(
  -s, --separate

    Save output into separate body and header files. The filename extensions
    will be set to .c/.h or .cpp/.hpp accordingly. This flag is ignored if the
    output is printed to STDOUT or if the output format is not "C" or "C++".)"
    };

    help_args.push_back(a_separate);


    /**************************************************************************/
    /* --force */
    constexpr arg_t a_force =
    {
        /* long */
        "force",

        /* short */
        'f',

        /* help */
        "  -f --force               Always overwrite existing output files",

        /* more help */
        R"(
  -f, --force

    Always overwrite existing output files. Use with care.)"
    };

    help_args.push_back(a_force);


    /**************************************************************************/
    /* --skip-param */
    constexpr arg_t a_skip_param =
    {
        /* long */
        "skip-param",

        /* short */
        LONGONLY_SKIP_PARAM,

        /* help */
        "  --skip-param             Don't look for parameter names in function prototypes",

        /* more help */
        R"(
  --skip-param

    Don't try to look for parameter names in function prototypes when the input
    is being processed. This will disable any kind of wrapped functions in the
    output (if the selected output format makes use of them).)"
    };

    help_args.push_back(a_skip_param);


    /**************************************************************************/
    /* --prefix */
    constexpr arg_t a_prefix =
    {
        /* long */
        "prefix",

        /* short */
        'P',

        /* help */
        "  -P --prefix=<string>     Look for symbols prefixed with <string>",

        /* more help */
        R"(
  -P <string>, --prefix=<string>

    Look for symbols that begin with STRING when parsing the input. This is most
    useful if the input is a Clang AST to ignore unwanted declarations coming
    from i.e. standard C headers.)"
    };

    help_args.push_back(a_prefix);


    /**************************************************************************/
    /* --symbol */
    constexpr arg_t a_symbol =
    {
        /* long */
        "symbol",

        /* short */
        'S',

        /* help */
        "  -S --symbol=<string>     Look for symbol name <string> (this flag may be\n"
        "                           passed multiple times)",

        /* more help */
        R"(
  -S <string>, --symbol=<string>

    Look for the symbol name <string> when parsing the input. This is most
    useful if the input is a Clang AST, to ignore unwanted declarations coming
    from i.e. standard C headers.

    This flag may be passed multiple times.)"
    };

    help_args.push_back(a_symbol);


    /**************************************************************************/
    /* --ast-all-symbols */
    constexpr arg_t a_ast_all_symbols =
    {
        /* long */
        "--ast-all-symbols",

        /* short */
        'a',

        /* help */
        "  --ast-all-symbols        Use all symbols from a Clang AST",

        /* more help */
        R"(
  --ast-all-symbols

    Pass this flag if you really want to use all symbols found in a Clang AST.
    Be careful as this might include unwanted prototypes from other headers.
    It's recommended to use `--prefix' and/or `--symbol' instead. This flag
    cannot be combined with `--prefix' or `--symbol'.

    This flag is ignored if the input isn't a Clang AST.)"
    };

    help_args.push_back(a_ast_all_symbols);


    /**************************************************************************/


    /* parse "help <cmd>" switch */
    if (argc > 1 && strcmp(argv[1], "help") == 0) {
        return parse_help_switch(argc, argv, help_args);
    }


    /**************************************************************************/


#define RQ_ARG(a)  { a.l, required_argument, NULL, a.s }
#define NO_ARG(a)  { a.l, no_argument,       NULL, a.s }

    struct option longopts[] =
    {
        NO_ARG( a_help ),
        NO_ARG( a_full_help ),
        RQ_ARG( a_input ),
        RQ_ARG( a_output ),
        RQ_ARG( a_name ),
        RQ_ARG( a_format ),
        RQ_ARG( a_template ),
        RQ_ARG( a_library ),
        RQ_ARG( a_include ),
        RQ_ARG( a_define ),
        RQ_ARG( a_prefix ),
        RQ_ARG( a_symbol ),
        NO_ARG( a_separate ),
        NO_ARG( a_force ),
        NO_ARG( a_skip_param ),
        NO_ARG( a_ast_all_symbols ),
        { NULL, 0, NULL, 0 }
    };

    /* default settings */
    const char *input = NULL;
    const char *output = "-";
    const char *name = "gdo";
    output::format fmt = output::c;

    /* create optstring from table entries */
    std::string optstring;

    for (int i = 0; longopts[i].name != NULL; i++) {
        if (utils::range<int>(longopts[i].val, 'a', 'z') ||
            utils::range<int>(longopts[i].val, 'A', 'Z'))
        {
            optstring.push_back(longopts[i].val);
        }

        if (longopts[i].has_arg == required_argument) {
            optstring.push_back(':');
        }
    }

    /* initialize class to receive options */
    auto gdo = gendlopen(&argc, &argv);

    int c;
    int longindex = 0;
    std::string lib_a, lib_w;

    while ((c = getopt_long(argc, argv, optstring.c_str(), longopts, &longindex)) != -1)
    {
        switch (c)
        {
        case a_help.s:
            print_help(argv, help_args, false);
            return 0;

        case a_full_help.s:
            print_help(argv, help_args, true);
            return 0;

        case a_input.s:
            input = optarg;
            break;

        case a_output.s:
            output = optarg;
            break;

        case a_name.s:
            name = optarg;
            break;

        case a_format.s:
            str_to_format_enum(optarg, fmt);
            gdo.format(fmt);
            break;

        case a_template.s:
            gdo.custom_template(optarg);
            break;

        case a_library.s:
            lib_a = format_libname(optarg, false);
            lib_w = format_libname(optarg, true);
            gdo.default_lib(lib_a, lib_w);
            break;

        case a_include.s:
            gdo.add_inc(format_inc(optarg));
            break;

        case a_define.s:
            gdo.add_def(format_def(optarg));
            break;

        case a_prefix.s:
            gdo.add_pfx(optarg);
            break;

        case a_symbol.s:
            gdo.add_sym(optarg);
            break;

        case a_separate.s:
            gdo.separate(true);
            break;

        case a_force.s:
            gdo.force(true);
            break;

        case a_skip_param.s:
            gdo.skip_parameter_names(true);
            break;

        case a_ast_all_symbols.s:
            gdo.ast_all_symbols(true);
            break;

        case '?':
            return 1;

        default:
            fprintf(stderr, "error: getopt_long() returned character code 0%o\n", c);
            return 1;
        }
    }

    if (optind < argc) {
        std::cerr << "error: non-option ARGV elements:";

        while (optind < argc) {
            std::cerr << ' ' << argv[optind++];
        }
        std::cerr << std::endl;

        return 1;
    }

    /* input is required */
    if (!input) {
        std::cerr << "error: `--input' is required" << std::endl;
        return 1;
    }

    /* generate output */
    return gdo.generate(input, output, name);
}
