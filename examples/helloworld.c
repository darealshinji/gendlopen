#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helloworld.h"

struct helloworld_
{
    char str[64];
};


/* allocate object */
helloworld *helloworld_init()
{
    return malloc(sizeof(helloworld));
}

/* use object and respond something */
void helloworld_hello(helloworld *hw, void (*callback)(const char *))
{
    if (!hw || !callback) return;
    strcpy(hw->str, "hello world");
    callback(hw->str);
}

/* release object */
void helloworld_release(helloworld *hw)
{
    if (hw) free(hw);
}

