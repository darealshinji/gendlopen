#ifdef GDO_WINAPI
    #include <windows.h>
    #define GDO_LOAD_LIB(filename)       LoadLibraryExA(filename, NULL, 0)
    #define GDO_FREE_LIB(handle)         FreeLibrary(handle)
    #define GDO_GET_SYM(handle, symbol)  GetProcAddress(handle, symbol)
#else
    #include <dlfcn.h>
    #define GDO_LOAD_LIB(filename)       dlopen(filename, RTLD_LAZY)
    #define GDO_FREE_LIB(handle)         dlclose(handle)
    #define GDO_GET_SYM(handle, symbol)  dlsym(handle, symbol)
#endif

#ifndef __cplusplus
    #include <stdbool.h>
#endif

#ifdef GDO_STATIC
    #define GDO_LINKAGE  static
#else
    #define GDO_LINKAGE  /**/
#endif


/* aliases to raw function pointers */
#define GDO_SYMBOL gdo_hndl.GDO_SYMBOL_ptr_

/* aliases to raw object pointers */
#define GDO_OBJ_SYMBOL *gdo_hndl.GDO_OBJ_SYMBOL_ptr_


/* Our library and symbols handle */
typedef struct
{
#ifdef GDO_WINAPI
    HMODULE handle;
#else
    void *handle;
#endif

    GDO_TYPE (*GDO_SYMBOL_ptr_)(GDO_ARGS);
    GDO_OBJ_TYPE *GDO_OBJ_SYMBOL_ptr_;

} gdo_handle_t;

GDO_LINKAGE gdo_handle_t gdo_hndl = {0};


GDO_LINKAGE bool gdo_load_library_and_symbols(const char *filename)
{
    gdo_hndl.handle = GDO_LOAD_LIB(filename);

    if (!gdo_hndl.handle) {
        return false;
    }
@
    /* GDO_SYMBOL */@
    gdo_hndl.GDO_SYMBOL_ptr_ = @
        (GDO_TYPE (*)(GDO_ARGS))@
            GDO_GET_SYM(gdo_hndl.handle, "GDO_SYMBOL");@
    if (!gdo_hndl.GDO_SYMBOL_ptr_) {@
        GDO_FREE_LIB(gdo_hndl.handle);@
        return false;@
    }
@
    /* GDO_OBJ_SYMBOL */@
    gdo_hndl.GDO_OBJ_SYMBOL_ptr_ = (GDO_OBJ_TYPE *)@
        GDO_GET_SYM(gdo_hndl.handle, "GDO_OBJ_SYMBOL");@
    if (!gdo_hndl.GDO_OBJ_SYMBOL_ptr_) {@
        GDO_FREE_LIB(gdo_hndl.handle);@
        return false;@
    }

    return true;
}
