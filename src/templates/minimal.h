#ifdef GDO_USE_SDL
/* SDL API */
# include <SDL_loadso.h>
# define GDO_LOAD_LIB(filename)       SDL_LoadObject(filename)
# define GDO_FREE_LIB(handle)         SDL_UnloadObject(handle)
# define GDO_GET_SYM(handle, symbol)  SDL_LoadFunction(handle, symbol)
#elif defined(_WIN32) && !defined(GDO_USE_DLOPEN)
/* WinAPI */
# include <windows.h>
# define GDO_LOAD_LIB(filename)       (void *)LoadLibraryA(filename)
# define GDO_FREE_LIB(handle)         FreeLibrary((HMODULE)handle)
# define GDO_GET_SYM(handle, symbol)  (void *)GetProcAddress((HMODULE)handle, symbol)
#else
/* POSIX */
# include <dlfcn.h>
# define GDO_LOAD_LIB(filename)       dlopen(filename, RTLD_LAZY)
# define GDO_FREE_LIB(handle)         dlclose(handle)
# define GDO_GET_SYM(handle, symbol)  dlsym(handle, symbol)
#endif

#ifdef GDO_STATIC
# define GDO_LINKAGE  static
#else
# define GDO_LINKAGE  /**/
#endif


/* Our library and symbols handle */
typedef struct gdo_handle
{
    void *handle;

    %%type%% (*%%func_symbol%%)(%%args%%);
    %%obj_type%% *%%obj_symbol%%;

} gdo_handle_t;

GDO_LINKAGE gdo_handle_t gdo_hndl;


/* returns NULL on success and an error message if loading failed */
GDO_LINKAGE const char *gdo_load_library_and_symbols(const char *filename)
{
    gdo_hndl.handle = GDO_LOAD_LIB(filename);

    if (!gdo_hndl.handle) {
        return "failed to load library";
    }
@
    /* %%symbol%% */@
    gdo_hndl.%%symbol%% =@
        (%%sym_type%%)@
            GDO_GET_SYM(gdo_hndl.handle, "%%symbol%%");@
    if (!gdo_hndl.%%symbol%%) {@
        GDO_FREE_LIB(gdo_hndl.handle);@
        return "failed to load symbol: %%symbol%%";@
    }

    return NULL;
}

/* free library handle, no error checks */
GDO_LINKAGE void gdo_free_library(void)
{
    GDO_FREE_LIB(gdo_hndl.handle);
}


#if !defined(GDO_NOALIAS)

/* aliases to raw symbol pointers */
#define %%func_symbol%%  gdo_hndl.%%func_symbol%%
#define %%obj_symbol%%  *gdo_hndl.%%obj_symbol%%

#endif // !GDO_NOALIAS
