#include <iostream>
#include <string>
#include "helloworld.h"

/* include generated header file */
#include "example_cxx.hpp"


int main()
{
    /* set library name on initialization; nothing is loaded yet */
    gdo::dl loader( GDO_LIB(helloworld,0) );

    /* load library and symbols */
    if (!loader.load() || !loader.load_symbols()) {
        std::cerr << loader.error() << std::endl;
        return 1;
    }

/*
alternatively the object can be initialized without arguments
and the filename is then specified during loading:

    gdo::dl loader;
    if (!loader.load("libhelloworld.so.0") || ...
*/

    /* get and print the full library path */
    std::string orig = loader.origin();

    if (orig.empty()) {
        /* print error and return */
        std::cerr << loader.error() << std::endl;
        return 1;
    }

    std::cout << "library loaded at: " << orig << std::endl;


    /* our code */

    auto cb = [] (const char *msg) {
        std::cout << msg << std::endl;
    };

    helloworld *hw = helloworld_init();
    helloworld_hello(hw, cb);
    helloworld_release(hw);


    /* resources are freed by the class d'tor */

    return 0;
}
