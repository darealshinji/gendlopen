#include "helloworld.h"

/* include generated header file */
#include "example_1.h"

/* see if we can load in another one with
 * differently prefixed symbols; this code is unused */
#include "example_1x.h"


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    /* filename of the library we want to load;
     * using the macro `GDO_LIB` for cross-platform compatibility */
    const gdo_char_t *filename = GDO_LIB(helloworld,0);

    /* flags that will be passed to dlopen() or LoadLibraryEx() */
    int flags = GDO_DEFAULT_FLAGS;

    /* whether or not to load symbols into a new namespace */
    bool new_namespace = true;


    /* load library and symbols */
    if (!gdo_load_lib_args(filename, flags, new_namespace) ||
        !gdo_load_symbols())
    {
        /* print the last saved error */
        fprintf(stderr, "%s\n", gdo_last_error());

        /* free library */
        gdo_free_lib();

        return 1;
    }

    /* our code */
    helloworld *hw = helloworld_init();
    helloworld_hello(hw, cb);
    helloworld_release(hw);

    /* free library */
    gdo_free_lib();

    return 0;
}
