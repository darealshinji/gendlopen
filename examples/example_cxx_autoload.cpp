#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#endif

#include "helloworld.h"

/* enable automatic loading through wrapper functions */
#define GDO_ENABLE_AUTOLOAD 1

/* define a default library to load */
#define GDO_DEFAULT_LIB LIBNAME(helloworld,0)

/* include generated header file */
#include "example_cxx.hpp"


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
    gdo::message_callback = print_error;

    auto cb = [] (const char *msg) {
        std::cout << "Custom callback >>> " << msg << std::endl;
    };

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    return 0;
}
