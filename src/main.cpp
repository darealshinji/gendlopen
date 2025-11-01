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
#include <cstdlib>
#include "gendlopen.hpp"
#include "get_args.hpp"
#include "utils.hpp"


#ifdef USE_EXTERNAL_RESOURCES

namespace /* anonymous */
{
    std::string get_templates_path_env()
    {
        std::string path;

#ifdef _MSC_VER
        char *buf;
        size_t len;

        /* MSVC doesn't like getenv() */
        if (_dupenv_s(&buf, &len, TEMPLATES_ENV) == 0) {
            if (buf && *buf) {
                path = buf;
            }

            free(buf);
        }
#else
        char *env = getenv(TEMPLATES_ENV);

        if (env && *env) {
            path = env;
        }
#endif /* !_MSC_VER */

        if (!path.empty()) {
            utils::append_missing_separator(path);
        }

        return path;
    }
} /* end anonymous namespace */

#endif /* USE_EXTERNAL_RESOURCES */



int main(int argc, char **argv)
{
    try {
        gendlopen gdo;

#ifdef USE_EXTERNAL_RESOURCES
        /* read templates path from environment variable */
        std::string env = get_templates_path_env();

        if (!env.empty()) {
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
    catch (const get_args::error &e) {
        /* command line error */
        std::cerr << utils::progname(argv[0]) << ": error: " << e.what() << std::endl;
        std::cerr << "Try `" << argv[0] << " -help' for more information." << std::endl;
        return 1;
    }
    catch (const gendlopen::error &e) {
        /* other error */
        std::cerr << utils::progname(argv[0]) << ": error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
