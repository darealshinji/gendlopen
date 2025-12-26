/*****************************************************************************/
/*                                   C API                                   */
/*****************************************************************************/

#ifdef _GDO_TARGET_WIDECHAR
# include <wchar.h>
#endif
#ifndef __cplusplus
# include <stdbool.h>
#endif

#ifdef GDO_WINAPI
# include <tchar.h>
# define GDO_T(x) _T(x)
#else
# define GDO_T(x)    x
#endif


#ifdef _WIN32
/* FormatMessage: maximum message length according to MSDN */
# define GDO_BUFLEN (64*1024)
#else
/* Linux MAX_PATH*2 */
# define GDO_BUFLEN (8*1024)
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


/**
 * Library and symbols handle
 */
typedef struct _gdo_handle
{
#ifdef GDO_WINAPI
    HMODULE handle;
    DWORD last_errno;
    gdo_char_t buf_formatted[GDO_BUFLEN];
#else
    void *handle;
#endif

    gdo_char_t buf[GDO_BUFLEN];

    int flags;
    bool free_lib_registered;

    /* symbol pointers; symbol names must be prefixed to avoid macro expansion */
    %%type%% (*_GDO_PTR_%%func_symbol%%)(%%args%%);
    %%obj_type%% *_GDO_PTR_%%obj_symbol%%;

} gdo_handle_t;

GDO_OBJ_DECL gdo_handle_t gdo_hndl;


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
#ifdef GDO_DEFAULT_LIB
GDO_DECL bool gdo_load_lib(void);
GDO_DECL bool gdo_load_lib_and_symbols(void);
#endif
GDO_DECL bool gdo_load_lib_name(const gdo_char_t *filename);
GDO_DECL bool gdo_load_lib_name_and_symbols(const gdo_char_t *filename);
GDO_DECL bool gdo_load_lib_args(const gdo_char_t *filename, int flags, bool new_namespace);


/**
 * Returns `true' if the library was successfully loaded.
 */
GDO_DECL bool gdo_lib_is_loaded(void);


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
 * Load symbols.
 *
 * symbol_num:
 *   Auto-generated enumeration value `GDO_LOAD_<symbol_name>'.
 *   For example use `GDO_LOAD_foo' to load the symbol `foo'.
 *
 * symbol:
 *   Name of the symbol to load.
 *
 * Returns `true' on success or if the symbol was already loaded.
 */
GDO_DECL bool gdo_load_all_symbols(void);
GDO_DECL bool gdo_load_symbol(int symbol_num);
GDO_DECL bool gdo_load_symbol_name(const char *symbol);


/**
 * Check if symbols were loaded.
 */
GDO_DECL bool gdo_all_symbols_loaded(void);
GDO_DECL bool gdo_no_symbols_loaded(void);
GDO_DECL bool gdo_any_symbol_loaded(void);


/**
 * Returns a pointer to the error message buffer with the last saved error string.
 * The message will indicate if no error had occured.
 * This function doesn't return a null pointer or empty string.
 * Do not free the returned pointer!
 */
GDO_DECL const gdo_char_t *gdo_last_error(void)
    GDO_GCC_ATTRIBUTE (returns_nonnull);


/**
 * Return the full library path. The returned string MUST be deallocated with free().
 * On error or if no library was loaded NULL is returned.
 *
 * On some systems and configurations the path is taken from the loaded symbols
 * in which case at least one symbol must have been successfully loaded before
 * using this function.
 */
GDO_DECL gdo_char_t *gdo_lib_origin(void)
    GDO_GCC_ATTRIBUTE (warn_unused_result);


/**
 * Prefixed aliases, useful if GDO_DISABLE_ALIASING was defined.
 */
#define GDO_RAWPTR_%%func_symbol_pad%% gdo_hndl._GDO_PTR_%%func_symbol%%
#define GDO_RAWPTR_%%obj_symbol_pad%% gdo_hndl._GDO_PTR_%%obj_symbol%%
%PARAM_SKIP_REMOVE_BEGIN%


/*****************************************************************************/
/*                                wrap code                                  */
/*****************************************************************************/
#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

/* #define empty hooks by default */
#ifndef GDO_HOOK_%%func_symbol%%@
#define GDO_HOOK_%%func_symbol%%(...) /**/@
#endif


GDO_DECL void _gdo_wrap_check(int load, bool sym_loaded, const gdo_char_t *sym);


/* diagnostic warnings on variable arguments functions */
#if !defined(GDO_DISABLE_WARNINGS)

/* %%func_symbol%%() */@
#ifdef GDO_HAS_VA_ARGS_%%func_symbol%%@
# ifdef GDO_HAS_BUILTIN_VA_ARG_PACK@
#  ifdef GDO_WRAP_VISIBILITY@
GDO_WARNING("GDO_WRAP_VISIBILITY defined but wrapper function %%func_symbol%%() can only be used inlined; use GDO_DISABLE_WARNINGS to silence this warning")@
#  elif defined(__NO_INLINE__)@
GDO_WARNING("inlining is required to use variable arguments wrapper for %%func_symbol%%(); use GDO_DISABLE_WARNINGS to silence this warning")@
#  endif@
# else@
GDO_WARNING("__builtin_va_arg_pack() required to use variable arguments wrapper for %%func_symbol%%(); use GDO_DISABLE_WARNINGS to silence this warning")@
# endif@
#endif@

#endif //!GDO_DISABLE_WARNINGS


/**
 * GNU inline wrapper function for use with variable arguments
 * https://gcc.gnu.org/onlinedocs/gcc/Constructing-Calls.html
 * https://gcc.gnu.org/onlinedocs/gcc/Inline.html
 *
 * __builtin_va_arg_pack() is a GNU extension and will be resolved to the
 * additional parameters provided by the `...' argument.
 */
