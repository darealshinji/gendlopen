#include "helloworld.h"

/* use wrap functions that will warn and exit
 * if the symbol wasn't loaded yet */
#define GDO_WRAP_FUNCTIONS 1

#define GDO_DEFAULT_LIB LIBNAME(helloworld,0)

/* include generated header file */
#include "cxx_wrapped_functions.hpp"

#include <iostream>


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    gdo::dl loader(gdo::make_libname("helloworld", 0));

    if (!loader.load_lib_and_symbols()) {
        std::cerr << loader.error() << std::endl;
        return 1;
    }

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    return 0;
}

