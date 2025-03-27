#include "c_multi_generated.h"

extern void cb(const char *msg);


void call2(helloworld *hw)
{
    helloworld_hello2(hw, cb);
}
