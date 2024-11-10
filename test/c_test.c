/* disable [-Wattributes] warnings */
#define DLL_PUBLIC /**/
#include "helloworld.h"

/* include generated header file */
#include "c_test.h"


void cb(const char *msg)
{
    puts(msg);
}

int main(int argc, char *argv[])
{
    /* filename of the library we want to load;
     * using the macro `LIBNAME` for cross-platform compatibility */
    const gdo_char_t *filename = LIBNAME(helloworld,0);

    /* flags that will be passed to dlopen() or LoadLibraryEx() */
    int flags = GDO_DEFAULT_FLAGS;

    /* whether or not to load symbols into a new namespace */
#ifdef __illumos__
    /* libhelloworld fails to use the callback if we use dlmopen() */
    bool new_namespace = false;
#else
    bool new_namespace = true;
#endif

    /* load library and symbols */
    if (!gdo_load_lib_args(filename, flags, new_namespace) ||
        !gdo_load_all_symbols(false))
    {
        /* print the last saved error */
        fprintf(stderr, "%s\n", gdo_last_error());

        /* free library */
        gdo_free_lib();

        return 1;
    }

    const gdo_char_t *origin = gdo_lib_origin();

    if (origin) {
        printf("library loaded at: %s\n", origin);
    } else {
        fprintf(stderr, "%s\n", gdo_last_error());
    }

    /* our code */
    helloworld *hw = helloworld_init_argv(argc, argv);
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_hello2(hw, cb);
    helloworld_release(hw);

    /* free library */
    gdo_free_lib();

    return 0;
}
