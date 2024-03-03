#include <rpc.h>
#include <stdio.h>

#include "uuid_win_dl.h"


int print_uuid()
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
    if (!gdo_load_lib_and_symbols()) {
        fprintf(stderr, "%s\n", gdo_last_error());
        return 1;
    }

    int ret = print_uuid();

    gdo_free_lib();

    return ret;
}
