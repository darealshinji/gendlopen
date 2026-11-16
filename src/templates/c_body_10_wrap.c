/*****************************************************************************/
/*                     helper used by wrapper functions                      */
/*****************************************************************************/
#ifdef GDO_WRAP_FUNCTIONS

void _gdo_noop(void)
{}


/* strstr() / wcsstr() */
GDO_INLINE const gdo_char_t *_gdo_tcsstr(const gdo_char_t *haystack, const gdo_char_t *needle)
{
#ifdef _WIN32
    return _tcsstr(haystack, needle);
#else
    return strstr(haystack, needle);
#endif
}


GDO_INLINE void _gdo_print_error(const gdo_char_t *fmt, const gdo_char_t *sym, const gdo_char_t *msg)
{
#ifdef _WIN32

# ifdef GDO_USE_MESSAGE_BOX
    /* we can safely use gdo_hndl.errbuf, the last error message
     * was saved in gdo_hndl.formatted */
    _sntprintf_s(gdo_hndl.errbuf, GDO_BUFLEN, _TRUNCATE, fmt, sym, msg);
    MessageBox(NULL, gdo_hndl.errbuf, _T("Error"), MB_OK | MB_ICONERROR);
# else
    _ftprintf_s(stderr, fmt, sym, msg);
    _fputtc(_T('\n'), stderr);
# endif //GDO_USE_MESSAGE_BOX

#else //!_WIN32

    fprintf(stderr, fmt, sym, msg);
    fputc('\n', stderr);

#endif //!_WIN32
}


#ifdef GDO_ENABLE_AUTOLOAD
GDO_LINKAGE void _gdo_wrap_autoload(int load, const gdo_char_t *sym)
{
    const gdo_char_t *msg;

    /* set auto-release, ignore errors */
    gdo_enable_autorelease();

    /* load library */
    if (!gdo_lib_is_loaded()) {
        gdo_load_lib();
    }

# ifdef GDO_ENABLE_AUTOLOAD_LAZY
    /* load a specific symbol */
    if (gdo_load_symbol(load)) {
        return;
    }
# else
    GDO_UNUSED(load);

    /* load all symbols */
    if (gdo_load_all_symbols()) {
        return;
    }
# endif //!GDO_ENABLE_AUTOLOAD_LAZY

    msg = gdo_last_error();

    if (_gdo_tcsstr(msg, GDO_DEFAULT_LIB)) {
        /* library name is already part of error message */
        _gdo_print_error(_T("error: %s: %s"), sym, msg);
    } else {
        _gdo_print_error(_T("error: ") GDO_DEFAULT_LIB _T(": %s: %s"), sym, msg);
    }

    gdo_force_free_lib();

    exit(1);
}
#endif //GDO_ENABLE_AUTOLOAD


/* used by wrapper functions */
GDO_LINKAGE void _gdo_wrap_check_loaded(void *symptr, int load, const gdo_char_t *sym)
{
    const gdo_char_t *msg;

    /* symbol was already loaded */
    if (symptr != NULL) {
        return;
    }

#ifdef GDO_ENABLE_AUTOLOAD

    GDO_UNUSED(msg);
    _gdo_wrap_autoload(load, sym);

#else

    /* don't load anything, print an error message and abort */

    if (!gdo_lib_is_loaded()) {
        msg = _T("library not loaded");
    } else {
        msg = _T("symbol not loaded");
    }

    GDO_UNUSED(load);
    _gdo_print_error(_T("fatal error: %s: %s"), sym, msg);

    abort();

#endif //!GDO_ENABLE_AUTOLOAD
}

#endif // GDO_WRAP_FUNCTIONS
/*****************************************************************************/




