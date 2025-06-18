#if defined(_WIN32) && !defined(GDO_USE_DLOPEN)
# define GDO_WINAPI
# include <windows.h>
#else
# include <dlfcn.h>
#endif

#ifdef GDO_STATIC
# define GDO_LINKAGE  static inline
#else
# define GDO_LINKAGE  /**/
#endif


/**
 * Library and symbols handle
 */
typedef struct gdo_handle
{
    /* symbol pointers */
    struct _gdo_ptr {
        %%type%% (*%%func_symbol%%)(%%args%%);
        %%obj_type%% *%%obj_symbol%%;
    } ptr;

#ifdef GDO_WINAPI
    HMODULE handle;
#else
    void *handle;
#endif

} gdo_handle_t;

GDO_LINKAGE gdo_handle_t gdo_hndl;


/**
 * Free library handle without error checks.
 * Internal handle and pointers are always set back to NULL.
 */
GDO_LINKAGE void gdo_free_library(void)
{
    if (gdo_hndl.handle) {
#ifdef GDO_WINAPI
        FreeLibrary(gdo_hndl.handle);
#else
        dlclose(gdo_hndl.handle);
#endif
    }

    gdo_hndl.handle = NULL;
    gdo_hndl.ptr.%%symbol%% = NULL;
}


/**
 * Load library and all symbols.
 *
 * filename:
 *   Library filename or path to load. Must not be empty or NULL.
 *
 * Returns NULL on success and an error message if loading has failed.
 * Library handle is always freed on an error.
 */
GDO_LINKAGE const char *gdo_load_library_and_symbols(const char *filename)
{
    if (!filename) {
        return "filename is <NULL>";
    } else if (!*filename) {
        return "filename is empty";
    }

    /* load library */
#ifdef GDO_WINAPI
    gdo_hndl.handle = LoadLibraryA(filename);
#elif defined(_AIX)
    gdo_hndl.handle = dlopen(filename, RTLD_LAZY | RTLD_MEMBER);
#else
    gdo_hndl.handle = dlopen(filename, RTLD_LAZY);
#endif

    if (!gdo_hndl.handle) {
        return "failed to load library";
    }

    /* load symbols */
#ifdef GDO_WINAPI
# define _GDO_LOAD_SYM(SYMBOL)  (void *)GetProcAddress(gdo_hndl.handle, SYMBOL)
#else
# define _GDO_LOAD_SYM(SYMBOL)  dlsym(gdo_hndl.handle, SYMBOL)
#endif
@
    /* %%symbol%% */@
    gdo_hndl.ptr.%%symbol%% =@
        (%%sym_type%%)@
            _GDO_LOAD_SYM("%%symbol%%");@
    if (!gdo_hndl.ptr.%%symbol%%) {@
        gdo_free_library();@
        return "failed to load symbol: %%symbol%%";@
    }

#undef _GDO_LOAD_SYM

    return NULL;
}


#if !defined(GDO_DISABLE_ALIASING)

/**
 * Aliases to raw symbol pointers
 */
#define %%func_symbol_pad%%  gdo_hndl.ptr.%%func_symbol%%
#define %%obj_symbol_pad%%  *gdo_hndl.ptr.%%obj_symbol%%

#endif // !GDO_DISABLE_ALIASING

