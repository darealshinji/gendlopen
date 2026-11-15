/*****************************************************************************/
/*                                wrap code                                  */
/*****************************************************************************/
%PARAM_SKIP_REMOVE_BEGIN%
#ifdef GDO_WRAP_FUNCTIONS

/* we need an actual function for the VA_ARGS macro */
GDO_INLINE void _gdo_noop(void) {}


/* by default #define hooks that do nothing */
#ifndef GDO_HOOK_%%func_symbol%%@
#define GDO_HOOK_%%func_symbol%%(...)  _gdo_noop()@
#endif


/* right now only GCC supports __builtin_va_arg_pack() */
#if defined(__GNUC__) && \
    !defined(__NO_INLINE__) && \
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
 * The function HAS to be declared `extern inline __attribute__ ((__gnu_inline__))'.
 */
@
/* %%func_symbol%%() */@
#ifdef GDO_HAS_VA_ARGS_%%func_symbol%%@
# ifdef __cplusplus@
#  define GDO_XDECL   template<typename... Types>@
#  define GDO_XARGS   Types... args@
#  define GDO_XPARAM  args...@
# elif defined(GDO_BUILTIN_VA_ARG_PACK)@
#  define GDO_XDECL   extern inline __attribute__ ((__gnu_inline__))@
#  define GDO_XARGS   %%args%%@
#  define GDO_XPARAM  %%param_names%%, __builtin_va_arg_pack()@
# endif@
#else@
# define GDO_XDECL    GDO_WRAP_DECL@
# define GDO_XARGS    %%args%%@
# define GDO_XPARAM   %%param_names%%@
#endif //!GDO_HAS_VA_ARGS_%%func_symbol%%@
#ifdef GDO_XDECL@
    GDO_WRAP_DECL %%type%% GDO_WRAP(%%func_symbol%%) (GDO_XARGS) {@
        _GDO_WRAP_CHECK_LOADED( %%func_symbol%% );@
        GDO_HOOK_%%func_symbol%%( GDO_XPARAM );@
        %%return%% GDO_RAWPTR_%%func_symbol%%( GDO_XPARAM );@
    }@
#else@
# define GDO_WRAP_%%func_symbol%%(...) \@
    (_GDO_WRAP_CHECK_LOADED( %%func_symbol%% ),\@
     (GDO_HOOK_%%func_symbol%%( __VA_ARGS__ )),\@
      GDO_RAWPTR_%%func_symbol%%( __VA_ARGS__ ))@
#endif@
#undef GDO_XDECL@
#undef GDO_XARGS@
#undef GDO_XPARAM

#endif //GDO_WRAP_FUNCTIONS
%PARAM_SKIP_END%
/***************************** end of wrap code ******************************/




