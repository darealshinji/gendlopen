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

