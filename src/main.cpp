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
#include <stdlib.h>

#include "gendlopen.hpp"
#include "getopt_long_cxx.hpp"


/* short option values */
enum {
    ARG_HELP        = 'h',
    ARG_FULL_HELP   = 'H',
    ARG_INPUT       = 'i',
    ARG_OUTPUT      = 'o',
    ARG_NAME        = 'n',
    ARG_FORMAT      = 'F',
    ARG_TEMPLATE    = 't',
    ARG_LIBRARY     = 'l',
    ARG_INLCUDE     = 'I',
    ARG_DEFINE      = 'D',
    ARG_PREFIX      = 'P',
    ARG_SYMBOL      = 'S',
    ARG_SEPARATE    = 's',
    ARG_FORCE       = 'f',
    ARG_SKIP_PARAM  = 'x',
    ARG_AST_ALL_SYM = 'a'
};


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


    /* append our command line options */
    void add_options(getopt_long_cxx &opt)
    {

        opt.add_flag("help", ARG_HELP,

            /* help */
            "  -h --help              display this information",

            /* more help */
            R"(
  -h, --help

    Show a brief description of all command line arguments.)");


        opt.add_flag("full-help", ARG_FULL_HELP,

            /* help */
            "  -H --full-help         show more detailed information",

            /* more help */
            R"(
  -H, --full-help

    Show more detailed information.)");


        opt.add_arg("input", ARG_INPUT,

            /* help */
            "  -i --input=<file>      input file, use `-' to read from STDIN",

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
    command: clang -Xclang -ast-dump foo.h > ast.txt)");


        opt.add_arg("output", ARG_OUTPUT,

            /* help */
            "  -o --output=<file>     output file (defaults to STDOUT if not set)",

            /* more help */
            R"(
  -o <file>, --output=<file>

    Specify an output file. If this flag isn't set or if FILE is `-' output will
    be printed to STDOUT.)");


        opt.add_arg("name", ARG_NAME,

            /* help */
            "  -n --name=<string>     use <string> in names of functions, macros and C++\n"
            "                         namespaces (default: gdo)",

            /* more help */
            R"(
  -n <string>, --name=<string>

    Use <string> as a prefix in names of functions and macros or as C++
    namespace when generating output to avoid symbol clashes. The default
    string is `gdo'. Upper/lower case and underscores will be set accordingly.)");


        opt.add_arg("format", ARG_FORMAT,

            /* help */
            "  -F --format=<fmt>      output format: c (default), c++, minimal, minimal-c++",

            /* more help */
            R"(
  -F <fmt>, --format=<fmt>

    Use one of the following output formats:
    C            -  many features, this is the default
    C++          -  many features but no exception handling (due to the design)
    minimal      -  small C header
    minimal-C++  -  small C++ header with exception handling

    More information can be found in the comments of the header files.)");


        opt.add_arg("template", ARG_TEMPLATE,

            /* help */
            "  -t --template=<file>   use a custom template (`--format' will be ignored)",

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

    All lines between a line `%SKIP_PARAM_UNUSED_BEGIN%' and a line
    `%SKIP_PARAM_UNUSED_END%' will be commented out if `--skip-param' was
    passed. This will skip code that would otherwise require parameter names.

    Any line that contains `%DNL%' ("Do Not Lex") will be skipped entirely.
    This can be used for meta comments. This is done AFTER glueing lines that
    end on `@'.)");


        opt.add_arg("library", ARG_LIBRARY,

            /* help */
            "  -l --library=[<mode>:]<lib>  set a default library name to load; if <mode> is\n"
            "                         'nq' no quotes are added, 'ext' will append a file\n"
            "                         extension to the library name and 'api:#' will create\n"
            "                         a library filename with API number",

            /* more help */
            R"(
  -l <lib>,       --library=<lib>
  -l nq:<lib>,    --library=nq:<lib>
  -l ext:<lib>,   --library=ext:<lib>
  -l api:#:<lib>, --library=api:#:<lib>

    Set a default library name to load.
    If no mode was set quotation marks are put around the filename as needed.

    Available modes:
    nq    - no quotes are added, the string will be used as is
    ext   - filename extension will be added automatically through a macro
    api:# - library filename with API number will be created through a macro

    --library=foo        ==>  "foo"
    --library=nq:foo     ==>  foo
    --library=ext:foo    ==>  "foo" LIBEXTA    ==>  i.e. "foo.dll"
    --library=api:2:foo  ==>  LIBNAMEA(foo,2)  ==>  i.e. "libfoo.so.2")");


        opt.add_arg("include", ARG_INLCUDE,

            /* help */
            "  -I --include=[nq:]<file>  include a header file (this flag may be passed\n"
            "                         multiple times); nq:<file> - no quotes are added, the\n"
            "                         string will be used as is",

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

    This flag may be passed multiple times.)");


        opt.add_arg("define", ARG_DEFINE,

            /* help */
            "  -D --define=<string>   define a preprocessor macro to be used (this flag may\n"
            "                         be passed multiple times)",

            /* more help */
            R"(
  -D <string>, --define=<string>

    Set a preprocessor definition macro to be added at the top of the output
    code. This macro may include a value in the form of `FOO=1'.

    This flag may be passed multiple times.)");


        opt.add_flag("separate", ARG_SEPARATE,

            /* help */
            "  -s --separate          save output into separate body and header files",

            /* more help */
            R"(
  -s, --separate

    Save output into separate body and header files. The filename extensions
    will be set to .c/.h or .cpp/.hpp accordingly. This flag is ignored if the
    output is printed to STDOUT or if the output format is not "C" or "C++".)");


        opt.add_flag("force", ARG_FORCE,

            /* help */
            "  -f --force             always overwrite existing output files",

            /* more help */
            R"(
  -f, --force

    Always overwrite existing output files. Use with care.)");


        opt.add_flag("skip-param", ARG_SKIP_PARAM,

            /* help */
            "  -x --skip-param        don't look for parameter names in function prototypes",

            /* more help */
            R"(
  -x, --skip-param

    Don't try to look for parameter names in function prototypes when the input
    is being processed. This will disable any kind of wrapped functions in the
    output (if the selected output format makes use of them).)");


        opt.add_arg("prefix", ARG_PREFIX,

            /* help */
            "  -P --prefix=<string>   look for symbols prefixed with <string>",

            /* more help */
            R"(
  -P <string>, --prefix=<string>

    Look for symbols that begin with <string> when parsing the input. This is
    most useful if the input is a Clang AST to ignore unwanted declarations
    coming from i.e. standard C headers.)");


        opt.add_arg("symbol", ARG_SYMBOL,

            /* help */
            "  -S --symbol=<string>   look for symbol name <string> (this flag may be passed\n"
            "                         multiple times)",

            /* more help */
            R"(
  -S <string>, --symbol=<string>

    Look for the symbol name <string> when parsing the input. This is most
    useful if the input is a Clang AST, to ignore unwanted declarations coming
    from i.e. standard C headers.

    This flag may be passed multiple times.)");


        opt.add_flag("ast-all-symbols", ARG_AST_ALL_SYM,

            /* help */
            "  -a --ast-all-symbols   use all symbols from a Clang AST",

            /* more help */
            R"(
  -a, --ast-all-symbols

    Pass this flag if you really want to use all symbols found in a Clang AST.
    Be careful as this might include unwanted prototypes from other headers.
    It's recommended to use `--prefix' and/or `--symbol' instead. This flag
    cannot be combined with `--prefix' or `--symbol'.

    This flag is ignored if the input isn't a Clang AST.)");

    }

} /* anonymous namespace */


int main(int argc, char **argv)
{
    /* default settings */
    const char *input = NULL;
    const char *output = "-";
    const char *name = "gdo";
    output::format fmt = output::c;

    int ret;
    std::string lib_a, lib_w;

    /* initialize command line options */
    getopt_long_cxx opt(argc, argv);

    try {
        add_options(opt);

        if (opt.parse_help_switch()) {
            /* "help <opt>" switch was invoked and
             * didn't throw an exception */
            return 0;
        }
    }
    catch (const getopt_long_cxx::error &e) {
        std::cerr << argv[0] << ": " << e.what() << std::endl;
        return 1;
    }

    /* initialize class */
    gendlopen gdo(argc, argv);

    while (opt.getopt(ret))
    {
        switch (ret)
        {
        case ARG_HELP:
            opt.print_help();
            return 0;

        case ARG_FULL_HELP:
            opt.print_full_help();
            return 0;

        case ARG_INPUT:
            input = opt.get_optarg();
            break;

        case ARG_OUTPUT:
            output = opt.get_optarg();
            break;

        case ARG_NAME:
            name = opt.get_optarg();
            break;

        case ARG_FORMAT:
            str_to_format_enum(opt.get_optarg(), fmt);
            gdo.format(fmt);
            break;

        case ARG_TEMPLATE:
            gdo.custom_template(opt.get_optarg());
            break;

        case ARG_LIBRARY:
            lib_a = format_libname(opt.get_optarg(), false);
            lib_w = format_libname(opt.get_optarg(), true);
            gdo.default_lib(lib_a, lib_w);
            break;

        case ARG_INLCUDE:
            gdo.add_inc(format_inc(opt.get_optarg()));
            break;

        case ARG_DEFINE:
            gdo.add_def(format_def(opt.get_optarg()));
            break;

        case ARG_PREFIX:
            gdo.add_pfx(opt.get_optarg());
            break;

        case ARG_SYMBOL:
            gdo.add_sym(opt.get_optarg());
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
            /* getopt_long() error */
            return 1;

        default:
            fprintf(stderr, "error: getopt_long() returned character code 0x%x\n", ret);
            return 1;
        }
    }

    /* input is required */
    if (!input) {
        std::cerr << argv[0] << ": option '--input' is required" << std::endl;
        return 1;
    }

    /* generate output */
    return gdo.generate(input, output, name);
}
