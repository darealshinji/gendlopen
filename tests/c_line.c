#include <stdio.h>
#include "helloworld.h"

#define GDO_DEFAULT_LIB GDO_LIBNAME(helloworld,0)

/* include generated header file */
#include "c_line.h"


void cb(const char *msg)
{
    helloworld_fprintf(stdout, "%s: %s\n", "callback", msg);
}

int main()
{
    /* load library and symbols */
    if (!gdo_load_lib_and_symbols()) {
        fprintf(stderr, "%s\n", gdo_last_error());
        return 1;
    }

    /* our code */
    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    /* free library */
    gdo_free_lib();

    return 0;
}
