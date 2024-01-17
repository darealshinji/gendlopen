#ifndef _STRCASECMP_H_
#define _STRCASECMP_H_

#include <string>
#include <string.h>
#ifndef _MSC_VER
#include <strings.h>
#endif

static inline bool same_string_case(const std::string &str1, const char *str2)
{
#ifdef _MSC_VER
    return (_stricmp(str1.c_str(), str2) == 0);
#else
    return (strcasecmp(str1.c_str(), str2) == 0);
#endif
}

#endif //_STRCASECMP_H_
