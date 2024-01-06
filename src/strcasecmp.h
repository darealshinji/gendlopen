#ifndef _COMPAT_H_
#define _COMPAT_H_

#include <string.h>

#ifdef _MSC_VER
#ifndef strcasecmp
#define strcasecmp _stricmp
#endif
#else
#include <strings.h>
#endif

#endif //_COMPAT_H_
