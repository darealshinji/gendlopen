/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2023-2026 Carsten Janssen

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

#ifdef _MSC_VER
# ifndef _CRT_SECURE_NO_WARNINGS
# define _CRT_SECURE_NO_WARNINGS
# endif
#endif

#include <iostream>
#include <string>
#include <cstdlib>
#ifdef _WIN32
# include <windows.h>
#endif
#include "gendlopen.hpp"
#include "utils.hpp"


int main(int argc, char **argv)
{
#if defined(_WIN32) && defined(USE_CP_UTF8)
    SetConsoleCP(CP_UTF8);
    SetConsoleOutputCP(CP_UTF8);
#endif

    try {
        gendlopen gdo;

#ifdef USE_EXTERNAL_RESOURCES
        /* read templates path from environment variable;
         * default path is set in "gendlopen.hpp" */
        char *env = getenv("TEMPLATES");

        if (env && *env) {
            gdo.templates_path(env);
        }
#endif

        /* parse command line and tokenize input file */
        gdo.parse_cmdline(argc, argv);
        gdo.tokenize();

        if (gdo.print_symbols()) { /* -print-symbols */
            gdo.print_symbols_to_stdout();
        } else if (!gdo.custom_template().empty()) { /* -template=.. */
            gdo.process_custom_template();
        } else {
            /* generate output */
            gdo.generate();
        }
    }
    catch (const gendlopen::error_cmd &e) {
        std::cerr << utils::progname(argv[0]) << ": error: " << e.what() << std::endl;
        std::cerr << "Try `" << argv[0] << " -help' for more information." << std::endl;
        return 1;
    }
    catch (const gendlopen::error &e) {
        std::cerr << utils::progname(argv[0]) << ": error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
