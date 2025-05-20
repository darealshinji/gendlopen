#ifdef GDO_WINAPI
# include <wchar.h>
#endif

/***

******************
*   gdo_char_t   *
******************

If compiling for win32 and `_UNICODE` is defined and `GDO_USE_DLOPEN` is NOT defined
`gdo_char_t` will become `wchar_t`.

Otherwise `gdo_char_t` will become `char`.



**************************
*   Functions provided   *
**************************

gdo_plugin_t gdo_load_plugins (const gdo_char_t **files, size_t num);

    Load list of plugins. `files' is an array of `gdo_char_t *' filenames to load
    and `num' is the number of array entries.
    To ensure loading was successful you must check the library handle and function
    pointers of each entry of the returned `gdo_plugin.list' array.


void gdo_release_plugins (gdo_plugin_t handle);

    Release library handles and other resources.



****************************************************
* The following options may be set through macros: *
****************************************************

GDO_USE_DLOPEN
    If defined use `dlopen()' API on win32 targets.
    On other targets `dlopen()' is always used.

GDO_STATIC
    If defined `static inline' linkage is used for all functions.

GDO_DEFAULT_FLAGS
    Override the default flags to use when loading a library.



*****************
* Helper macros *
*****************

LIBEXT
LIBEXTA
LIBEXTW
    Shared library file extension without dot ("dll", "dylib" or "so").

***/


/* declaration */
#ifdef GDO_STATIC
# define GDO_DECL  static inline
#else
# define GDO_DECL  extern
#endif


/* char / wchar_t */
#if defined(GDO_WINAPI) && defined(_UNICODE)
typedef wchar_t gdo_char_t;
#else
typedef char    gdo_char_t;
#endif


/* our library and symbols handle */
typedef struct gdo_handle
{
    /* copy of module filename */
    gdo_char_t *filename;

    /* library handle */
#ifdef GDO_WINAPI
    HMODULE handle;
#else
    void *handle;
#endif

    /* symbol pointers */
    struct _gdo_ptr {
        %%type%% (*%%func_symbol%%)(%%args%%);
        %%obj_type%% *%%obj_symbol%%;
    } ptr;

} gdo_handle_t;


/* plugins handle */
typedef struct gdo_plugin
{
    size_t num;
    gdo_handle_t *list;

} gdo_plugin_t;


GDO_DECL gdo_plugin_t *gdo_load_plugins(const gdo_char_t **files, size_t num)
    GDO_ATTR (warn_unused_result);

GDO_DECL void gdo_release_plugins(gdo_plugin_t *handle);

