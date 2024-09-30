#include "helloworld.h"

/* automatically release library on exit using
 * the atexit() function */
#define GDO_AUTO_RELEASE 1

/* include generated header file */
#include "c_auto_free.h"


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    /* whether or not to load symbols into a new namespace */
#ifdef __illumos__
    /* libhelloworld fails to use the callback if we use dlmopen() */
    bool new_namespace = false;
#else
    bool new_namespace = true;
#endif

    /* no explicit call to gdo_free_lib() */

    if (!gdo_load_lib_args(LIBNAME(helloworld,0), GDO_DEFAULT_FLAGS, new_namespace) ||
        !gdo_load_all_symbols(false))
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
