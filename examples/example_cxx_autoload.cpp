#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#endif

#include "helloworld.h"

/* enable automatic loading through wrapper functions */
#define GDO_USE_WRAPPER 1

/* define a default library to load */
#define GDO_DEFAULT_LIB GDO_LIB(helloworld,0)

/* include generated header file */
#include "example_cxx.hpp"


#ifdef _WIN32
void print_error(const char *msg)
{
    MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
}
#endif


int main()
{
#ifdef _WIN32
    gdo::dl::set_callback(print_error);
#endif

    auto cb = [] (const char *msg) {
        std::cout << msg << std::endl;
    };

    helloworld *hw = helloworld_init();
    helloworld_hello(hw, cb);
    helloworld_release(hw);

    return 0;
}
