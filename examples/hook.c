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

#include <stdio.h>
#include <glib.h>

/**
 * Define hook macros that will be inserted into the wrap functions.
 * It's important to use the GDO_ALIAS_* prefix to call the actual function pointer.
 * Parameter names are taken from the function prototype declarations.
 */

// gpointer g_malloc (gsize n_bytes);
#define GDO_HOOK_g_malloc(...) \
    { gpointer ptr = GDO_ALIAS_g_malloc(__VA_ARGS__); \
      printf("memory address %p: %ld bytes allocated using g_malloc()\n", &ptr, (long)n_bytes); \
      return ptr; }

// gpointer g_realloc (gpointer mem, gsize n_bytes);
#define GDO_HOOK_g_realloc(...) \
    { mem = GDO_ALIAS_g_realloc(__VA_ARGS__); \
      printf("memory address %p: %ld bytes allocated using g_realloc()\n", &mem, (long)n_bytes); \
      return mem; }

// void g_free (gpointer mem);
#define GDO_HOOK_g_free(...) \
    { GDO_ALIAS_g_free(__VA_ARGS__); \
      printf("memory address %p: memory released using g_free()\n", &mem); \
      return; }


#define GDO_WRAP_FUNCTIONS  /* enable wrap functions */
#include "example_hook.h"


int main()
{
    const char *lib = LIBNAME(glib-2.0,0);

    /* load */
    if (!gdo_load_lib_name_and_symbols(lib)) {
        fprintf(stderr, "error: %s: %s\n", lib, gdo_last_error());
        gdo_free_lib();
        return 1;
    }

    /* GLib stuff */
    gpointer mem = g_malloc(32);
    mem = g_realloc(mem, 64);
    mem = g_realloc(mem, 1024);
    g_free(mem);

    /* release */
    gdo_free_lib();

    return 0;
}
