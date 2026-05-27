#include <iostream>
#include <string>

#include "helloworld.h"

/* include generated header file */
#include "cxx_test.hpp"


static void ctor_load_lib_and_symbols()
{
    const std::wstring name = L"./" + gdo::make_libname(L"helloworld", 0);

    gdo::dl loader(name);

    if (!loader.load_lib_and_symbols()) {
        std::wcerr << loader.error_w() << std::endl;
        std::exit(1);
    }

    std::cerr << __func__ << " succeeded" << std::endl;
}

static void ctor_load_lib_and_symbols_convert()
{
    const std::string name = "./" + gdo::make_libname("helloworld", 0);

    gdo::dl loader(name);

    loader.convert_filename_to_wcs(true);

    if (!loader.load_lib_and_symbols()) {
        std::wcerr << loader.error_w() << std::endl;
        std::exit(1);
    }

    std::cerr << __func__ << " succeeded" << std::endl;
}


int wmain()
{
    ctor_load_lib_and_symbols();
    ctor_load_lib_and_symbols_convert();

    gdo::dl loader( gdo::make_libname(L"helloworld", 0) );

    if (!loader.load_lib_and_symbols()) {
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
