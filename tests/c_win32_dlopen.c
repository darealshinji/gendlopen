/* copy of c_test.c */

#include <stdio.h>
#include <assert.h>

#include "helloworld.h"

#define GDO_DEFAULT_LIB GDO_LIBNAME(helloworld,0)

/* include generated header file */
#include "c_win32_dlopen.h"

#ifndef GDO_USE_DLOPEN
#error GDO_USE_DLOPEN not defined!
#endif


static void cb(const char *msg)
{
    puts(msg);
}

static void load_lib_args()
{
    /* filename of the library we want to load;
     * using the macro `GDO_LIBNAME` for cross-platform compatibility */
    const gdo_char_t *filename = GDO_LIBNAME(helloworld,0);

    /* flags that will be passed to dlopen() or LoadLibraryEx() */
    const int flags = GDO_DEFAULT_FLAGS;

    /* whether or not to load symbols into a new namespace */
#ifdef __illumos__
    /* libhelloworld fails to use the callback if we use dlmopen() */
    const bool new_namespace = false;
#else
    const bool new_namespace = true;
#endif

    /* load library and symbols */
    if (!gdo_load_lib_args(filename, flags, new_namespace) ||
        !gdo_load_all_symbols())
    {
        /* print the last saved error */
        fprintf(stderr, "%s\n", gdo_last_error());

        /* free library */
        gdo_free_lib();

        exit(1);
    }
}

static void load_lib_name()
{
    const gdo_char_t *filename = GDO_LIBNAME(helloworld,0);

    /* load library and each individual symbol */
    if (!gdo_load_lib_name(filename) ||
        !gdo_load_symbol(GDO_LOAD_helloworld_init_argv) ||
        !gdo_load_symbol(GDO_LOAD_helloworld_callback) ||
        !gdo_load_symbol(GDO_LOAD_helloworld_buffer) ||
        !gdo_load_symbol(GDO_LOAD_helloworld_hello) ||
        !gdo_load_symbol(GDO_LOAD_helloworld_hello2) ||
        !gdo_load_symbol(GDO_LOAD_helloworld_release))
    {
        fprintf(stderr, "%s\n", gdo_last_error());
        gdo_free_lib();
        exit(1);
    }
}

static void load_lib_name2()
{
    const gdo_char_t *filename = GDO_LIBNAME(helloworld,0);

    /* load library and each individual symbol */
    if (!gdo_load_lib_name(filename) ||
        !gdo_load_symbol_name("helloworld_init_argv") ||
        !gdo_load_symbol_name("helloworld_callback") ||
        !gdo_load_symbol_name("helloworld_buffer") ||
        !gdo_load_symbol_name("helloworld_hello") ||
        !gdo_load_symbol_name("helloworld_hello2") ||
        !gdo_load_symbol_name("helloworld_release"))
    {
        fprintf(stderr, "%s\n", gdo_last_error());
        gdo_free_lib();
        exit(1);
    }
}

static void load_lib_and_symbols()
{
    if (!gdo_load_lib_and_symbols()) {
        fprintf(stderr, "%s\n", gdo_last_error());
        gdo_free_lib();
        exit(1);
    }
}

static void print_lib_origin()
{
    const gdo_char_t *origin = gdo_lib_origin();

    if (origin) {
        printf("library loaded at: %s\n", origin);
    } else {
        fprintf(stderr, "%s\n", gdo_last_error());
    }
}

int main(int argc, char *argv[])
{
    // #1
    load_lib_args();
    gdo_free_lib();

    // #2
    load_lib_name();
    gdo_free_lib();

    // #3
    load_lib_name2();
    gdo_free_lib();

    // #4
    load_lib_and_symbols();

    print_lib_origin();

    assert(gdo_all_symbols_loaded() == true);
    assert(gdo_no_symbols_loaded() == false);
    assert(gdo_any_symbol_loaded() == true);

    ///////////////////////////////////////////////////////////////////////////

    /* our code */
    helloworld *hw = helloworld_init_argv(argc, argv);
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_hello2(hw, cb);

    printf("helloworld_buffer content: %s\n", helloworld_buffer);
    memcpy(helloworld_buffer, "abc", 4);
    printf("helloworld_buffer new content: %s\n", helloworld_buffer);

    helloworld_release(hw);

    /* free library */
    gdo_free_lib();

    return 0;
}
