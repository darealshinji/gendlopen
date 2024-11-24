#include "multi.hpp"
#include "cxx_test.hpp"


int main()
{
    helloworld *hw = helloworld_init();

    helloworld_hello2(hw, cb); /* call 1 */
    call2(hw);
    helloworld_release(hw);

    return 0;
}
