#include <iostream>
#include <string>
#include "helloworld.h"

/* include generated header file */
#include "cxx_test.hpp"


int main()
{
    /* set library name on initialization; nothing is loaded yet */
    gdo::dl loader( gdo::dl::make_libname("helloworld", 0) );

    /* load library and symbols */
    if (!loader.load_lib_and_symbols()) {
        std::cerr << loader.error() << std::endl;
        return 1;
    }

/*
alternatively the object can be initialized without arguments
and the filename is then specified during loading:

    gdo::dl loader;
    if (!loader.load( gdo::dl::make_libname("helloworld", 0) ) || ...
*/

    /* get and print the full library path */
    std::string orig = loader.origin();

    if (orig.empty()) {
        /* print error and return */
        std::cerr << loader.error() << std::endl;
    } else {
        std::cout << "library loaded at: " << orig << std::endl;
    }


    /* our code */

    auto cb = [] (const char *msg) {
        std::cout << msg << std::endl;
    };

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);

    std::cout << "helloworld_buffer content: " << helloworld_buffer << std::endl;
    memcpy(helloworld_buffer, "abc", 4);
    std::cout << "helloworld_buffer new content: " << helloworld_buffer << std::endl;

    helloworld_release(hw);


    /* resources are freed by the class d'tor */

    return 0;
}
