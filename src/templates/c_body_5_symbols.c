/*****************************************************************************/
/*                    check if ALL symbols were loaded                       */
/*****************************************************************************/
GDO_LINKAGE bool gdo_all_symbols_loaded(void)
{
    if (true
        && GDO_RAWPTR_%%symbol%% != NULL
    ) {
        return true;
    }

    return false;
}
/*****************************************************************************/


/*****************************************************************************/
/*                    check if NO symbols were loaded                        */
/*****************************************************************************/
GDO_LINKAGE bool gdo_no_symbols_loaded(void)
{
    if (true
        && GDO_RAWPTR_%%symbol%% == NULL
    ) {
        return true;
    }

    return false;
}
/*****************************************************************************/


/*****************************************************************************/
/*                     check if ANY symbol was loaded                        */
/*****************************************************************************/
GDO_LINKAGE bool gdo_any_symbol_loaded(void)
{
    if (false
        || GDO_RAWPTR_%%symbol%% != NULL
    ) {
        return true;
    }

    return false;
}
/*****************************************************************************/


/*****************************************************************************/
/*                             load all symbols                              */
/*****************************************************************************/
GDO_INLINE void *_gdo_sym(const char *symbol, const gdo_char_t *msg);

GDO_LINKAGE bool gdo_load_all_symbols(void)
{
    _gdo_clear_error();

    if (gdo_all_symbols_loaded()) {
        return true;
    }

    if (!gdo_lib_is_loaded()) {
        _gdo_set_error_no_library_loaded();
        return false;
    }

    /* get symbol addresses */

    /* %%symbol%% */@
    if ((GDO_RAWPTR_%%symbol%% =@
            (%%sym_type%%)@
                _gdo_sym("%%symbol%%", _T("%%symbol%%"))) == NULL) {@
        return false;@
    }@

    return true;
}

GDO_INLINE void *_gdo_sym(const char *symbol, const gdo_char_t *msg)
{
#ifdef GDO_WINAPI
    void *ptr = (void *)GetProcAddress(gdo_hndl.handle, symbol);
#else
    void *ptr = dlsym(gdo_hndl.handle, symbol);
#endif

    if (!ptr) {
        _gdo_save_error(msg);
    }

    return ptr;
}
/*****************************************************************************/


/*****************************************************************************/
/*                        load a specific symbol                             */
/*****************************************************************************/
GDO_LINKAGE bool gdo_load_symbol(int symbol_num)
{
    _gdo_clear_error();

    if (!gdo_lib_is_loaded()) {
        _gdo_set_error_no_library_loaded();
        return false;
    }

    switch (symbol_num)
    {
    /* %%symbol%% */@
    case GDO_LOAD_%%symbol%%:@
        if (!GDO_RAWPTR_%%symbol%%) {@
            GDO_RAWPTR_%%symbol%% =@
                (%%sym_type%%)@
                    _gdo_sym("%%symbol%%", _T("%%symbol%%"));@
        }@
        return (GDO_RAWPTR_%%symbol%% != NULL);@

    default:
        break;
    }

    GDO_SET_LAST_ERRNO(ERROR_NOT_FOUND);
    GDO_SNPRINTF(gdo_hndl.errbuf, _T("unknown symbol number: %d"), symbol_num);

    return false;
}
/*****************************************************************************/


/*****************************************************************************/
/*                    load a specific symbol by name                         */
/*****************************************************************************/
GDO_LINKAGE bool gdo_load_symbol_name(const char *symbol)
{
    _gdo_clear_error();

    if (!gdo_lib_is_loaded()) {
        _gdo_set_error_no_library_loaded();
        return false;
    }

    if (!symbol || *symbol == 0) {
        GDO_SET_LAST_ERRNO(ERROR_INVALID_PARAMETER);
        _gdo_save_to_errbuf(_T("empty symbol name"));
        return false;
    }

    /* check symbol prefix */
    const char pfx[] = GDO_COMMON_PREFIX;
    const size_t pfxlen = sizeof(pfx) - 1;

    if (pfxlen == 0 || strncmp(symbol, pfx, pfxlen) == 0) {
        /* check symbol names (skips common prefix if present) */
@
        if (strcmp(symbol + pfxlen, "%%symbol_nopfx%%") == 0) {@
            return gdo_load_symbol(GDO_LOAD_%%symbol%%);@
        }
    }

    GDO_SET_LAST_ERRNO(ERROR_NOT_FOUND);
    GDO_SNPRINTF(gdo_hndl.errbuf, _T("unknown symbol: ") GDO_XHS, symbol);

    return false;
}
/*****************************************************************************/




