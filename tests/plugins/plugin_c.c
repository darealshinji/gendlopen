#include <stdio.h>

#define NUM 'C'

void plugin_main(void)
{
    printf("plugin %c: %s called\n", NUM, __func__);
}
