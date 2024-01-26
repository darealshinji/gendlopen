/* compile this as a static library replacement for libhelloworld.so */

/* regular headers */
#define BUILDING_STATIC 1
#include "helloworld.h"

/* configuration */
#define GDO_ENABLE_AUTOLOAD  1
#define GDO_WRAP_FUNCTIONS   1
#define GDO_DEFAULT_LIB      GDO_LIB(helloworld, 0)

/* generated header file */
#include "example_helloworld_static_dl.h"
