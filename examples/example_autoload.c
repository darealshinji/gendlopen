#define _UNICODE 1
#define UNICODE 1

/* disabled [-Wattributes] warnings */
#define DLL_PUBLIC /**/

#include "helloworld.h"

/* enable automatic loading through wrapper functions;
 * the library handle is automatically released on exit */
#define GDO_ENABLE_AUTOLOAD 1
#define GDO_WRAP_FUNCTIONS  1

/* show error messages for errors that occurred during
 * auto-loading in a message box window */
#define GDO_USE_MESSAGE_BOX 1

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
