/* disable [-Wattributes] warnings */
#define DLL_PUBLIC /**/
#include "helloworld.h"

/* include generated header file */
#include "c_test.h"


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
        /* print the last saved error */
        fprintf(stderr, "%s\n", gdo_last_error());

        /* free library */
        gdo_free_lib();

        return 1;
    }

    /* our code */
    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_hello2(hw, cb);
    helloworld_release(hw);

    /* free library */
    gdo_free_lib();

    return 0;
}
