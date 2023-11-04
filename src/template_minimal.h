#ifdef GDO_WINAPI
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#ifndef __cplusplus
#include <stdbool.h>
#endif

#if !defined(GDO_LINKAGE)
#define GDO_LINKAGE  inline static
#endif

GDO_TYPEDEFS


/***************************************************************************/
/* typedefs */
/***************************************************************************/
#define GDO_DECLARE_TYPEDEFS \
    typedef GDO_TYPE (*_gdo_GDO_SYMBOL_t)(GDO_ARGS); \
    /**/
/***************************************************************************/



/***************************************************************************/
/* function pointers */
/***************************************************************************/
#define GDO_DECLARE_FUNCTION_POINTERS \
    GDO_LINKAGE _gdo_GDO_SYMBOL_t _gdo_GDO_SYMBOL_ptr_ = NULL; \
    /**/
/***************************************************************************/



/***************************************************************************/
/* object pointers */
/***************************************************************************/
#define GDO_DECLARE_OBJECT_POINTERS \
    GDO_LINKAGE GDO_OBJ_TYPE *_gdo_GDO_OBJ_SYMBOL_ptr_ = NULL; \
    /**/
/***************************************************************************/



/***************************************************************************/
/* aliases */
/***************************************************************************/
#define GDO_SYMBOL _gdo_GDO_SYMBOL_ptr_
#define GDO_OBJ_SYMBOL *_gdo_GDO_OBJ_SYMBOL_ptr_
/***************************************************************************/



GDO_LINKAGE
void *load_library(const char *filename)
{
#ifdef GDO_WINAPI
    return (void *)LoadLibrary(filename);
#else
    return dlopen(filename, RTLD_LAZY);
#endif
}


GDO_LINKAGE
void *load_library_flags(const char *filename, int flags)
{
#ifdef GDO_WINAPI
    return (void *)LoadLibraryEx(filename, NULL, flags);
#else
    return dlopen(filename, flags);
#endif
}


GDO_LINKAGE
bool free_library(void *handle)
{
#ifdef GDO_WINAPI
    return (FreeLibrary((HMODULE)handle) == TRUE);
#else
    return (dlclose(handle) == 0);
#endif
}


GDO_LINKAGE
void *get_symbol(void *handle, const char *symbol)
{
#ifdef GDO_WINAPI
    return (void *)GetProcAddress((HMODULE)handle, symbol);
#else
    return dlsym(handle, symbol);
#endif
}
