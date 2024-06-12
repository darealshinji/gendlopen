/* disable [-Wattributes] warnings */
#define DLL_PUBLIC /**/
#include "helloworld.h"
#include "c_multi.h"


extern void cb(const char *msg);

void call2(helloworld *hw)
{
    helloworld_hello2(hw, cb);
}
