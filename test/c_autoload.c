/* enable wide char API */
#ifdef _WIN32
#define _UNICODE 1
#define UNICODE  1
#endif // _WIN32

/* disable [-Wattributes] warnings */
#define DLL_PUBLIC /**/

/* our "hello world" API */
#include "helloworld.h"

/* enable automatic loading through wrapper functions */
#define GDO_ENABLE_AUTOLOAD 1
#define GDO_WRAP_FUNCTIONS  1

/* enable delayloading (each function is only loaded when it's first used) */
#define GDO_DELAYLOAD 1

/* automatically release library on exit using
 * the atexit() function */
//#define GDO_AUTO_RELEASE 1

/* show error messages for errors that occurred during
 * auto-loading in a message box window (Windows only) */
#define GDO_USE_MESSAGE_BOX 1

/* define a default library to load; this is required */
#define GDO_DEFAULT_LIB LIBNAME(helloworld,0)

/* include generated header file */
#include "c_autoload.h"


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
    /* cannot delayload an object pointer */
    helloworld_callback = cb;
    helloworld_hello(hw);
#endif

    helloworld_release(hw);

    return 0;
}
