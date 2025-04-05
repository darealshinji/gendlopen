#include <stdio.h>

#define NUM 'A'

void plugin_main(void)
{
    printf("plugin %c: %s called\n", NUM, __func__);
}
