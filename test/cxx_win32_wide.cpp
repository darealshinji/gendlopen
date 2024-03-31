#include <iostream>
#include <string>
#include "helloworld.h"

/* this is a win32 API example using wide characters;
 * note: on C++ wide and narrrow characters can be
 * used simultanously */

/* include generated header file */
#include "cxx_win32_wide.hpp"


int main()
{
    /* load library and symbols */

    gdo::dl loader;

    if (!loader.load(L"libhelloworld-0.dll") || !loader.load_symbols()) {
        std::cerr << loader.error() << std::endl;
        //std::wcerr << L"(wide characters message) " << loader.error_w() << std::endl;
        return 1;
    }

    /* get and print library path */
    std::wstring orig = loader.origin_w();

    if (orig.empty()) {
        /* print error and return */
        std::cerr << loader.error() << std::endl;
        return 1;
    }

    std::wcout << L"library loaded at: " << orig << std::endl;


    /* our code */

    auto cb = [] (const char *msg) {
        std::cout << msg << std::endl;
    };

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);


    /* resources are freed by the class d'tor */

    return 0;
}
