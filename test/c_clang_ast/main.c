#include <stdio.h>
#include "helloworld.h"

#define GDO_DEFAULT_LIB LIBNAME(helloworld,0)

/* include generated header file */
#include "c_test.h"


void cb(const char *msg)
{
    puts(msg);
}

int main(int argc, char *argv[])
{
    /* no explicit call to gdo_free_lib() */
    if (!gdo_load_lib_and_symbols()) {
        fprintf(stderr, "%s\n", gdo_last_error());
        gdo_free_lib();
        return 1;
    }

    helloworld *hw = helloworld_init_argv(argc, argv);
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    gdo_free_lib();

    return 0;
}
