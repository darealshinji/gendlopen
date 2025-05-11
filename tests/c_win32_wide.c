/* this is a win32 API example using wide characters */
#include <stdio.h>
#include "helloworld.h"

/* set a default library to load */
#define GDO_DEFAULT_LIB  LIBNAMEW(helloworld,0)

/* include generated header file */
#include "c_test.h"


void cb(const char *msg)
{
    puts(msg);
}

int wmain()
{
    /* load library and symbols */
    if (!gdo_load_lib_and_symbols()) {
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
