#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#endif

#define BUILDING_STATIC
#include "helloworld.h"

/* enable automatic loading through wrapper functions */
#define GDO_ENABLE_AUTOLOAD 1
#define GDO_DELAYLOAD       1

/* define a default library to load */
//#define GDO_DEFAULT_LIB LIBNAME(helloworld,0)

/* include generated header file */
#include "cxx_autoload.hpp"


void print_error(const char *msg)
{
#ifdef _WIN32
    MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
#else
    std::cout << "ERROR >>> " << msg << std::endl;
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
    helloworld_callback = cb;
    helloworld_hello(hw);
#endif

    helloworld_release(hw);

    return 0;
}
