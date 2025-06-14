/*****************************************************************************/
/*                                   C API                                   */
/*****************************************************************************/

#ifdef GDO_WINAPI
# include <wchar.h>
#endif
#ifndef __cplusplus
# include <stdbool.h>
#endif


/* static/extern declarations */
#ifdef GDO_STATIC
# define GDO_DECL      static inline /* use `inline' to silence warnings */
# define GDO_OBJ_DECL  static
#else
# define GDO_DECL      extern
# define GDO_OBJ_DECL  extern
#endif


/**
 * If compiling for win32 and `_UNICODE` is defined and `GDO_USE_DLOPEN` is NOT
 * defined, then `gdo_char_t` will become `wchar_t`.
 */
#ifdef _GDO_TARGET_WIDECHAR
typedef wchar_t gdo_char_t;
#else
typedef char    gdo_char_t;
#endif


/**
 * Enumeration values for gdo_load_symbol()
 */
enum {
    GDO_LOAD_%%symbol%%,
    GDO_ENUM_LAST
};


#if defined(GDO_SEPARATE) && !defined(GDO_INCLUDED_IN_BODY)
/**
 * Library and symbols handle
 */
typedef struct _gdo_handle
{
    /* symbol pointers */
    struct _gdo_ptr {
        %%type%% (*%%func_symbol%%)(%%args%%);
        %%obj_type%% *%%obj_symbol%%;
    } ptr;

    /* private */

} gdo_handle_t;

GDO_OBJ_DECL gdo_handle_t gdo_hndl;
#endif


#ifdef GDO_DEFAULT_LIB
/**
 * Load the default library specified by the macro GDO_DEFAULT_LIB.
 *
 * On success `true' is returned.
 * On an error or if the library is already loaded the return value is `false'.
 */
GDO_DECL bool gdo_load_lib(void);
#endif


#ifdef GDO_DEFAULT_LIB
/**
 * Load the default library and all symbols.
 *
 * On success `true' is returned.
 * On an error or if the library is already loaded the return value is `false'.
 */
GDO_DECL bool gdo_load_lib_and_symbols(void);
#endif


/**
 * Load a library from a given filename.
 *
 * filename:
 *   Library filename or path to load. Must not be empty or NULL.
 *
 * On success `true' is returned.
 * On an error or if the library is already loaded the return value is `false'.
 */
GDO_DECL bool gdo_load_lib_name(const gdo_char_t *filename);


/**
 * Load a library using default flags and all symbols.
 *
 * filename:
 *   Library filename or path to load. Must not be empty or NULL.
 *
 * On success `true' is returned.
 * On an error or if the library is already loaded the return value is `false'.
 */
GDO_DECL bool gdo_load_lib_name_and_symbols(const gdo_char_t *filename);


/**
 * Load a library.
 *
 * filename:
 *   Library filename or path to load. Must not be empty or NULL.
 *
 * flags:
 *   These are passed to the underlying library loading functions.
 *
 * new_namespace:
 *   If true the library will be loaded into a new namespace using dlmopen().
 *   This argument is only used on Linux (if `_GNU_SOURCE' was defined) and
 *   Solaris/IllumOS.
 *
 * On success `true' is returned.
 * On an error or if the library is already loaded the return value is `false'.
 */
GDO_DECL bool gdo_load_lib_args(const gdo_char_t *filename, int flags, bool new_namespace);


/**
 * Returns `true' if the library was successfully loaded.
 */
GDO_DECL bool gdo_lib_is_loaded(void);


/**
 * Returns the flags used on the last attempt to load the library or zero.
 */
GDO_DECL int gdo_lib_flags(void);


/**
 * Free/release the library.
 *
 * Internal handle and pointers are only set back to NULL if the underlying calls
 * were successful, in which case `true' is returned.
 * Return value is also `true' if no library was loaded.
 */
GDO_DECL bool gdo_free_lib(void);


/**
 * Free/release the library.
 * Don't check if the underlying calls were successful.
 *
 * Internal handle and pointers are always set back to NULL.
 * Can safely be called even if no library was loaded.
 */
GDO_DECL void gdo_force_free_lib(void);


/**
 * Registers the function `gdo_force_free_lib()' to be called upon the program's exit.
 * It's recommended to do this before you attempt to load anything.
 *
 * It returns `false' if registering the function wasn't successful.
 * On success or if a function was already registered the return value is `true'.
 */
GDO_DECL bool gdo_enable_autorelease(void);


/**
 * Load all symbols. Returns `true' on success.
 * If all symbols were already loaded, nothing is done and the return value is `true'.
 */
GDO_DECL bool gdo_load_all_symbols(void);


/**
 * Load a specific symbol from an enum value.
 *
 * symbol_num:
 *   Auto-generated enumeration value `GDO_LOAD_<symbol_name>'.
 *   For example use `GDO_LOAD_foo' to load the symbol `foo'.
 *
 * Returns `true' on success or if the symbol was already loaded.
 */
GDO_DECL bool gdo_load_symbol(int symbol_num);


/**
 * Load a specific symbol.
 *
 * symbol:
 *   Name of the symbol to load.
 *
 * Returns `true' on success or if the symbol was already loaded.
 */
GDO_DECL bool gdo_load_symbol_name(const char *symbol);


/**
 * Returns true if ALL symbols were loaded.
 */
GDO_DECL bool gdo_all_symbols_loaded(void);


/**
 * Returns true if NO symbols were loaded.
 */
GDO_DECL bool gdo_no_symbols_loaded(void);


/**
 * Returns true if one or more symbols were loaded.
 */
GDO_DECL bool gdo_any_symbol_loaded(void);


/**
 * Returns a pointer to the error message buffer with the last saved error string.
 * The message will indicate if no error had occured.
 * This function doesn't return a null pointer or empty string.
 * Do not free the returned pointer!
 */
GDO_DECL const gdo_char_t *gdo_last_error(void)
    GDO_ATTR (returns_nonnull);


/**
 * Return the full library path. The returned string MUST be deallocated with free().
 * On error or if no library was loaded NULL is returned.
 *
 * On some systems and configurations the path is taken from the loaded symbols
 * in which case at least one symbol must have been successfully loaded before
 * using this function.
 */
GDO_DECL gdo_char_t *gdo_lib_origin(void)
    GDO_ATTR (warn_unused_result);


/**
 * Prefixed aliases, useful if GDO_DISABLE_ALIASING was defined.
 */
#define GDO_ALIAS_%%func_symbol_pad%% gdo_hndl.ptr.%%func_symbol%%
#define GDO_ALIAS_%%obj_symbol_pad%% *gdo_hndl.ptr.%%obj_symbol%%


/**
 * Disable aliasing if we saved into separate files and the
 * header file was included from the body file.
 */
#if defined(GDO_SEPARATE) && !defined(GDO_INCLUDED_IN_BODY)

/**
 * Aliases to raw function pointers
 */
#if !defined(GDO_DISABLE_ALIASING) && !defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)
#define %%func_symbol_pad%% GDO_ALIAS_%%func_symbol%%
#endif

/**
 * Aliases to raw object pointers
 */
#if !defined(GDO_DISABLE_ALIASING)
#define %%obj_symbol_pad%% GDO_ALIAS_%%obj_symbol%%
#endif

#endif //GDO_SEPARATE && !GDO_INCLUDED_IN_BODY

