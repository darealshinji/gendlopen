/* compatibility with MSVC */
#pragma once
#include <string.h>

template<typename T=char>
int strcasecmp(const T *a, const T *b)
{
    return _stricmp(a, b);
}

template<typename T=char>
int strncasecmp(const T *a, const T *b, size_t n)
{
    return _strnicmp(a, b, n);
}

