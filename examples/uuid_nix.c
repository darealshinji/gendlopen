/* libuuid example */

#include <uuid.h>
#include <stdio.h>

/* generated header */
#include "example_uuid_nix.h"


/* generate and print a UUID */
static int print_uuid()
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
