/* declaration */
#ifdef GDO_STATIC
# define GDO_DECL  static inline
#else
# define GDO_DECL  extern
#endif


/**
 * If compiling for win32 and `_UNICODE` is defined and `GDO_USE_DLOPEN` is NOT
 * defined `gdo_char_t` will become `wchar_t`.
 */
#ifdef _GDO_TARGET_WIDECHAR
typedef wchar_t gdo_char_t;
#else
typedef char    gdo_char_t;
#endif


/**
 * Library and symbols handle
 */
typedef struct _gdo_handle
{
    gdo_char_t *filename;  /* copy of module filename */
    gdo_hmod_t  handle;    /* library handle */

    /* symbol pointers */
    struct _gdo_ptr {
        %%type%% (*%%func_symbol%%)(%%args%%);
        %%obj_type%% *%%obj_symbol%%;
    } ptr;

} gdo_handle_t;


/**
 * Plugin handle
 */
typedef struct _gdo_plugin
{
    size_t        num;   /* number of elements in `list' */
    gdo_handle_t *list;  /* list of library handles */

} gdo_plugin_t;


/**
 * Load from a list of plugin names.
 *
 * files:
 *   array of `gdo_char_t *' filenames to load
 *
 * num:
 *   number of entries in `files'
 *
 * Returns a pointer to a `gdo_plugin_t' array of `num' entries.
 * Always release the result with `gdo_release_plugins()'.
 *
 * To ensure loading was successful you must check the library handle and function
 * pointers of each entry of the returned `gdo_plugin.list' array.
 * If `files' is NULL or `num' is 0 a NULL pointer is returned.
 */
GDO_DECL gdo_plugin_t *gdo_load_plugins(const gdo_char_t **files, size_t num)
    GDO_GCC_ATTRIBUTE (warn_unused_result);


/**
 * Release plugins and free resources.
 *
 * plug:
 *   Pointer returned by `gdo_load_plugins()'.
 */
GDO_DECL void gdo_release_plugins(gdo_plugin_t *plug);

