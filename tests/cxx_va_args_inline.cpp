#include <iostream>

/* simple function hook */
#define GDO_HOOK_helloworld_fprintf(...) \
    std::cout << "helloworld_fprintf() function hooked!" << std::endl;

/* include generated header file */
#include "cxx_va_args_inline_generated.hpp"



int main()
{
    helloworld_fprintf(stdout, "%s is a %s%c\n", "This", "test", '!');

    return 0;
}

