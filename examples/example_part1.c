#include <stdio.h>

/* disabled [-Wattributes] warnings */
#define DLL_PUBLIC /**/
#include "helloworld.h"
#include "example_multi.h"

extern void call2(helloworld *hw);


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    helloworld *hw = helloworld_init();

    helloworld_hello2(hw, cb); /* call 1 */
    call2(hw);
    helloworld_release(hw);

    return 0;
}
