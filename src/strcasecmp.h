#ifndef _STRCASECMP_H_
#define _STRCASECMP_H_

#include <string.h>

#ifdef _MSC_VER
  #ifndef strcasecmp
  #define strcasecmp _stricmp
  #endif
#else
  #include <strings.h>
#endif

#endif //_STRCASECMP_H_
