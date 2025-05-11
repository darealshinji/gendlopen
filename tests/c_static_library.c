/* example file without generated headers;
 * generated auto-loading code will be linked in
 * form of a static library */

#include <stdio.h>
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
