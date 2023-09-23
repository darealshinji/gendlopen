#include "helloworld.h"

/* this is a win32 API example using wide characters */

/* set a default library to load */
#define GDO_DEFAULT_LIB  L"libhelloworld-0.dll"

/* include generated header file */
#include "example_wide.h"


void cb(const char *msg)
{
    puts(msg);
}


int wmain()
{
    /* load library and symbols */
    if (!gdo_load_lib() || !gdo_load_symbols()) {
        /* print error */
        fwprintf(stderr, L"%ls\n", gdo_last_error());

        /* free library */
        gdo_free_lib();
        return 1;
    }

    /* our code */
    helloworld *hw = helloworld_init();
    helloworld_hello(hw, cb);
    helloworld_release(hw);

    /* get the full library path */
    const wchar_t *wcs = gdo_lib_origin();
    if (wcs) wprintf(L"library loaded at: %ls\n", wcs);

    /* free library */
    gdo_free_lib();

    return 0;
}
