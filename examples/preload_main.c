#include <glib.h>


int main()
{
    gpointer mem = g_malloc(32);
    mem = g_realloc(mem, 64);
    mem = g_realloc(mem, 1024);
    g_free_sized(mem, 1024);

    return 0;
}
