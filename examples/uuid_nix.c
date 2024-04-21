#include <uuid.h>
#include <stdio.h>

#include "uuid_nix_dl.h"


int print_uuid()
{
    char buf[37];
    uuid_t uu;

    uuid_generate_random(uu);

    if (uuid_is_null(uu)) {
        fprintf(stderr, "error: UUID is null\n");
        return 1;
    }

    uuid_unparse_lower(uu, buf);

    printf("%s\n", buf);

    return 0;
}

int main()
{
    if (!gdo_load_lib_and_symbols()) {
        fprintf(stderr, "an error has occurred trying to dynamically load `%s':\n", GDO_DEFAULT_LIBA);
        fprintf(stderr, "%s\n", gdo_last_error());
        return 1;
    }

    int ret = print_uuid();

    gdo_free_lib();

    return ret;
}