#include <iostream>
#include <string>

/* disable [-Wattributes] warnings */
#define DLL_PUBLIC /**/

#include "helloworld.h"

/* include generated header file */
#include "cxx_test.hpp"


int wmain()
{
    /* load library and symbols */

    gdo::dl loader;

    if (!loader.load(L"libhelloworld-0.dll") || !loader.load_all_symbols()) {
        std::wcerr << loader.error_w() << std::endl;
        return 1;
    }

    /* get and print library path */
    std::wstring orig = loader.origin_w();

    if (orig.empty()) {
        std::wcerr << loader.error_w() << std::endl;
    } else {
        std::wcout << L"library loaded at: " << orig << std::endl;
    }


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
