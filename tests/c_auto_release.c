#include <stdio.h>
#include "helloworld.h"

#define GDO_DEFAULT_LIB GDO_LIBNAME(helloworld,0)

/* include generated header file */
#include "c_auto_release.h"


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    /* enable auto-release, load library and symbols */
    if (!gdo_enable_autorelease() || !gdo_load_lib_and_symbols()) {
        fprintf(stderr, "%s\n", gdo_last_error());
        return 1;
    }

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    /* no explicit call to gdo_free_lib() */

    return 0;
}
