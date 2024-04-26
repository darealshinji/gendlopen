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
#include <cstdlib>
#include "gendlopen.hpp"


typedef struct {
    const char *long_opt;
    char short_opt;
    const char *help;
    const char *more_help;
} arg_t;


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

} /* anonymous namespace */


int main(int argc, char **argv)
{
    std::vector<arg_t> help_args;


    /********************* declare command line arguments *********************/

    /* --help */
    arg_t a_help = {
        "help", 'h',

        /* help */
    ////"..########################.#####################################################"
        "  -? -h --help             Display this information",

        /* more help */
        R"(
  -?, -h, --help

    Show a brief description of all command line arguments.)"
    };

    help_args.push_back(a_help);


    /* --full-help */
    arg_t a_full_help = {
        "full-help", 0,

        /* help */
        "  --full-help              Show more detailed information",

        /* more help */
        R"(
  --full-help

    Show more detailed information.)"
    };

    help_args.push_back(a_full_help);


    /* --input */
    arg_t a_input = {
        "input", 'i',

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


    /* --output */
    arg_t a_output = {
        "output", 'o',

        /* help */
        "  -o --output=<file>       Output file (defaults to STDOUT if not set)",

        /* more help */
        R"(
  -o <file>, --output=<file>
    Specify an output file. If this flag isn't set or if FILE is `-' output will
    be printed to STDOUT.)"
    };

    help_args.push_back(a_output);


    /* --name */

    arg_t a_name = {
        "name", 'n',

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


    /* --format */

    arg_t a_format = {
        "format", 'F',

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


    /* --custom-template */

    arg_t a_custom_template = {
        "custom-template", 0,

        /* help */
        "  --custom-template=<file> Use a custom template (`--format' will be ignored)",

        /* more help */
        R"(
  --custom-template=<file>

    Use a custom template file to generate output from. The flag `--format' will
    be ignored in this case.

    Text substitution in the template file is done as following:

    If a prefix was set with `--name' then any instances of `GDO_' and `gdo_'
    will be substituted with it, converted to uppercase and lowercase.

    Any instance of `%%any_symbol%%' will be replaced with a pseudo-randomly
    selected symbol name. This is useful if you need a pointer for `dladdr()'.

    Any lines containing one or more of the following will be replaced multiple
    times with code from the input (used to generate typedefs, prototyes, etc.):

    %%return%%: empty if function doesn't return anything (void), else `return'
    %%type%%: function return type
    %%symbol%%: function symbol name
    %%args%%: function arguments
    %%notype_args%%: function argument names without type
    %%obj_type%%: object type
    %%obj_symbol%%: object symbol name

    If a line ends on `@' it will be processed together with the next line as if
    there was no line break, but the line break will still appear in the output.

    All lines between a line `%SKIP_BEGIN%' and a line `%SKIP_END%' will be
    commented out if `--skip-parameter-names' was passed. This is used to skip
    code that would otherwise require parameter names.

    Any line that contains `%DNL%' ("Do Not Lex") will be skipped entirely.
    This can be used for meta comments. This is done after glueing lines that
    end on `@'.)"
    };

    help_args.push_back(a_custom_template);


    /* --library */

    arg_t a_library = {
        "library", 'l',

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


    /* --include */

    arg_t a_include = {
        "include", 'I',

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


    /* --define */

    arg_t a_define = {
        "define", 'D',

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


    /* --separate */

    arg_t a_separate = {
        "separate", 's',

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


    /* --force */

    arg_t a_force = {
        "force", 'f',

        /* help */
        "  -f --force               Always overwrite existing output files",

        /* more help */
        R"(
  -f, --force

    Always overwrite existing output files. Use with care.)"
    };

    help_args.push_back(a_force);


    /* --skip-parameter-names */

    arg_t a_skip_parameter_names = {
        "skip-parameter-names", 0,

        /* help */
        "  --skip-parameter-names   Don't look for parameter names in function prototypes",

        /* more help */
        R"(
  --skip-parameter-names

    Don't try to look for parameter names in function prototypes when the input
    is being processed. This will disable any kind of wrapped functions in the
    output (if the selected output format makes use of them).)"
    };

    help_args.push_back(a_skip_parameter_names);


    /* --prefix */

    arg_t a_prefix = {
        "prefix", 'P',

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



    /* --symbol */

    arg_t a_symbol = {
        "symbol", 'S',

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


    /* --ast-all-symbols */

    arg_t a_ast_all_symbols = {
        "--ast-all-symbols", 0,

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


    /* parse arguments */
    if (argc > 1 && strcmp(argv[1], "help") == 0) {
        return args::parse_help_switch(argc, argv, help_args);
    }

    /* vectorize arguments and check for --help */
    vstring_t arg_vec = args::parse_args(argc, argv, help_args);

    /* single values (never empty if passed) */
    auto input = args::get_value(arg_vec, a_input);
    auto output = args::get_value(arg_vec, a_output);
    auto name = args::get_value(arg_vec, a_name);
    auto format = args::get_value(arg_vec, a_format);
    auto custom_template = args::get_value(arg_vec, a_custom_template);
    auto library = args::get_value(arg_vec, a_library);

    /* value lists (never empty if passed) */
    auto includes = args::get_value_list(arg_vec, a_include);
    auto defines = args::get_value_list(arg_vec, a_define);
    auto symbols = args::get_value_list(arg_vec, a_symbol);
    auto prefixes = args::get_value_list(arg_vec, a_prefix);

    /* boolean flags */
    bool separate = args::get_flag(arg_vec, a_separate);
    bool force = args::get_flag(arg_vec, a_force);
    bool skip_parameter_names = args::get_flag(arg_vec, a_skip_parameter_names);
    //bool use_param_names = !skip_parameter_names;
    bool ast_all_symbols = args::get_flag(arg_vec, a_ast_all_symbols);

    /* check if anything is left */
    if (arg_vec.size() != 0) {
        if (arg_vec.size() == 1) {
            std::cerr << "error: option not found: " << arg_vec.at(0) << std::endl;
        } else {
            std::cerr << "error: options not found:";

            for (const auto &e : arg_vec) {
                std::cerr << ' ' << e;
            }
            std::cerr << std::endl;
        }

        return 1;
    }

    /* required flags */
    if (input.empty()) {
        std::cerr << "error: `--input' is required" << std::endl;
        return 1;
    }

    /* excluding flags */
    if (ast_all_symbols && (!symbols.empty() || !prefixes.empty())) {
        std::cerr << "error: cannot combine `--ast-all-symbols' with `--symbol' or `--prefix'" << std::endl;
        return 1;
    }

    /* "--format" will be ignored if "--custom-template" is given */
    if (!custom_template.empty()) {
        format.clear();
    }


    /* set options */

    auto gdo = gendlopen(&argc, &argv, input);

    /* --format */
    if (!format.empty()) {
        output::format out = output::c;
        str_to_format_enum(format, out);
        gdo.format(out);
    }

    /* --custom-template */
    if (!custom_template.empty()) {
        gdo.custom_template(custom_template);
    }

    /* --library */
    if (!library.empty()) {
        auto lib_a = format_libname(library, false);
        auto lib_w = format_libname(library, true);
        gdo.default_lib(lib_a, lib_w);
    }

    /* --define */
    for (const auto &e : defines) {
        gdo.add_def(format_def(e));
    }

    /* --include */
    for (const auto &e : includes) {
        gdo.add_inc(format_inc(e));
    }

    /* --prefix */
    for (const auto &e : prefixes) {
        gdo.add_pfx(e);
    }

    /* --symbol */
    for (const auto &e : symbols) {
        gdo.add_sym(e);
    }

    /* other flags */
    gdo.force(force);
    gdo.separate(separate);
    gdo.skip_parameter_names(skip_parameter_names);
    gdo.ast_all_symbols(ast_all_symbols);

    /* generate output */

    if (output.empty()) {
        output = "-";
    }

    if (name.empty()) {
        name = "gdo";
    }

    return gdo.generate(output, name);
}
