/*****************************************************************************/
/*                            C plugin loader API                            */
/*****************************************************************************/

#include <string.h>

#ifdef GDO_WINAPI
# include <tchar.h>
#endif

#ifndef _tcsdup
# ifdef _WIN32
#  define _tcsdup _strdup
# else
#  define _tcsdup strdup
# endif
#endif

/* linkage */
#ifdef GDO_STATIC
# define GDO_LINKAGE  static inline
#else
# define GDO_LINKAGE  /**/
#endif

#ifdef GDO_WINAPI
# define GDO_LOAD_LIB(filename)       LoadLibraryEx(filename, NULL, GDO_DEFAULT_FLAGS)
# define GDO_FREE_LIB(handle)         FreeLibrary(handle)
# define GDO_GET_SYM(handle, symbol)  GetProcAddress(handle, symbol)
#else
# define GDO_LOAD_LIB(filename)       dlopen(filename, GDO_DEFAULT_FLAGS)
# define GDO_FREE_LIB(handle)         dlclose(handle)
# define GDO_GET_SYM(handle, symbol)  dlsym(handle, symbol)
#endif


/* load list of plugins */
GDO_LINKAGE gdo_plugin_t *gdo_load_plugins(const gdo_char_t **files, size_t num)
{
    if (num == 0) {
        return NULL;
    }

    gdo_plugin_t *plug = (gdo_plugin_t *)malloc(sizeof(gdo_plugin_t));
    plug->num = num;

    /* same as malloc(num * sizeof(gdo_plugin_t)) and memset() */
    plug->list = (gdo_handle_t *)calloc(num, sizeof(gdo_handle_t));

    for (size_t i = 0; i < num; i++) {
        if (!files[i] || *files[i] == 0) {
            /* empty filename */
            continue;
        }

        /* copy filename */
        plug->list[i].filename = _tcsdup(files[i]);

        /* load plugin */
        if ((plug->list[i].handle = GDO_LOAD_LIB(files[i])) == NULL) {
            continue;
        }
@
        /* load %%symbol%% */@
        plug->list[i].ptr.%%symbol%% =@
            (%%sym_type%%)@
                GDO_GET_SYM(plug->list[i].handle, "%%symbol%%");
    }

    return plug;
}


/* release plugins */
GDO_LINKAGE void gdo_release_plugins(gdo_plugin_t *_Nullable plug)
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

