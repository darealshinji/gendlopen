#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "helloworld.h"


struct helloworld_
{
    char str[16];
};

helloworld_cb_t helloworld_callback = NULL;

char helloworld_buffer[64] = "pointless buffer!";


/* allocate object */
helloworld *helloworld_init()
{
    return malloc(sizeof(helloworld));
}

/* same as above */
helloworld *helloworld_init_argv(int argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return helloworld_init();
}

/* use object and respond something */
void helloworld_hello(helloworld *hw)
{
    helloworld_hello2(hw, helloworld_callback);
}

/* use object and respond something */
void helloworld_hello2(helloworld *hw, void (*helloworld_cb)(const char *))
{
    if (hw && helloworld_cb) {
        memcpy(hw->str, "hello world\0", 12);
        helloworld_cb(hw->str);
    } else {
        helloworld_fprintf(stderr, "%s\n", "helloworld_cb == NULL");
    }
}

/* release object */
void helloworld_release(helloworld *hw)
{
    if (hw) free(hw);
}

/* fprintf implementation */
int helloworld_fprintf(FILE *stream, const char *format, ...)
{
    int n;
    va_list ap;

    va_start(ap, format);
    n = vfprintf(stream, format, ap);
    va_end(ap);

    return n;
}
