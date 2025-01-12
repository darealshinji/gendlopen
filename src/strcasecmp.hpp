/* compatibility with MSVC */
#pragma once
#include <string.h>

template<typename T=char>
int strcasecmp(const T *a, const T *b)
{
    return _stricmp(a, b);
}
