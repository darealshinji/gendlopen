#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helloworld.h"

struct helloworld_
{
    char str[64];
};

helloworld_cb_t helloworld_callback = NULL;


/* allocate object */
helloworld *HELLOWORLD_INIT()
{
    return malloc(sizeof(helloworld));
}

/* use object and respond something */
void helloworld_hello(helloworld *hw)
{
    if (hw && helloworld_callback) {
        strcpy(hw->str, "hello world");
        helloworld_callback(hw->str);
    }
}

/* release object */
void helloworld_release(helloworld *hw)
{
    if (hw) free(hw);
}

