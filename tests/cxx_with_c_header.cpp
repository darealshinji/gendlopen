#include <iostream>
#include "helloworld.h"

#define GDO_DEFAULT_LIB LIBNAME(helloworld,0)

/* include generated header file */
#include "cxx_with_c_header.h"


void cb(const char *msg)
{
    std::cout << msg << std::endl;
}

int main()
{
    if (!gdo_load_lib_and_symbols()) {
        std::cerr << gdo_last_error() << std::endl;
        gdo_free_lib();
        return 1;
    }

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    gdo_free_lib();

    return 0;
}
