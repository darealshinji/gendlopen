/* WinAPI UUID example */

#include <rpc.h>
#include <stdio.h>

/* generated header */
#include "example_uuid_win.h"


/* generate and print a UUID */
static int print_uuid()
{
    UUID uu;
    RPC_CSTR *str = NULL;

    if (UuidCreate(&uu) != RPC_S_OK) {
        fprintf(stderr, "Failed to create UUID\n");
        return 1;
    }

    if (UuidToStringA(&uu, (RPC_CSTR *)&str) != RPC_S_OK) {
        fprintf(stderr, "Failed to create string from UUID\n");
        return 1;
    }

    printf("%s\n", (char *)str);

    RpcStringFreeA((RPC_CSTR *)&str);

    return 0;
}

int main()
{
    /* load library and symbols */
    if (!gdo_load_lib_and_symbols()) {
        fprintf(stderr, "an error has occurred trying to dynamically load `%s':\n%s\n",
            GDO_DEFAULT_LIBA, gdo_last_error());
        return 1;
    }

    /* run our code */
    int rv = print_uuid();

    /* free resources */
    gdo_free_lib();

    return rv;
}
