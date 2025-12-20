#include <stdio.h>
#include <glib.h>

/* enable wrap functions */
#define GDO_WRAP_FUNCTIONS

/* hook macros */
#include "hook.h"

/* include generated header */
#include "example_hook.h"


int main()
{
    const char *library = GDO_LIBNAME(glib-2.0,0);

    /* load library and symbols */
    if (!gdo_load_lib_name_and_symbols(library)) {
        fprintf(stderr, "error: %s: %s\n", library, gdo_last_error());
        gdo_free_lib();
        return 1;
    }

    /* do GLib stuff */
    gpointer mem = g_malloc(32);
    mem = g_realloc(mem, 64);
    mem = g_realloc(mem, 1024);
    g_free(mem);

    /* free resources */
    gdo_free_lib();

    return 0;
}
