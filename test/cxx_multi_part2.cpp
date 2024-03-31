#include <iostream>

/* disabled [-Wattributes] warnings */
#define DLL_PUBLIC /**/
#include "helloworld.h"
#include "cxx_multi.hpp"


extern void cb(const char *msg);

void call2(helloworld *hw)
{
    helloworld_hello2(hw, cb);
}
