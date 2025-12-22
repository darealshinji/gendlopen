/* Create a shim library. Do not put it into any default library search path
 * to prevent the library recursively loading itself!
 * Use with "LD_LIBRARY_PATH=$PWD ./shim_main" to test the hooks. */

#include <stdio.h>
#include <stdlib.h>

/* static linkage for gdo_* functions */
#define GDO_STATIC

/* enable and export wrapped functions */
#define GDO_WRAP_FUNCTIONS
#define GDO_WRAP_VISIBILITY  __attribute__ ((visibility ("default")))

/* include hook macros */
#include "hook.h"

/* include generated header */
#include <glib.h>
#include "example_hook.h"


/* library name */
#define GLIB GDO_LIBNAME(glib-2.0,0)

/* Linux multiarch paths */
#define MULTIARCH(x) \
    "/usr/local/lib/" x "-linux-gnu/" GLIB, \
    "/lib/" x "-linux-gnu/" GLIB, \
    "/usr/lib/" x "-linux-gnu/" GLIB


/* custom library constructor */
void shim_constructor() __attribute__ ((constructor));
void shim_constructor()
{
    /* do a manual search through default library paths */
    const char * const paths[] =
    {
#ifdef __linux__
# ifdef __x86_64__
        MULTIARCH("x86_64"),
# endif
# ifdef __i386__
        MULTIARCH("i386"),
# endif
# ifdef __i686__
        MULTIARCH("i686"),
# endif
#endif
        "/usr/local/lib/" GLIB,
#ifdef __LP64__
        "/lib64/" GLIB,
        "/usr/lib64/" GLIB,
#endif
        "/lib/" GLIB,
        "/usr/lib/" GLIB,
        NULL
    };

    const bool verbose = false;


    for (int i = 0; paths[i] != NULL; i++) {
        if (!gdo_load_lib_name_and_symbols(paths[i])) {
            gdo_force_free_lib();
            continue;
        }

        if (verbose) {
            char *orig = gdo_lib_origin();

            if (orig) {
                fprintf(stderr, "shim library initialized: %s -> %s\n", GLIB, orig);
                free(orig);
            }
        }

        return;
    }

    /* errors are handled by function wrappers */
}

