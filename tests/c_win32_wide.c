/* this is a win32 API example using wide characters */
#include <stdio.h>
#include "helloworld.h"

/* include generated header file */
#include "c_test.h"


void cb(const char *msg)
{
    puts(msg);
}

int wmain()
{
    const wchar_t *lib = L"./" GDO_LIBNAMEW(helloworld, 0);

    /* load library and symbols */
    if (!gdo_load_lib_args(lib, GDO_DEFAULT_FLAGS, false) ||
        !gdo_load_all_symbols())
    {
        /* print error */
        fwprintf(stderr, L"%ls\n", gdo_last_error());

        /* free library */
        gdo_free_lib();
        return 1;
    }

    /* our code */
    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    /* get the full library path */
    wchar_t *wcs = gdo_lib_origin();

    if (wcs) {
        wprintf(L"library loaded at: %ls\n", wcs);
        free(wcs);
    }

    /* free library */
    gdo_free_lib();

    return 0;
}
