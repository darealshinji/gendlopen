/*****************************************************************************/
/*                            C plugin loader API                            */
/*****************************************************************************/

#include <string.h>

#ifdef GDO_WINAPI
# include <tchar.h>
#endif

/* linkage */
#ifdef GDO_STATIC
# define GDO_LINKAGE  static inline
#else
# define GDO_LINKAGE  /**/
#endif

#ifdef GDO_WINAPI
# define GDO_LOAD_LIB(HANDLE, FLAGS)   LoadLibraryEx(HANDLE, NULL, FLAGS)
# define GDO_FREE_LIB(HANDLE)          FreeLibrary(HANDLE)
# define GDO_LOAD_SYM(HANDLE, SYMBOL)  (void *)GetProcAddress(HANDLE, SYMBOL)
# define GDO_STRDUP                    _tcsdup
#else
# define GDO_LOAD_LIB(HANDLE, FLAGS)   dlopen(HANDLE, FLAGS)
# define GDO_FREE_LIB(HANDLE)          dlclose(HANDLE)
# define GDO_LOAD_SYM(HANDLE, SYMBOL)  dlsym(HANDLE, SYMBOL)
# define GDO_STRDUP                    strdup
#endif


/* load list of plugins */
GDO_LINKAGE gdo_plugin_t *gdo_load_plugins(const gdo_char_t **files, size_t num)
{
    if (!files || num == 0) {
        return NULL;
    }

    /* allocate struct */
    gdo_plugin_t *plug = (gdo_plugin_t *)malloc(sizeof(gdo_plugin_t));
    plug->num = num;
    plug->list = (gdo_handle_t *)calloc(num, sizeof(gdo_handle_t));

    for (size_t i = 0; i < num; i++) {
        if (!files[i] || *files[i] == 0) {
            /* empty filename */
            continue;
        }

        /* copy filename first */
        plug->list[i].filename = GDO_STRDUP(files[i]);

        /* load plugin */
        plug->list[i].handle = GDO_LOAD_LIB(files[i], GDO_DEFAULT_FLAGS);

        if (!plug->list[i].handle) {
            continue;
        }
@
        /* load %%symbol%% */@
        plug->list[i].ptr.%%symbol%% =@
            (%%sym_type%%)@
                GDO_LOAD_SYM(plug->list[i].handle, "%%symbol%%");
    }

    return plug;
}


/* release plugins */
GDO_LINKAGE void gdo_release_plugins(gdo_plugin_t *plug)
{
    if (!plug) {
        return;
    }

    for (size_t i = 0; i < plug->num; i++) {
        free(plug->list[i].filename);

        if (plug->list[i].handle) {
            GDO_FREE_LIB(plug->list[i].handle);
        }
    }

    free(plug->list);
    free(plug);
}

