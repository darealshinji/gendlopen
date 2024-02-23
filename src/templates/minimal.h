
#ifdef GDO_WINAPI
#include <windows.h>
#else
#include <dlfcn.h>
#endif
#ifndef __cplusplus
#include <stdbool.h>
#endif

#if !defined(GDO_LINKAGE)
#define GDO_LINKAGE  static
#endif


/***************************************************************************/
/* typedefs */
/***************************************************************************/
#define GDO_DECLARE_TYPEDEFS \
    typedef GDO_TYPE (*gdo_GDO_SYMBOL_t)(GDO_ARGS); \
    /**/
/***************************************************************************/



/***************************************************************************/
/* function pointers */
/***************************************************************************/
#define GDO_DECLARE_FUNCTION_POINTERS \
    GDO_LINKAGE gdo_GDO_SYMBOL_t gdo_GDO_SYMBOL_ptr_ = NULL; \
    /**/
/***************************************************************************/



/***************************************************************************/
/* object pointers */
/***************************************************************************/
#define GDO_DECLARE_OBJECT_POINTERS \
    GDO_LINKAGE GDO_OBJ_TYPE *gdo_GDO_OBJ_SYMBOL_ptr_ = NULL; \
    /**/
/***************************************************************************/



/***************************************************************************/
/* aliases */
/***************************************************************************/
#define GDO_SYMBOL gdo_GDO_SYMBOL_ptr_
#define GDO_OBJ_SYMBOL *gdo_GDO_OBJ_SYMBOL_ptr_
/***************************************************************************/



inline GDO_LINKAGE
void *load_library(const char *filename)
{
#ifdef GDO_WINAPI
    return (void *)LoadLibraryExA(filename, NULL, 0);
#else
    return dlopen(filename, RTLD_LAZY);
#endif
}


inline GDO_LINKAGE
void *load_library_flags(const char *filename, int flags)
{
#ifdef GDO_WINAPI
    return (void *)LoadLibraryExA(filename, NULL, flags);
#else
    return dlopen(filename, flags);
#endif
}


inline GDO_LINKAGE
bool free_library(void *handle)
{
#ifdef GDO_WINAPI
    return (FreeLibrary((HMODULE)handle) == TRUE);
#else
    return (dlclose(handle) == 0);
#endif
}


inline GDO_LINKAGE
void *get_symbol(void *handle, const char *symbol)
{
#ifdef GDO_WINAPI
    return (void *)GetProcAddress((HMODULE)handle, symbol);
#else
    return dlsym(handle, symbol);
#endif
}
