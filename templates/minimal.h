/* whether to use WinAPI */
#if defined(_WIN32) && !defined(GDO_USE_DLOPEN)
    #define GDO_WINAPI
#endif

#ifdef GDO_WINAPI
    #include <windows.h>
    #define GDO_LOAD_LIB(filename)       LoadLibraryExA(filename, NULL, 0)
    #define GDO_FREE_LIB(handle)         FreeLibrary(handle)
    #define GDO_GET_SYM(handle, symbol)  ((void *)GetProcAddress(handle, symbol))
#else
    #include <dlfcn.h>
    #define GDO_LOAD_LIB(filename)       dlopen(filename, RTLD_LAZY)
    #define GDO_FREE_LIB(handle)         dlclose(handle)
    #define GDO_GET_SYM(handle, symbol)  dlsym(handle, symbol)
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


/* returns NULL on success and an error message if loading failed */
GDO_LINKAGE const char *gdo_load_library_and_symbols(const char *filename)
{
    gdo_hndl.handle = GDO_LOAD_LIB(filename);

    if (!gdo_hndl.handle) {
        return "failed to load library";
    }
@
    /* GDO_SYMBOL */@
    gdo_hndl.GDO_SYMBOL_ptr_ = @
        (GDO_TYPE (*)(GDO_ARGS))@
            GDO_GET_SYM(gdo_hndl.handle, "GDO_SYMBOL");@
    if (!gdo_hndl.GDO_SYMBOL_ptr_) {@
        GDO_FREE_LIB(gdo_hndl.handle);@
        return "failed to load symbol: GDO_SYMBOL";@
    }
@
    /* GDO_OBJ_SYMBOL */@
    gdo_hndl.GDO_OBJ_SYMBOL_ptr_ = (GDO_OBJ_TYPE *)@
        GDO_GET_SYM(gdo_hndl.handle, "GDO_OBJ_SYMBOL");@
    if (!gdo_hndl.GDO_OBJ_SYMBOL_ptr_) {@
        GDO_FREE_LIB(gdo_hndl.handle);@
        return "failed to load symbol: GDO_OBJ_SYMBOL";@
    }

    return NULL;
}
