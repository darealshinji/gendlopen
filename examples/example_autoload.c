#include "helloworld.h"

/* enable automatic loading through wrapper functions;
 * the library handle is automatically released on exit */
#define GDO_USE_WRAPPER 1

/* define a default library to load; this is required */
#define GDO_DEFAULT_LIB GDO_LIB(helloworld,0)

/* include generated header file */
#include "example_autoload.h"


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    /* no extra code needs to be added here */

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    return 0;
}