@
/* %%func_symbol%%() */@
#ifdef GDO_HAS_VA_ARGS_%%func_symbol%%@
# ifdef GDO_HAS_BUILTIN_VA_ARG_PACK@
    extern inline GDO_GCC_ATTRIBUTE(gnu_inline)@
    %%type%% GDO_WRAP(%%func_symbol%%) (%%args%%) {@
        /* puts("DEBUG: %%func_symbol%%: GNU inline wrapper function called"); */@
        const bool sym_loaded = (GDO_RAWPTR_%%func_symbol%% != NULL);@
        _gdo_wrap_check( GDO_LOAD_%%func_symbol%%, sym_loaded, GDO_T("%%func_symbol%%") );@
        GDO_HOOK_%%func_symbol%%( %%param_names%%, __builtin_va_arg_pack() );@
        %%return%% GDO_RAWPTR_%%func_symbol%%( %%param_names%%, __builtin_va_arg_pack() );@
    }@
# endif@
#else@
    GDO_WRAP_DECL@
    %%type%% GDO_WRAP(%%func_symbol%%) (%%args%%) {@
        /* puts("DEBUG: %%func_symbol%%: wrapper function called"); */@
        const bool sym_loaded = (GDO_RAWPTR_%%func_symbol%% != NULL);@
        _gdo_wrap_check( GDO_LOAD_%%func_symbol%%, sym_loaded, GDO_T("%%func_symbol%%") );@
        GDO_HOOK_%%func_symbol%%( %%param_names%% );@
        %%return%% GDO_RAWPTR_%%func_symbol%%( %%param_names%% );@
    }@
#endif //!GDO_HAS_VA_ARGS_%%func_symbol%%

#endif //GDO_WRAP_FUNCTIONS ...
/***************************** end of wrap code ******************************/
%PARAM_SKIP_END%


/**
 * Set function name alias prefix.
 */
#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)
# define GDO_FUNC_ALIAS(x) GDO_WRAP_##x
#else
# define GDO_FUNC_ALIAS(x) GDO_RAWPTR_##x
#endif


/**
 * Disable aliasing if we saved into separate files and the
 * header file was included from the body file.
 */
#if  defined(GDO_SEPARATE) && \
    !defined(GDO_INCLUDED_IN_BODY) && \
    !defined(GDO_DISABLE_ALIASING)

/* aliases to raw function pointers */
#if !defined(GDO_WRAP_VISIBILITY)
# define %%func_symbol_pad%% GDO_FUNC_ALIAS(%%func_symbol%%)
#endif

/* aliases to raw object pointers */
#define %%obj_symbol_pad%% *GDO_RAWPTR_%%obj_symbol%%

#endif //GDO_SEPARATE ...

