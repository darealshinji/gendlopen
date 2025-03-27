#include "cxx_multi_generated.hpp"

extern void cb(const char *msg);
extern void call2(helloworld *hw);


int main()
{
    helloworld *hw = helloworld_init();

    helloworld_hello2(hw, cb); /* call 1 */
    call2(hw);
    helloworld_release(hw);

    return 0;
}
