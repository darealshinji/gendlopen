/* example file without generated headers */

#include <stdio.h>

#define BUILDING_STATIC
#include "helloworld.h"


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    helloworld *hw = helloworld_init();
    helloworld_hello2(hw, cb);
    helloworld_release(hw);

    return 0;
}
