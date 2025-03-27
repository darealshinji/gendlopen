/* include generated header file */
#include "c_option.h"

#ifndef HELLOWORLD_TEST
#error HELLOWORLD_TEST was not defined!
#endif


void cb(const char *msg)
{
    puts(msg);
}

int main()
{
    if (!mydl_load_lib_and_symbols()) {
        fprintf(stderr, "%s\n", mydl_last_error());
        mydl_free_lib();
        return 1;
    }

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    mydl_free_lib();

    return 0;
}
