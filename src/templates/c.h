#ifdef GDO_WINAPI
# include <wchar.h>
#endif
#ifndef __cplusplus
# include <stdbool.h>
#endif

/***

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

GDO_DEFAULT_LIB
    Set a default library name through this macro (including double quote
    marks). This macro must be defined if you want to set GDO_ENABLE_AUTOLOAD
    or if you want to use the `gdo_load_lib()' function.

GDO_WRAP_FUNCTIONS
    Use actual wrapped functions instead of a name alias. This is useful if you
    want to create a library to later link an application against.
    These functions print an error message and call `exit(1)' if they were called
    and the library and symbols weren't loaded properly.

GDO_ENABLE_AUTOLOAD
    Define this macro if you want to use auto-loading wrapper functions.
    This means you don't need to explicitly call library load functions.
    The first wrapper function called will load all symbols at once.
    It requires GDO_DEFAULT_LIB to be defined.
    If an error occures during loading these functions print an error message
    and call `exit(1)'!
    The library handle will be freed automatically on program exit.

GDO_DELAYLOAD
    Same as GDO_ENABLE_AUTOLOAD but only the requested symbol is loaded when its
    wrapper function is called instead of all symbols.
    It requires GDO_ENABLE_AUTOLOAD to be defined.

GDO_VISIBILITY
    You can set the symbol visibility of wrapped functions (enabled with GDO_WRAP_FUNCTIONS)
    using this macro.

GDO_USE_MESSAGE_BOX
    Windows only: if GDO_ENABLE_AUTOLOAD was activated this will enable
    error messages from auto-loading to be displayed in MessageBox windows.

GDO_DISABLE_ALIASING
    Don't use preprocessor macros to alias symbol names. Use with care.

GDO_DISABLE_DLINFO
    Always disable usage of `dlinfo(3)' to retrieve the library path.
    `dladdr(3)' will be used instead.
    Note: on Linux you need to define _GNU_SOURCE to enable `dlinfo(3)'.

GDO_DISABLE_DLMOPEN
    Always disable usage of `dlmopen(3)'.
    Note: on Linux you need to define _GNU_SOURCE to enable `dlmopen(3)'.


*********
* Hooks *
*********

GDO_HOOK_<function>(...)
    Define a hook macro that will be inserted into a wrap function.
    The hook is placed before the actual function call.
    If you want to call the function inside the macro you must do so using the GDO_ALIAS_* prefix.
    Parameter names are taken from the function prototype declarations (or it's "a, b, c" and so on
    if the header was created with `-param=create'). A hook may be left undefined.

    For example if a function declaration is `int sum_of_a_and_b(int val_a, int val_b)':

    #define GDO_HOOK_sum_of_a_and_b(...) \
      printf("debug: the sum of %d and %d is %d\n", \
        val_a, val_b, GDO_ALIAS_sum_of_a_and_b(__VA_ARGS__));

***/


/*****************************************************************************/
/*                                   C API                                   */
/*****************************************************************************/


/* declaration */
#ifdef GDO_STATIC
# define GDO_DECL      static inline
# define GDO_OBJ_DECL  static
#else
# define GDO_DECL      extern
# define GDO_OBJ_DECL  extern
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


/* our library and symbols handle */
typedef struct gdo_handle
{
#ifdef GDO_WINAPI
    HMODULE handle;
    DWORD last_errno;
    /* FormatMessage: according to MSDN the maximum message length is 64k */
    gdo_char_t buf_formatted[64*1024];
#else
    void *handle;
#endif

    int flags;
    bool free_lib_registered;

#ifdef _WIN32
    gdo_char_t buf[64*1024];
#else
    gdo_char_t buf[8*1024];
#endif

    /* symbols */
    struct _gdo_ptr {
        %%type%% (*%%func_symbol%%)(%%args%%);
        %%obj_type%% *%%obj_symbol%%;
    } ptr;

} gdo_handle_t;

GDO_OBJ_DECL gdo_handle_t gdo_hndl;


/* enumeration values for gdo_load_symbol() */
enum {
    GDO_LOAD_%%symbol%%,
    GDO_ENUM_LAST
};


#ifdef GDO_DEFAULT_LIB
/**
 * Load the default library specified by the macro GDO_DEFAULT_LIB using default flags.
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
 * Load a library using default flags.
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
 *   If true the library will be loaded into a new namespace.
 *   This is done using dlmopen() with the LM_ID_NEWLM argument.
 *   This argument is only used on Glibc and if _GNU_SOURCE was defined,
 *   it has no effect otherwise.
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
 * Free/release the library. Internal handle and pointers are set back to NULL
 * if the underlying calls were successful, in which case `true' is returned.
 * Return value is `true' if no library was loaded.
 */
GDO_DECL bool gdo_free_lib(void);


/**
 * Free/release the library. Don't check if the underlying calls were successful.
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


/* prefixed aliases, useful if GDO_DISABLE_ALIASING was defined */
#define GDO_ALIAS_%%func_symbol_pad%% gdo_hndl.ptr.%%func_symbol%%
#define GDO_ALIAS_%%obj_symbol_pad%% *gdo_hndl.ptr.%%obj_symbol%%


/* disable aliasing if we saved into separate files and the
 * header file was included from the body file */
#if defined(GDO_SEPARATE) && !defined(GDO_INCLUDED_IN_BODY)

/* aliases to raw function pointers */
#if !defined(GDO_DISABLE_ALIASING) && !defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)
#define %%func_symbol_pad%% GDO_ALIAS_%%func_symbol%%
#endif

/* aliases to raw object pointers */
#if !defined(GDO_DISABLE_ALIASING)
#define %%obj_symbol_pad%% GDO_ALIAS_%%obj_symbol%%
#endif

#endif //GDO_SEPARATE && !GDO_INCLUDED_IN_BODY

