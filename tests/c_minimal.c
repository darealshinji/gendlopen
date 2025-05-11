#include <stdio.h>
#include "helloworld.h"

/* include generated header file */
#include "c_minimal.h"


void cb(const char *msg)
{
    helloworld_fprintf(stdout, "%s: %s\n", "callback", msg);
}

int main()
{
    /* quick load and error check */
    const char *lib = LIBNAME(helloworld,0);
    const char *err = gdo_load_library_and_symbols(lib);

    if (err) {
        fprintf(stderr, "error: %s: %s\n", lib, err);
        return 1;
    }

    /* our code */
    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    /* free resources */
    gdo_free_library();

    return 0;
}
