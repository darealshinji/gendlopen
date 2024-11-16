/* disable [-Wattributes] warnings */
#define DLL_PUBLIC /**/
#include "helloworld.h"

/* include generated header file */
#include "cxx_load_symbol.hpp"


int main()
{
    gdo::dl loader;

    /* load library and each individual symbol */
    if (!loader.load( gdo::dl::make_libname("helloworld", 0) ) ||
        !loader.load_symbol(gdo::dl::LOAD_helloworld_init) ||
        !loader.load_symbol(gdo::dl::LOAD_helloworld_callback) ||
        !loader.load_symbol(gdo::dl::LOAD_helloworld_hello) ||
        !loader.load_symbol(gdo::dl::LOAD_helloworld_hello2) ||
        !loader.load_symbol(gdo::dl::LOAD_helloworld_release))
    {
        std::cerr << loader.error() << std::endl;
        return 1;
    }

    /* our code */

    auto cb = [] (const char *msg) {
        std::cout << msg << std::endl;
    };

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_hello2(hw, cb);
    helloworld_release(hw);

    return 0;
}
