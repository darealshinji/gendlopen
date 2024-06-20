#include <iostream>
#include <string>
#include <vector>
#include "helloworld.h"

/* include generated header file */
#include "cxx_list.hpp"


int main()
{
    gdo::dl loader;

#ifdef _WIN32
    std::vector<std::wstring> list = {
        L"helloworld.dll",
        L"libhelloworld.dll",
        L"libhelloworld-0.dll",
    };
#else
    std::vector<std::string> list = {
        "libhelloworld.0.dylib",
        "libhelloworld.so.0",
        "libhelloworld.so"
    };
#endif

    if (!loader.load_from_list(list) || !loader.load_symbols()) {
        std::cerr << loader.error() << std::endl;
        return 1;
    }

    /* get and print the full library path */
    std::string orig = loader.origin();

    if (orig.empty()) {
        /* print error and return */
        std::cerr << loader.error() << std::endl;
    } else {
        std::cout << "library loaded at: "
            << loader.filename() << " -> " << orig << std::endl;
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
