/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2023-2025 Carsten Janssen

 Permission is hereby  granted, free of charge, to any  person obtaining a copy
 of this software and associated  documentation files (the "Software"), to deal
 in the Software  without restriction, including without  limitation the rights
 to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
 copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
 IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
 FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
 AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
 LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
**/

#include <iostream>
#include <string>


namespace help
{
    void print(const char *prog)
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
            //"  --help\n"
            "  -help             display this information\n"
            "  -full-help        show more detailed information\n"
            "  -o<file>          save to file instead of stdout\n"
            "  -prefix=<string>  use <string> to prefix functions, macros and C++ namespaces (default: gdo)\n"
            "  -format=<string>  set output format: c (default), c++, plugin, minimal, minimal-c++\n"
            "  -template=<file>  use a custom template (`-format' is ignored)\n"
            "  -library=[<mode>:]<lib>\n"
            "                    set a default library name to load; if <mode> is 'nq' no quotes are\n"
            "                    added, 'ext' will append a file extension to the library name and 'api:#'\n"
            "                    will create a library filename with API number\n"
            "  -include=[nq:]<file>\n"
            "                    include a header file *;\n"
            "                    nq:<file> - no quotes are added, the string will be used as is\n"
            "  -define=<string>\n"
            "  -D<string>        define a preprocessor macro *\n"
            "  -P<string>        look for symbols prefixed with <string> *\n"
            "  -S<string>        look for symbol name <string> *\n"
            "  -separate         save output into separate body and header files\n"
            "  -force            always overwrite existing output files\n"
            "  -param=<mode>     how to handle parameter names in function prototypes from input file;\n"
            "                    modes are: read (default), skip, create\n"
            "  -ast-all-symbols  use all symbols from a Clang AST (`-P' and `-S' are ignored)\n"
            "  -print-symbols    print list of found symbols and exit\n"
            "  -print-lookup     print a C lookup code macro generated from the found symbols and exit\n"
            "  -ignore-options   ignore `%option' lines from input file\n"
            "  -no-date          don't show current date in output\n"
            "  -no-pragma-once   use `#ifndef' header guard instead of `#pragma once'\n"
            "  -line             add `#line' directives to output\n"
            "  -dump-templates   dump internal template files in the current working directory and exit\n"
            "\n"
            "  * option may be passed multiple times" << std::endl;
    }


    void print_full(const char *prog)
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
            "    They must be listed as C-style prototypes, ending on semi-colon (;).\n"
            "    Comments are ignored, line-breaks are treated as spaces.\n"
            "    Any other code will throw an error.\n"
            "\n"
            "    Some options can be set on a line beginning with `%option':\n"
            "    %option format=<string> prefix=<string> library=[<mode>:]<lib>\n"
            "    %option include=[nq:]<file> define=<string> param=[skip|create|read]\n"
            "    %option no-date no-pragma-once line\n"
            "\n"
            "    See the corresponding command line options for details.\n"
            "\n"
            "    Alternatively the input may be an Abstract Syntax Tree (AST) generated by Clang.\n"
            "    To dump the AST created from `foo.h' you can run the following command:\n"
            "    clang -Xclang -ast-dump foo.h > ast.txt\n"
            "\n"


            "Options:\n"
            "\n"

            //"  --help\n"
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


            "  -prefix=<string>\n"
            "    Use <string> as a prefix in names of functions and macros or as C++\n"
            "    namespace when generating output to avoid symbol clashes.\n"
            "    The default string is `gdo'.\n"
            "    Upper/lower case and underscores will be set accordingly.\n"
            "\n"


            "  -format=<string>\n"
            "    Use one of the following output formats:\n"
            "    C            -  many features, this is the default\n"
            "    C++          -  many features but no exception handling (due to the design)\n"
            "    plugin       -  intended to write a plugin loader\n"
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
            "    %%func_symbol_pad%%: function symbol name with padding spaces\n"
            "    %%args%%: function arguments\n"
            "    %%notype_args%%: function argument names without type\n"
            "    %%return%%: empty if function doesn't return anything (void), else `return'\n"
            "\n"
            "    %%obj_type%%: object type\n"
            "    %%obj_symbol%%: object symbol name\n"
            "    %%obj_symbol_pad%%: object symbol name with padding spaces\n"
            "\n"
            "    %%sym_type%%: function or object symbol type\n"
            "    %%symbol%%: function or object symbol name\n"
            "\n"
            "    If a line ends on `@' it will be processed together with the next line as if\n"
            "    there was no line break, but the line break will still appear in the output.\n"
            "\n"
            "    If `-param=skip' was passed all lines after a `%PARAM_SKIP_REMOVE_BEGIN%' line\n"
            "    won't be used, while code after a `%PARAM_SKIP_USE_BEGIN%' line will be.\n"
            "    It's the other way around if `-param=skip' was not passed.\n"
            "    A line `%PARAM_SKIP_END%' will reset everything to default. This is used to\n"
            "    skip code that would otherwise require parameter names.\n"
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


            "  -define=<string>\n"
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


            "  -param=<mode>\n"
            "    If <mode> is 'read':\n"
            "    Always try to read parameter names in function prototypes when\n"
            "    the input is being processed. This is the default behavior if `-param'\n"
            "    was not explicitly set.\n"
            "\n"
            "    If <mode> is 'skip':\n"
            "    Don't try to look for parameter names in function prototypes. This will\n"
            "    disable any kind of wrapped functions in the output.\n"
            "\n"
            "    If <mode> is 'create':\n"
            "    Create parameter names for the output to be used in wrapped functions.\n"
            "    Function prototypes from the input are assumed to have no parameter names!\n"
            "\n"


            "  -ast-all-symbols\n"
            "    Pass this flag if you really want to use all symbols found in a Clang AST.\n"
            "    Be careful as this might include unwanted prototypes from other headers.\n"
            "    It's recommended to use `-P' and/or `-S' instead.\n"
            "    If the input is a Clang AST and this flag was set then `-P' and `-S' are\n"
            "    ignored.\n"
            "\n"
            "    This flag is ignored if the input is not a Clang AST.\n"
            "\n"


            "  -print-symbols\n"
            "    Don't create any output, just print a list of found symbols and exit.\n"
            "    This is useful for debugging.\n"
            "\n"


            "  -print-lookup\n"
            "    Print a C lookup code macro generated from the found symbols and exit.\n"
            "\n"


            "  -ignore-options\n"
            "    Ignore lines beginning with `%option' from the input file.\n"
            "\n"


            "  -no-date\n"
            "    Don't show the current date in output. Useful for reproducable builds.\n"
            "\n"


            "  -no-pragma-once\n"
            "    Don't add `#pragma once' to output header file. Instead use the classic\n"
            "    `#ifndef' preprocessor header guard.\n"
            "\n"


            "  -line\n"
            "    Add `#line' directives to the output that will refer to the original template\n"
            "    files.\n"
            "\n"


            "  -dump-templates\n"
            "    Dump internal template files in the current working directory and exit.\n"
            "\n"

            << std::endl;
    }

} /* namespace help */
