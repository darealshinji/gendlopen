/*****************************************************************************/
/*                                   C API                                   */
/*****************************************************************************/

#ifdef _WIN32
# include <tchar.h>
#else
# undef _T
# define _T(x) x
#endif

/* static/extern declarations */
#ifdef GDO_STATIC
# define GDO_DECL      static inline /* use `inline' to silence compiler warnings */
# define GDO_OBJ_DECL  static
#else
# define GDO_DECL      extern
# define GDO_OBJ_DECL  extern
#endif

#ifdef _GDO_TARGET_WIDECHAR
typedef wchar_t gdo_char_t;
#else
typedef char    gdo_char_t;
#endif


/**
 * Enumeration values for use with gdo_load_symbol()
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
    gdo_hmod_t handle;        /* handle returned by dlopen()/LoadLibraryEx() */
#ifdef GDO_WINAPI
    DWORD      last_errno;    /* value returned by GetLastError() */
#endif
    bool       free_lib_reg;  /* whether registering the function to automatically */
                              /* free the library upon exit was successful */

    /* symbol pointers; symbol names MUST be prefixed to avoid macro expansion */
    %%type%% (*GDO_PTR_%%func_symbol%%)(%%args%%);
    %%obj_type%% *GDO_PTR_%%obj_symbol%%;

    gdo_char_t errbuf[GDO_BUFLEN]; /* buffer for error messages */

#ifdef GDO_WINAPI
    /**
     * FormatMessage() saves the formatted message string in this buffer;
     * MSDN says this buffer cannot be larger than 64K bytes (not characters!)
     */
    gdo_char_t formatted[(64*1024) / sizeof(gdo_char_t)];
#endif
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
 *   This argument is only used on Linux (Glibc) and Solaris/IllumOS.
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
 *   The main intention is to check if a certain symbol is present in a
 *   library so that you can conditionally enable or disable features.
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
 * Returns a pointer to the last saved error string.
 * This function doesn't return a null pointer or empty string,
 * the message will indicate if no error had occured.
 * Do not free the returned pointer!
 */
GDO_DECL const gdo_char_t *gdo_last_error(void)
    GDO_GCC_ATTRIBUTE (returns_nonnull);


/**
 * Return the library path. On error or if no library was loaded NULL is returned.
 * Do not free the returned pointer!
 *
 * On some systems and configurations the path is taken from the loaded symbols
 * in which case at least one symbol must have been successfully loaded before
 * using this function.
 */
GDO_DECL const gdo_char_t *gdo_lib_origin(void);


/**
 * Prefixed aliases, useful if GDO_DISABLE_ALIASING was defined.
 */
#define GDO_RAWPTR_%%func_symbol_pad%% gdo_hndl.GDO_PTR_%%func_symbol%%
#define GDO_RAWPTR_%%obj_symbol_pad%% gdo_hndl.GDO_PTR_%%obj_symbol%%
%PARAM_SKIP_REMOVE_BEGIN%


/*****************************************************************************/
/*                                wrap code                                  */
/*****************************************************************************/
#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

/* we need an actual function for the VA_ARGS macro */
inline void _gdo_noop(void) {}


/* by default #define hooks that do nothing */
#ifndef GDO_HOOK_%%func_symbol%%@
#define GDO_HOOK_%%func_symbol%%(...)  _gdo_noop()@
#endif


/* right now only GCC supports __builtin_va_arg_pack() */
#if defined(__GNUC__) && \
    defined(__has_builtin)
# if __has_builtin(__builtin_va_arg_pack)
#  define GDO_BUILTIN_VA_ARG_PACK
# endif
#endif


/* diagnostic warnings on variable arguments functions */
#if !defined(GDO_DISABLE_WARNINGS)

/* %%func_symbol%%() */@
#ifdef GDO_HAS_VA_ARGS_%%func_symbol%%@
# ifdef GDO_WRAP_VISIBILITY@
GDO_WARNING("GDO_WRAP_VISIBILITY defined but wrapper function %%func_symbol%%() can only be used inlined; define GDO_DISABLE_WARNINGS to silence this message")@
# endif@
#endif@

#endif //!GDO_DISABLE_WARNINGS


GDO_DECL void _gdo_wrap_check_loaded(void *symptr, int load, const gdo_char_t *sym);

#define _GDO_WRAP_CHECK_LOADED(SYMBOL) \
    _gdo_wrap_check_loaded( (void *)GDO_RAWPTR_##SYMBOL, GDO_LOAD_##SYMBOL, _T( #SYMBOL ) )


/**
 * GNU inline wrapper function for use with variable arguments
 * https://gcc.gnu.org/onlinedocs/gcc/Constructing-Calls.html
 * https://gcc.gnu.org/onlinedocs/gcc/Inline.html
 *
 * __builtin_va_arg_pack() is a GNU extension and will be resolved to the
 * additional parameters provided by the `...' argument.
 * This requires function inlining optimizations to be enabled.
 */
@
/* %%func_symbol%%() */@
#ifdef GDO_HAS_VA_ARGS_%%func_symbol%%@
# ifdef GDO_BUILTIN_VA_ARG_PACK@
    /* inline function (always inlined) */@
    extern inline __attribute__ ((__gnu_inline__))@
    %%type%% GDO_WRAP(%%func_symbol%%) (%%args%%) {@
        _GDO_WRAP_CHECK_LOADED( %%func_symbol%% );@
        GDO_HOOK_%%func_symbol%%( %%param_names%%, __builtin_va_arg_pack() );@
        %%return%% GDO_RAWPTR_%%func_symbol%%( %%param_names%%, __builtin_va_arg_pack() );@
    }@
# else /* fall back to using a macro */@
#  define GDO_WRAP_%%func_symbol%%(...) \@
    (_GDO_WRAP_CHECK_LOADED( %%func_symbol%% ),\@
     (GDO_HOOK_%%func_symbol%%( __VA_ARGS__ )),\@
      GDO_RAWPTR_%%func_symbol%%( __VA_ARGS__ ))@
# endif@
#else //!GDO_HAS_VA_ARGS_%%func_symbol%%@
    GDO_WRAP_DECL /* wrapper function */@
    %%type%% GDO_WRAP(%%func_symbol%%) (%%args%%) {@
        _GDO_WRAP_CHECK_LOADED( %%func_symbol%% );@
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

/* function name aliases */
#if !defined(GDO_WRAP_VISIBILITY)
# define %%func_symbol_pad%% GDO_FUNC_ALIAS(%%func_symbol%%)
#endif

/* object name aliases */
#define %%obj_symbol_pad%% *GDO_RAWPTR_%%obj_symbol%%

#endif //GDO_SEPARATE ...

