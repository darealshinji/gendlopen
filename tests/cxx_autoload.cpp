#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")  /* MessageBoxA */
#endif

#include "helloworld.h"

/* enable automatic loading through wrapper functions */
#define GDO_ENABLE_AUTOLOAD 1
#define GDO_DELAYLOAD       1

/* define a default library to load */
#define GDO_DEFAULT_LIB LIBNAME(helloworld,0)

/* simple function hook */
#define GDO_HOOK_helloworld_hello(...) \
    std::cout << "helloworld_hello() function hooked!" << std::endl;

/* manually call the function and return early */
#define GDO_HOOK_helloworld_hello2(...) \
    std::cout << "helloworld_hello2() function hooked!" << std::endl; \
    GDO_ALIAS_helloworld_hello2(__VA_ARGS__); \
    return;

/* include generated header file */
#include "cxx_autoload.hpp"

#include <iostream>


void print_error(const char *msg)
{
#ifdef _WIN32
    MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
#else
    std::cout << "Custom callback >>> " << msg << std::endl;
#endif
}


int main()
{
    gdo::dl::message_callback(print_error);

    auto cb = [] (const char *msg) {
        /* the original prototype of helloworld_fprintf() was replaced with
         * one that has a fixed number of arguments, so we can actually
         * autoload it with a wrapper function */
        helloworld_fprintf(stdout, "%s >>> %s\n", "Custom callback", msg);
    };

    helloworld *hw = helloworld_init();

#ifdef GDO_DELAYLOAD
    helloworld_hello2(hw, cb);
#else
    /* cannot delayload an object pointer */
    helloworld_callback = cb;
    helloworld_hello(hw);
#endif

    helloworld_release(hw);

    return 0;
}
