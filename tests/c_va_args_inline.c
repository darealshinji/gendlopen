#include <stdio.h>

/* simple function hook */
#define GDO_HOOK_helloworld_fprintf(...) \
    puts("helloworld_fprintf() function hooked!");

/* include generated header file */
#include "c_va_args_inline_generated.h"



int main()
{
#ifdef GDO_VA_ARG_PACK_INLINE
    helloworld_fprintf(stdout, "%s is a %s%c\n", "This", "test", '!');
#else
    puts("cannot use inline VA_ARGS functions");
#endif

    return 0;
}

