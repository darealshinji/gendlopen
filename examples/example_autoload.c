#define _UNICODE 1
#define UNICODE 1

/* disabled [-Wattributes] warnings */
#define DLL_PUBLIC /**/

#include "helloworld.h"

/* enable automatic loading through wrapper functions;
 * enable delayloading (each function is only loaded when it's first used);
 * the library handle is automatically released on exit */
#define GDO_ENABLE_AUTOLOAD 1
#define GDO_WRAP_FUNCTIONS  1
#define GDO_DELAYLOAD       1

/* show error messages for errors that occurred during
 * auto-loading in a message box window */
#define GDO_USE_MESSAGE_BOX 1

/* define a default library to load; this is required */
#define GDO_DEFAULT_LIB LIBNAME(helloworld,0)

/* include generated header file */
#include "example_autoload.h"


void cb(const char *msg)
{
    printf("Custom callback >>> %s\n", msg);
}

int main()
{
    /* no extra code needs to be added here */

    helloworld *hw = helloworld_init();

#ifdef GDO_DELAYLOAD
    helloworld_hello2(hw, cb);
#else
    helloworld_callback = cb;
    helloworld_hello(hw);
#endif

    helloworld_release(hw);

    return 0;
}
