#include <iostream>
#include <string>
#include <list>
#include "helloworld.h"

/* include generated header file */
#include "cxx_test.hpp"


static void ctor_load_lib_and_symbols()
{
    const auto filename = myprefix::dl::make_libname("helloworld", 0);
    const int flags = myprefix::dl::default_flags;

    /* whether or not to load symbols into a new namespace */
#ifdef __illumos__
    /* libhelloworld fails to use the callback if we use dlmopen() */
    const bool new_namespace = false;
#else
    const bool new_namespace = true;
#endif

    /* set library name on initialization; nothing is loaded yet */
    myprefix::dl loader(filename, flags, new_namespace);

    if (!loader.load_lib_and_symbols()) {
        std::cerr << loader.error() << std::endl;
        std::exit(1);
    }

    loader.free();
}

static void ctor_load()
{
    /* set library name on initialization; nothing is loaded yet */
    myprefix::dl loader( myprefix::dl::make_libname("helloworld", 0) );

    if (!loader.load() || !loader.load_all_symbols()) {
        std::cerr << loader.error() << std::endl;
        std::exit(1);
    }

    loader.free();
}

static void empty_ctor_load()
{
    myprefix::dl loader;

    if (!loader.load( myprefix::dl::make_libname("helloworld", 0) ) ||
        !loader.load_symbol(myprefix::dl::LOAD_helloworld_init) ||
        !loader.load_symbol(myprefix::dl::LOAD_helloworld_callback) ||
        !loader.load_symbol(myprefix::dl::LOAD_helloworld_hello) ||
        !loader.load_symbol(myprefix::dl::LOAD_helloworld_hello2) ||
        !loader.load_symbol(myprefix::dl::LOAD_helloworld_release))
    {
        std::cerr << loader.error() << std::endl;
        std::exit(1);
    }

    loader.free();
}

static void load_from_list(myprefix::dl &loader)
{
#ifdef _WIN32
    std::list<std::wstring> list = {
        L"helloworld.dll",
        L"libhelloworld.dll",
        L"libhelloworld-0.dll",
    };
#else
    std::list<std::string> list = {
        "libhelloworld.0.dylib",
        "libhelloworld.so.0",
        "libhelloworld.so",
        "libhelloworld.a"
    };
#endif

    if (!loader.load_from_list(list) || !loader.load_all_symbols()) {
        std::cerr << loader.error() << std::endl;
        std::exit(1);
    }

    /* get and print the full library path */
    std::string orig = loader.origin();

    if (orig.empty()) {
        /* print error and return */
        std::cerr << loader.error() << std::endl;
    } else {
        std::cout << "library loaded at: " << orig << std::endl;
    }
}


int main()
{
    ctor_load_lib_and_symbols();
    ctor_load();
    empty_ctor_load();

    myprefix::dl loader;
    load_from_list(loader);

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
