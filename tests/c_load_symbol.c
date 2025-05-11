#include <stdio.h>
#include "helloworld.h"

/* include generated header file */
#include "c_load_symbol.h"


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    /* load library and each individual symbol */
    if (!gdo_load_lib_name(LIBNAME(helloworld,0)) ||
        !gdo_load_symbol(GDO_LOAD_helloworld_init) ||
        !gdo_load_symbol(GDO_LOAD_helloworld_callback) ||
        !gdo_load_symbol(GDO_LOAD_helloworld_hello) ||
        !gdo_load_symbol(GDO_LOAD_helloworld_hello2) ||
        !gdo_load_symbol(GDO_LOAD_helloworld_release))
    {
        fprintf(stderr, "%s\n", gdo_last_error());
        gdo_free_lib();
        return 1;
    }

    /* our code */
    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_hello2(hw, cb);
    helloworld_release(hw);

    /* free resources */
    gdo_free_lib();

    return 0;
}
