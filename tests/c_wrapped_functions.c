#include <stdio.h>
#include "helloworld.h"

/* use wrap functions that will warn and exit
 * if the symbol wasn't loaded yet */
#define GDO_WRAP_FUNCTIONS 1

#define GDO_DEFAULT_LIB GDO_LIBNAME(helloworld,0)

/* include generated header file */
#include "c_wrapped_functions.h"


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    /* load library and symbols */
    if (!gdo_load_lib_and_symbols()) {
        fprintf(stderr, "%s\n", gdo_last_error());
        gdo_free_lib();
        return 1;
    }

    /* our code */
    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    /* free resources */
    gdo_free_lib();

    return 0;
}
