#if !defined(GDO_SEPARATE) && \
    !defined(GDO_DISABLE_ALIASING)

/* aliases to raw function pointers */
#if !defined(GDO_WRAP_IS_VISIBLE)
# define %%func_symbol_pad%% GDO_FUNC_ALIAS(%%func_symbol%%)
#endif

/* aliases to raw object pointers */
#define %%obj_symbol_pad%% *GDO_RAWPTR_%%obj_symbol%%

#endif //!GDO_SEPARATE


#undef GDO_SET_LAST_ERRNO
#undef GDO_SNPRINTF

