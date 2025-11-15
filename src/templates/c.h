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
    /* symbol pointers */
    struct _gdo_ptr {
        %%type%% (*%%func_symbol%%)(%%args%%);
        %%obj_type%% *%%obj_symbol%%;
    } ptr;

    /* private */
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
%PARAM_SKIP_REMOVE_BEGIN%


/*****************************************************************************/
/*                                wrap code                                  */
/*****************************************************************************/
#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

/* #define empty hooks by default */
#ifndef GDO_HOOK_%%func_symbol%%@
#define GDO_HOOK_%%func_symbol%%(...) /**/@
#endif


/* set visibility of wrapped functions */
#ifdef GDO_WRAP_IS_VISIBLE
/* visible as regular functions */
# define GDO_WRAP_DECL  /**/
# define GDO_WRAP(x)    x
# else
/* declare as prefixed inline functions by default */
# define GDO_WRAP_DECL  static inline
# define GDO_WRAP(x)    GDO_WRAP_##x
#endif


GDO_DECL void _gdo_wrap_check(int load, const gdo_char_t *sym);

#ifdef GDO_ENABLE_AUTOLOAD
# define GDO_WRAP_LOAD(x)  GDO_LOAD_##x
#else
# define GDO_WRAP_LOAD(x)  (gdo_hndl.ptr.x != NULL)
#endif


/* create a wrapper function */
#define GDO_MAKE_FUNCTION(RETURN, TYPE, SYMBOL, ARGS, ...) \
    GDO_WRAP_DECL \
    TYPE GDO_WRAP(SYMBOL) ARGS { \
        /* puts("DEBUG: wrapper function called"); */ \
        _gdo_wrap_check(GDO_WRAP_LOAD(SYMBOL), GDO_T(#SYMBOL)); \
        GDO_HOOK_##SYMBOL(__VA_ARGS__); \
        RETURN gdo_hndl.ptr.SYMBOL(__VA_ARGS__); \
    }

/**
 * create a GNU inline wrapper function for use with variable arguments
 * https://gcc.gnu.org/onlinedocs/gcc/Constructing-Calls.html
 * https://gcc.gnu.org/onlinedocs/gcc/Inline.html
 */
#define GDO_MAKE_VA_ARG_FUNCTION(RETURN, TYPE, SYMBOL, ARGS, ...) \
    extern inline __attribute__((__gnu_inline__)) \
    TYPE GDO_WRAP(SYMBOL) ARGS { \
        /* puts("DEBUG: GNU inline wrapper function called"); */ \
        _gdo_wrap_check(GDO_WRAP_LOAD(SYMBOL), GDO_T(#SYMBOL)); \
        GDO_HOOK_##SYMBOL(__VA_ARGS__, __builtin_va_arg_pack()); \
        RETURN gdo_hndl.ptr.SYMBOL(__VA_ARGS__, __builtin_va_arg_pack()); \
    }


/* diagnostic warnings on variable arguments functions */
#if !defined(GDO_DISABLE_WARNINGS)

/* %%func_symbol%%() */@
#ifdef GDO_HAS_VA_ARGS_%%func_symbol%%@
# ifdef GDO_HAS_BUILTIN_VA_ARG_PACK@
#  ifdef GDO_WRAP_IS_VISIBLE@
GDO_PRAGMA_WARNING("GDO_WRAP_IS_VISIBLE defined but wrapper function %%func_symbol%%() can only be used inlined")@
#  elif defined(__NO_INLINE__)@
GDO_PRAGMA_WARNING("inlining is required to use variable arguments wrapper for %%func_symbol%%()")@
#  endif@
# else //!GDO_HAS_BUILTIN_VA_ARG_PACK@
GDO_PRAGMA_WARNING("__builtin_va_arg_pack() required to use variable arguments wrapper for %%func_symbol%%()")@
# endif@
#endif@

#endif //!GDO_DISABLE_WARNINGS

@
/* %%func_symbol%%() */@
#ifdef GDO_HAS_VA_ARGS_%%func_symbol%%@
# ifdef GDO_HAS_BUILTIN_VA_ARG_PACK@
    GDO_MAKE_VA_ARG_FUNCTION(%%return%%, %%type%%,@
        %%func_symbol%%, (%%args%%),@
        %%param_names%%)@
# endif@
#else@
    GDO_MAKE_FUNCTION(%%return%%, %%type%%,@
        %%func_symbol%%, (%%args%%),@
        %%param_names%%)@
#endif //!GDO_HAS_VA_ARGS_%%func_symbol%%@


#undef GDO_WRAP_DECL
#undef GDO_WRAP
#undef GDO_WRAP_LOAD
#undef GDO_MAKE_FUNCTION
#undef GDO_MAKE_VA_ARG_FUNCTION

#endif //GDO_WRAP_FUNCTIONS ...
/***************************** end of wrap code ******************************/
%PARAM_SKIP_END%


/**
 * Set function name alias prefix.
 */
#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)
# define GDO_FUNC_ALIAS(x) GDO_WRAP_##x
#else
# define GDO_FUNC_ALIAS(x) GDO_ALIAS_##x
#endif


/**
 * Disable aliasing if we saved into separate files and the
 * header file was included from the body file.
 */
#if  defined(GDO_SEPARATE) && \
    !defined(GDO_INCLUDED_IN_BODY) && \
    !defined(GDO_DISABLE_ALIASING)

/* aliases to raw function pointers */
#if !defined(GDO_WRAP_IS_VISIBLE)
# define %%func_symbol_pad%% GDO_FUNC_ALIAS(%%func_symbol%%)
#endif

/* aliases to raw object pointers */
#define %%obj_symbol_pad%% GDO_ALIAS_%%obj_symbol%%

#endif //GDO_SEPARATE ...

