#include <stdio.h>
#include "helloworld.h"

/* use SDL_loadso API */
#define GDO_USE_SDL

/* include generated header file */
#include "c_minimal_sdl.h"


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

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    /* release */
    gdo_free_library();

    return 0;
}
