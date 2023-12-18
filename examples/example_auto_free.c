#include "helloworld.h"

/* automatically release library on exit using
 * the atexit() function */
#define GDO_ATEXIT 1

/* include generated header file */
#include "example_auto_free.h"


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    /* this code is practically identical to example_1.c except
     * that there is no explicit call to gdo_free_lib() */

    if (!gdo_load_lib_args(GDO_LIB(helloworld,0), GDO_DEFAULT_FLAGS, true) ||
        !gdo_load_symbols(false))
    {
        fprintf(stderr, "%s\n", gdo_last_error());
        return 1;
    }

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    return 0;
}
