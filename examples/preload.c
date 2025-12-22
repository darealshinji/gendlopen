/* Create a shim library to use with LD_PRELOAD as a safer alternative to the RTLD_NEXT method.
 * Use with "LD_PRELOAD=./preload.so ./preload_main" to test the hooks. */

/* enable auto-loading */
#define GDO_DEFAULT_LIB  GDO_LIBNAME(glib-2.0,0)
#define GDO_ENABLE_AUTOLOAD
#define GDO_ENABLE_AUTOLOAD_LAZY

/* static linkage for gdo_* functions */
#define GDO_STATIC

/* export wrapped functions */
#define GDO_WRAP_VISIBILITY  __attribute__ ((visibility ("default")))

/* include hook macros */
#include <stdio.h>
#include "hook.h"

/* include generated header */
#include <glib.h>
#include "example_hook.h"

