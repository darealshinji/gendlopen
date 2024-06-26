#include <iostream>

/* disabled [-Wattributes] warnings */
#define DLL_PUBLIC /**/
#include "helloworld.h"
#include "cxx_multi.hpp"


extern void call2(helloworld *hw);

void cb(const char *msg)
{
    std::cout << msg << std::endl;
}

int main()
{
    helloworld *hw = helloworld_init();

    helloworld_hello2(hw, cb); /* call 1 */
    call2(hw);
    helloworld_release(hw);

    return 0;
}
