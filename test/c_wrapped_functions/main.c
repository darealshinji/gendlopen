#include "helloworld.h"

/* use wrap functions that will warn and exit
 * if the symbol wasn't loaded yet */
#define GDO_WRAP_FUNCTIONS 1

#define GDO_DEFAULT_LIB LIBNAME(helloworld,0)

/* include generated header file */
#include "c_test.h"


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    /* no explicit call to gdo_free_lib() */
    if (!gdo_load_lib_and_symbols()) {
        fprintf(stderr, "%s\n", gdo_last_error());
        return 1;
    }

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    return 0;
}
