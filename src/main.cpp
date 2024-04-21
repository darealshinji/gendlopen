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
#include "args.hxx"
#include "gendlopen.hpp"

/* ./src/gendlopen --more-help | wc -c  (minus a few characters from the first line) */
#define HELP_TEXT_RESERVE_LENGTH 4850


using StrValue = args::ValueFlag<std::string>;
using StrList = args::ValueFlagList<std::string>;
using Opt = args::Options;
using args::ArgumentParser;
using args::HelpFlag;
using args::Flag;


/* anonymous */
namespace
{
    template<class T=StrList>
    void check_empty(const std::string &val, T &flag)
    {
        if (val.empty()) {
            std::string arg = flag.GetMatcher().GetLongOrAny().str(/* "-", "--" */);
            std::cerr << "error: Flag '" << arg << "' requires a non-empty argument" << std::endl;
            std::exit(1);
        }
    }

    std::string getstr(StrValue &flag)
    {
        std::string val = flag.Get();
        check_empty<StrValue>(val, flag);
        return val;
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
            std::cerr << "error: Flag 'define' requires a non-empty value" << std::endl;
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
    std::string quote_inc(const std::string &inc)
    {
        if ((inc.front() == '<' && inc.back() == '>') ||
            (inc.front() == '"' && inc.back() == '"'))
        {
            return inc;
        }

        return "\"" + (inc + "\"");
    }


    /* return the correct enum or error */
    output::format str_to_enum(const std::string &in)
    {
        std::string fmt = utils::convert_to_lower(in, false);

        switch (fmt.front())
        {
        case 'c':
            if (fmt == "c") {
                return output::c;
            } else if (fmt == "cxx" || fmt == "c++" || fmt == "cpp") {
                return output::cxx;
            }
            break;
        case 'm':
            if (fmt.starts_with("minimal")) {
                if (fmt == "minimal" || fmt == "minimal-c") {
                    return output::minimal;
                } else if (fmt == "minimal-cxx" || fmt == "minimal-c++" ||
                    fmt == "minimal-cpp")
                {
                    return output::minimal_cxx;
                }
            }
            break;
        default:
            break;
        }

        return output::error;
    }

} /* anonymous namespace */


int main(int argc, char **argv)
{
    auto print_info = [&] () {
        std::cerr << "Try `" << argv[0] << " --help' for more information." << std::endl;
    };

    std::string more_help;

    more_help.reserve(HELP_TEXT_RESERVE_LENGTH);

    ArgumentParser args("Tool to generate library loading code");

//  more_help += "<<--------------------------------- 80 chars --------------------------------->>" "\n";
    more_help +=                                                                                    "\n";
    more_help += "  Gendlopen is a small tool intended to help with the creation of code that"      "\n";
    more_help += "  dynamically loads shared libraries. It takes text files with C prototype"       "\n";
    more_help += "  declarations as input and creates C or C++ header files as output."             "\n";
    more_help +=                                                                                    "\n";
    more_help += "  The output is compatible with POSIX dlopen() and win32 LoadLibrary() API."      "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";
    more_help += "OPTIONS:"                                                                         "\n";
    more_help +=                                                                                    "\n";


    /* --help */

    HelpFlag a_help(args, {}, "show this help", {'h', "help"});
    HelpFlag a_help2(args, {}, {}, {'?'}, Opt::Hidden);

    more_help +=                                                                                    "\n";
    more_help += "  -h, --help"                                                                     "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Print a summary of all avaiable options with a brief description."            "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --more-help */

    HelpFlag a_more_help(args, {},
        "show full help with additional info",
        {"more-help"});

    more_help +=                                                                                    "\n";
    more_help += "  --more-help"                                                                    "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Show this more detailed information."                                         "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --input */

    StrValue a_input(args, "FILE",
        "input file",
        {'i', "input"},
        Opt::Single | Opt::Required);

    more_help +=                                                                                    "\n";
    more_help += "  -i[FILE], --input=[FILE]"                                                       "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Specify an input file. Use `-' to read data from STDIN."                      "\n";
    more_help += "    This flag is required unless `--help' or `--more-help' was passed."           "\n";
    more_help +=                                                                                    "\n";
    more_help += "    The input text must contain all symbols that should be loaded. They must be"  "\n";
    more_help += "    listed as modern C-style prototypes, ending on semi-colon (;). Comments are"  "\n";
    more_help += "    ignored, line-breaks are treated as spaces. Any other code will throw an"     "\n";
    more_help += "    error."                                                                       "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Alternatively the input may be an Abstract Syntax Tree (AST) generated by"    "\n";
    more_help += "    Clang."                                                                       "\n";
    more_help += "    To dump the AST created from `foo.h' you can run the following command:"      "\n";
    more_help += "    clang -Xclang -ast-dump foo.h > ast.txt"                                      "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --output */

    StrValue a_output(args, "FILE",
        "output file",
        {'o', "output"},
        "-", Opt::Single);

    more_help +=                                                                                    "\n";
    more_help += "  -o[FILE], --output=[FILE]"                                                      "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Specify an output file. If this flag isn't set or if FILE is `-' output will" "\n";
    more_help += "    be printed to STDOUT."                                                        "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --name */

    StrValue a_name(args, "STRING",
        "use STRING in names of functions, macros and namespaces (default: gdo)",
        {'n', "name"},
        "gdo", Opt::Single);

    more_help +=                                                                                    "\n";
    more_help += "  -n[STRING], --name=[STRING]"                                                    "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Use STRING as a prefix in names of functions and macros or as C++ namespace"  "\n";
    more_help += "    when generating output to avoid symbol clashes. The default string is `gdo'." "\n";
    more_help += "    Upper/lower case and underscores will be set accordingly."                    "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --format */

    StrValue a_format(args, "STRING",
        "output format: c (default), c++, minimal, minimal-c++",
        {'F', "format"},
        Opt::Single);

    more_help +=                                                                                    "\n";
    more_help += "  -F[STRING], --format=[STRING]"                                                  "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Use one of the following output formats:"                                     "\n";
    more_help += "    C            -  many features, this is the default"                           "\n";
    more_help += "    C++          -  many features but no exception handling (due to the design)"  "\n";
    more_help += "    minimal      -  small C header"                                               "\n";
    more_help += "    minimal-C++  -  small C++ header with exception handling"                     "\n";
    more_help +=                                                                                    "\n";
    more_help += "    More information can be found in the comments of the header files."           "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --custom-template */

    StrValue a_custom_template(args, "FILE",
        "use a custom template (`--format' will be ignored)",
        {"custom-template"},
        Opt::Single);

    more_help +=                                                                                    "\n";
    more_help += "  --custom-template=[FILE]"                                                       "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Use a custom template file to generate output from. The flag `--format' will" "\n";
    more_help += "    be ignored in this case."                                                     "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Text substitution in the template file is done as following:"                 "\n";
    more_help +=                                                                                    "\n";
    // maybe only replace if not encapsulated by [A-Za-z] characters?
    // what if a symbol has a name like "sendingdoitalready"?
    more_help += "    If a prefix was set with `--name' then any instances of `GDO' and `gdo' will" "\n";
    more_help += "    be substituted with it, converted to uppercase and lowercase."                "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Any lines containing one or more of the following will be replaced multiple"  "\n";
    more_help += "    times with code from the input (used to generate typedefs, prototyes, etc.):" "\n";
    more_help +=                                                                                    "\n";
    more_help += "    %%return%%: empty if function doesn't return anything (void), else `return'"  "\n";
    more_help += "    %%type%%: function return type"                                               "\n";
    more_help += "    %%symbol%%: function symbol name"                                             "\n";
    more_help += "    %%args%%: function arguments"                                                 "\n";
    more_help += "    %%notype_args%%: function argument names without type"                        "\n";
    more_help += "    %%obj_type%%: object type"                                                    "\n";
    more_help += "    %%obj_symbol%%: object symbol name"                                           "\n";
    more_help +=                                                                                    "\n";
    more_help += "    If a line ends on `@' it will be processed together with the next line as if" "\n";
    more_help += "    there was no line break, but the line break will still appear in the output." "\n";
//  more_help += "    This is similar to a preprocessor line that ends on a backslash."             "\n";
    more_help +=                                                                                    "\n";
    more_help += "    All lines between a line `%%SKIP_BEGIN%%' and a line `%%SKIP_END%%' will be"  "\n";
    more_help += "    commented out if `--skip-parameter-names' was passed. This is used to skip"   "\n";
    more_help += "    code that would otherwise require parameter names. "                          "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --library */

    StrValue a_library(args, "[MODE:]STRING",
        "default library to load; modes: nq ext api:#",
        {'l', "library"},
        Opt::Single);

    more_help +=                                                                                    "\n";
    more_help += "  -l[[MODE:]STRING], --library=[[MODE:]STRING]"                                   "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Set a default library name to load."                                          "\n";
    more_help += "    If no mode was set quotation marks are put around the filename as needed."    "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Available modes:"                                                             "\n";
    more_help += "    nq     - no quotes are added, the string will be used as is"                  "\n";
    more_help += "    ext    - filename extension will be added through a macro"                    "\n";
    more_help += "    api:#  - library filename with API number will be created through a macro"    "\n";
    more_help +=                                                                                    "\n";
    more_help += "    --library=foo        ==>  \"foo\""                                            "\n";
    more_help += "    --library=nq:foo     ==>  foo"                                                "\n";
    more_help += "    --library=ext:foo    ==>  \"foo\" LIBEXTA    ==>    i.e. \"foo.dll\""         "\n";
    more_help += "    --library=api:2:foo  ==>  LIBNAMEA(foo,2)    ==>    i.e. \"libfoo.so.2\""     "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --include */

    StrList a_include(args, "STRING",
        "header to include",
        {'I', "include"});

    more_help +=                                                                                    "\n";
    more_help += "  -I[STRING], --include=[STRING]"                                                 "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Set a header file name to be included at the top of the output code."         "\n";
    more_help += "    Quotation marks are put around the filename if it's not enclosed in brackets" "\n";
    more_help += "    or quotation marks. This flag may be passed multiple times."                  "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --include-nq */

    StrList a_include_nq(args, "STRING",
        "header to include (no quotes added)",
        {"include-nq"});

    more_help +=                                                                                    "\n";
    more_help += "  --include-nq=[STRING]"                                                          "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Set a header file name to be included at the top of the output code."         "\n";
    more_help += "    Quotation marks are never added. This is useful if you want to specify a"     "\n";
    more_help += "    header through a macro. This flag may be passed multiple times."              "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --define */

    StrList a_define(args, "STRING",
        "define preprocessor macro",
        {'D', "define"});

    more_help +=                                                                                    "\n";
    more_help += "  -D[STRING], --define=[STRING]"                                                  "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Set a preprocessor definition macro to be added at the top of the output"     "\n";
    more_help += "    code. This macro may include a value in the form of `FOO=1'. This flag may"   "\n";
    more_help += "    be passed multiple times."                                                    "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --separate */

    Flag a_separate(args, {},
        "save into header and body files",
        {'s', "separate"});

    more_help +=                                                                                    "\n";
    more_help += "  -s, --separate"                                                                 "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Save output into separate body and header files. The filename extensions"     "\n";
    more_help += "    will be set to .c/.h or .cpp/.hpp accordingly. This flag is ignored if the"   "\n";
    more_help += "    output is printed to STDOUT. Currently this is only supported if the output"  "\n";
    more_help += "    format is `C'. On other formats this flag is ignored."                        "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --force */

    Flag a_force(args, {},
        "overwrite existing files",
        {'f', "force"});

    more_help +=                                                                                    "\n";
    more_help += "  -f, --force"                                                                    "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Always overwrite existing output files. Use with care."                       "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --skip-parameter-names */

    Flag a_skip_parameter_names(args, {},
        "skip parameter name lookup in function prototypes",
        {"skip-parameter-names"});

    more_help +=                                                                                    "\n";
    more_help += "  --skip-parameter-names"                                                         "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Don't try to look for parameter names in function prototypes when the input"  "\n";
    more_help += "    is being processed. This will disable any kind of wrapped functions in the"   "\n";
    more_help += "    output (if the selected output format makes use of them)."                    "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --prefix */

    StrList a_prefix(args, "STRING",
        "look for symbols prefixed with STRING",
        {'P', "prefix"});

    more_help +=                                                                                    "\n";
    more_help += "  -P[STRING...], --prefix=[STRING]"                                               "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Look for symbols that begin with STRING when parsing the input. This is most" "\n";
    more_help += "    useful if the input is a Clang AST to ignore unwanted declarations coming"    "\n";
    more_help += "    from i.e. standard C headers."                                                "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --symbol */

    StrList a_symbol(args, "STRING",
        "look for symbol STRING",
        {'S', "symbol"});

    more_help +=                                                                                    "\n";
    more_help += "  -S[STRING], --symbol=[STRING]"                                                  "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Look for the symbol name STRING when parsing the input. This is most useful"  "\n";
    more_help += "    useful if the input is a Clang AST to ignore unwanted declarations coming"    "\n";
    more_help += "    from i.e. standard C headers. This flag may be passed multiple times."        "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* --ast-all-symbols */

    Flag a_ast_all_symbols(args, {},
        "use all symbols from a Clang AST",
        {"ast-all-symbols"});

    more_help +=                                                                                    "\n";
    more_help += "  --ast-all-symbols"                                                              "\n";
    more_help +=                                                                                    "\n";
    more_help += "    Pass this flag if you really want to use all symbols found in a Clang AST."   "\n";
    more_help += "    Be careful, as this might include unwanted prototypes from other headers."    "\n";
    more_help += "    It's recommended to use `--prefix' and/or `--symbol' instead. This flag "     "\n";
    more_help += "    cannot be combined with `--prefix' or `--symbol'."                            "\n";
    more_help +=                                                                                    "\n";
    more_help += "    This flag has no effect if the input isn't a Clang AST."                      "\n";
    more_help +=                                                                                    "\n";
    more_help +=                                                                                    "\n";


    /* parse arguments */
    try {
        args.ParseCLI(argc, argv);
    }
    catch (const args::Help&) {
        if (a_more_help) {
            std::cout << "  " << *argv << " {OPTIONS}\n\n";
            std::cout << more_help << std::flush;
        } else {
            std::cout << args << std::flush;
        }
        return 0;
    }
    catch (const args::Error &e) {
        std::cerr << "error: " << e.what() << std::endl;
        print_info();
        return 1;
    }
    catch (...) {
        std::cerr << "an unknown error has occurred" << std::endl;
        return 1;
    }


    /* check excluding flags */
    if (a_ast_all_symbols && (a_symbol || a_prefix)) {
        std::cerr << "error: cannot combine `--ast-all-symbols' with `--symbol' or `--prefix'" << std::endl;
        print_info();
        return 1;
    }


    auto gdo = gendlopen(&argc, &argv);

    /* --input (flagged as required) */
    gdo.input(getstr(a_input));

    /* --format */
    if (a_format && !a_custom_template) {
        auto fmt = str_to_enum(getstr(a_format));

        if (fmt == output::error) {
            std::cerr << "error: unknown output format given: " << a_format.Get() << std::endl;
            print_info();
            return 1;
        }

        gdo.format(fmt);
    }

    /* --custom-template */
    if (a_custom_template) {
        gdo.custom_template(getstr(a_custom_template));
    }

    /* --library */
    if (a_library) {
        auto s = getstr(a_library);
        auto lib_a = format_libname(s, false);
        auto lib_w = format_libname(s, true);
        gdo.default_lib(lib_a, lib_w);
    }

    /* --define */
    for (const auto &e : args::get(a_define)) {
        check_empty(e, a_define);
        gdo.add_def(format_def(e));
    }

    /* --include */
    for (const auto &e : args::get(a_include)) {
        check_empty(e, a_include);
        gdo.add_inc(quote_inc(e));
    }

    /* --include-no-quotes */
    for (const auto &e : args::get(a_include_nq)) {
        check_empty(e, a_include_nq);
        gdo.add_inc(e);
    }

    /* --prefix */
    for (const auto &e : args::get(a_prefix)) {
        check_empty(e, a_prefix);
        gdo.add_pfx(e);
    }

    /* --symbol */
    for (const auto &e : args::get(a_symbol)) {
        check_empty(e, a_symbol);
        gdo.add_sym(e);
    }

    /* other flags */
    gdo.force(a_force);
    gdo.separate(a_separate);
    gdo.skip_parameter_names(a_skip_parameter_names);
    gdo.ast_all_symbols(a_ast_all_symbols);

    /* generate output */
    return gdo.generate(getstr(a_output), getstr(a_name));
}
