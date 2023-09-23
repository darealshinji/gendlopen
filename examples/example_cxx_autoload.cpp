#ifdef _MSC_VER
#pragma comment(lib, "user32.lib")
#endif

#include "helloworld.h"

/* enable automatic loading through wrapper functions */
#define GDO_USE_WRAPPER 1

/* define a default library to load */
#define GDO_DEFAULT_LIB GDO_LIB(helloworld,0)

/* use message box window on Windows (has no effect on other platforms) */
#define GDO_USE_MESSAGE_BOX 1

/* include generated header file */
#include "example_cxx.hpp"


int main()
{
    /* no extra code needs to be added here */

    auto cb = [] (const char *msg) {
        std::cout << msg << std::endl;
    };

    helloworld *hw = helloworld_init();
    helloworld_hello(hw, cb);
    helloworld_release(hw);

    return 0;
}
