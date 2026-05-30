#include <stdio.h>

/* simple function hook */
#define GDO_HOOK_helloworld_fprintf(...) \
    puts("helloworld_fprintf() function hooked!")

/* include generated header file */
#include "c_va_args_inline_generated.h"



int main()
{
    if (helloworld_fprintf(stdout, "%s is a %s%c\n", "This", "test", '!') == 0) {
        return 1;
    }

    return 0;
}

