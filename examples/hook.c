#include <stdio.h>
#include <glib.h>

/**
 * Define hook macros that will be inserted into the wrap functions.
 * It's important to use the GDO_ALIAS_* prefix to call the actual function pointer.
 * Parameter names are taken from the function prototype declarations used to
 * generate the header. The macros must be defined before the header is included.
 */

// gpointer g_malloc (gsize n_bytes);
#define GDO_HOOK_g_malloc(...) \
    do { \
      gpointer ptr = GDO_ALIAS_g_malloc(__VA_ARGS__); \
      printf("memory address %p: %ld bytes allocated using g_malloc()\n", &ptr, (long)n_bytes); \
      return ptr; \
    } while(0)

// gpointer g_realloc (gpointer mem, gsize n_bytes);
#define GDO_HOOK_g_realloc(...) \
    do { \
      mem = GDO_ALIAS_g_realloc(__VA_ARGS__); \
      printf("memory address %p: %ld bytes allocated using g_realloc()\n", &mem, (long)n_bytes); \
      return mem; \
    } while(0)

// void g_free (gpointer mem);
#define GDO_HOOK_g_free(...) \
    do { \
      GDO_ALIAS_g_free(__VA_ARGS__); \
      printf("memory address %p: memory released using g_free()\n", &mem); \
      return; \
    } while(0)


/* enable wrap functions */
#define GDO_WRAP_FUNCTIONS

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
