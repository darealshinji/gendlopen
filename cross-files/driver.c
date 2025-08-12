/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2025 Carsten Janssen

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

/* small wrapper for clang-cl and lld-link driver */

// cc -Wall -O3 -DWINSYSROOT=\"$HOME/opt/msvc-wine/msvc\" driver.c -o clang-cl -s
// cc -Wall -O3 -DWINSYSROOT=\"$HOME/opt/msvc-wine/msvc\" -DLLD_LINK driver.c -o lld-link -s

#include <alloca.h>
#include <unistd.h>

#ifndef CLANG_VERSION
#define CLANG_VERSION "19"
#endif

#ifndef DEFAULT_PREFIX
#define DEFAULT_PREFIX "/usr/bin"
#endif

#ifndef WINSYSROOT
#define WINSYSROOT "/opt/clang/" CLANG_VERSION "/x86_64-pc-windows-msvc"
#endif

#ifdef LLD_LINK
#define DRIVER "lld-link"
#define EXTRA_ARGS 2
#else
#define DRIVER "clang-cl"
#define EXTRA_ARGS 4
#endif


int main(int argc, char **argv)
{
    int i, j;
    char **argv_new = alloca((argc + EXTRA_ARGS) * sizeof(char**));

    /* driver */
    const char *driver = DEFAULT_PREFIX "/" DRIVER "-" CLANG_VERSION;

    /* replace argv[0] */
    argv_new[0] = (char *)driver;

    /* set linker and sysroot */
#ifdef LLD_LINK
    argv_new[1] = (char *)"/winsysroot:" WINSYSROOT;
#else
    argv_new[1] = (char *)"-fuse-ld=" DEFAULT_PREFIX "/lld-link-" CLANG_VERSION;
    argv_new[2] = (char *)"/winsysroot";
    argv_new[3] = (char *)WINSYSROOT;
#endif

    /* add remaining command line arguments */
    for (i = 1, j = EXTRA_ARGS; i < argc; i++, j++) {
        argv_new[j] = argv[i];
    }

    /* append NULL */
    argv_new[argc + EXTRA_ARGS] = NULL;

    /* start process */
    return execvp(driver, argv_new);
}
