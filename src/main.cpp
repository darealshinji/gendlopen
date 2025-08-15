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

#include <errno.h>  /* program_invocation_short_name */
#include <string.h>
#include <cstdlib>
#include <iostream>
#include <string>
#include "gendlopen.hpp"
#include "parse_args.hpp"

#ifdef _WIN32
# define PATH_SEPARATOR '\\'
#else
# define PATH_SEPARATOR '/'
#endif


namespace help
{
    extern void print(const char *prog);
    extern void print_full(const char *prog);
}


int main(int argc, char **argv)
{
    /* get program name without full path */
    auto progname = [&argv] () -> const char*
    {
#ifdef HAVE_PROGRAM_INVOCATION_SHORT_NAME
        return program_invocation_short_name; /* GNU */

#elif defined(HAVE_GETPROGNAME)
        return getprogname(); /* BSD */

#else
        const char *p = strrchr(argv[0], PATH_SEPARATOR);

        if (p && *(p+1) != 0) {
            return p + 1;
        }

        return argv[0];
#endif
    };

    gendlopen gdo;

    try {
        gdo.process(argc, argv);
    }
    catch (const parse_args::error &e) {
        std::cerr << progname() << ": error: " << e.what() << std::endl;
        std::cerr << "Try `" << argv[0] << " -help' for more information." << std::endl;
        return 1;
    }
    catch (const gendlopen::error &e) {
        std::cerr << progname() << ": error: " << e.what() << std::endl;
        return 1;
    }
    catch (const gendlopen::help &e) {
        const char *w = e.what();

        if (w && *w) {
            help::print_full(progname());
        } else {
            help::print(progname());
        }
    }

    return 0;
}
