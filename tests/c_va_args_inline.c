#include <stdio.h>

/* simple function hook */
#define GDO_HOOK_helloworld_fprintf(...) \
    puts("helloworld_fprintf() function hooked!");

/* include generated header file */
#include "c_va_args_inline_generated.h"



int main()
{
#ifdef GDO_HAS_BUILTIN_VA_ARG_PACK
    helloworld_fprintf(stdout, "%s is a %s%c\n", "This", "test", '!');
#else
    puts("cannot use inline VA_ARGS functions");
#endif

    return 0;
}

