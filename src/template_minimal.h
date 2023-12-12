#ifdef _$WINAPI
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#ifndef __cplusplus
#include <stdbool.h>
#endif

#if !defined(_$LINKAGE)
#define _$LINKAGE  inline static
#endif

GDO_TYPEDEFS


/***************************************************************************/
/* typedefs */
/***************************************************************************/
#define _$DECLARE_TYPEDEFS \
    typedef GDO_TYPE (*_gdo_GDO_SYMBOL_t)(GDO_ARGS); \
    /**/
/***************************************************************************/



/***************************************************************************/
/* function pointers */
/***************************************************************************/
#define _$DECLARE_FUNCTION_POINTERS \
    _$LINKAGE _gdo_GDO_SYMBOL_t _gdo_GDO_SYMBOL_ptr_ = NULL; \
    /**/
/***************************************************************************/



/***************************************************************************/
/* object pointers */
/***************************************************************************/
#define _$DECLARE_OBJECT_POINTERS \
    _$LINKAGE GDO_OBJ_TYPE *_gdo_GDO_OBJ_SYMBOL_ptr_ = NULL; \
    /**/
/***************************************************************************/



/***************************************************************************/
/* aliases */
/***************************************************************************/
#define GDO_SYMBOL _gdo_GDO_SYMBOL_ptr_
#define GDO_OBJ_SYMBOL *_gdo_GDO_OBJ_SYMBOL_ptr_
/***************************************************************************/



_$LINKAGE
void *load_library(const char *filename)
{
#ifdef _$WINAPI
    return (void *)LoadLibrary(filename);
#else
    return dlopen(filename, RTLD_LAZY);
#endif
}


_$LINKAGE
void *load_library_flags(const char *filename, int flags)
{
#ifdef _$WINAPI
    return (void *)LoadLibraryEx(filename, NULL, flags);
#else
    return dlopen(filename, flags);
#endif
}


_$LINKAGE
bool free_library(void *handle)
{
#ifdef _$WINAPI
    return (FreeLibrary((HMODULE)handle) == TRUE);
#else
    return (dlclose(handle) == 0);
#endif
}


_$LINKAGE
void *get_symbol(void *handle, const char *symbol)
{
#ifdef _$WINAPI
    return (void *)GetProcAddress((HMODULE)handle, symbol);
#else
    return dlsym(handle, symbol);
#endif
}
