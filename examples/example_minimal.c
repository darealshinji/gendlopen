#include <stdio.h>
#include "helloworld.h"

/* include generated header file */
#include "example_minimal.h"


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    /* quick load */
    if (!gdo_load_library_and_symbols("libhelloworld.so.0")) {
        fprintf(stderr, "failed to load library\n");
        return 1;
    }

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    return 0;
}
