


/***************************************************************************/
/* autoload functions */
/***************************************************************************/
#ifdef GDO_ENABLE_AUTOLOAD

#if !defined(GDO_DEFAULT_LIB)
#error "You need to define GDO_DEFAULT_LIB if you want to make use of GDO_ENABLE_AUTOLOAD"
#endif


#ifdef _WIN32 // not GDO_WINAPI !!

#ifdef _UNICODE
/* convert narrow to wide characters */
GDO_LINKAGE wchar_t *gdo_convert_str_to_wcs(const char *str)
{
    size_t len, n;
    wchar_t *buf;

    if (!str) return NULL;

    if (mbstowcs_s(&len, NULL, 0, str, 0) != 0 || len == 0) {
        return NULL;
    }

    buf = malloc((len + 1) * sizeof(wchar_t));
    if (!buf) return NULL;

    if (mbstowcs_s(&n, buf, len+1, str, len) != 0 || n == 0) {
        free(buf);
        return NULL;
    }

    buf[len] = L'\0';

    return buf;
}
#endif //_UNICODE

#ifdef GDO_USE_MESSAGE_BOX
/* Windows: show message in a MessageBox window */
GDO_LINKAGE void gdo_win32_show_last_error_in_messagebox(const char *function, const gdo_char_t *symbol)
{
    const gdo_char_t *err = gdo_last_error();
    const gdo_char_t *pfunc;
    /* double newline at end */
    const gdo_char_t *fmt = _T("error in wrapper function `%s' for symbol `%s':\n\n%s");

#ifdef _UNICODE
    /* convert function name to wide characters
     * (we cannot receive the function name in a wide character format) */
    pfunc = gdo_convert_str_to_wcs(function);
#else
    pfunc = function;
#endif //_UNICODE

    /* allocate message buffer */
    const size_t len = _tcslen(fmt) + _tcslen(pfunc) + _tcslen(symbol) + _tcslen(err);
    gdo_char_t *buf = malloc((len + 1) * sizeof(gdo_char_t));

    /* save message to buffer */
    _sntprintf_s(buf, len, _TRUNCATE, fmt, pfunc, symbol, err);

    /* show message */
    MessageBox(NULL, buf, _T("Error"), MB_OK | MB_ICONERROR);

    /* free buffers */
#ifdef _UNICODE
    free((gdo_char_t *)pfunc);
#endif
    free(buf);
}
#endif //GDO_USE_MESSAGE_BOX

#endif //_WIN32


/* This function is used by the wrapper functions to perform the loading
 * and handle errors. */
GDO_LINKAGE void gdo_quick_load(const char *function, const gdo_char_t *symbol)
{
    /* load library+symbols and return if successful */
    if (gdo_load_lib_and_symbols()) {
        return;
    }

    /* an error has occured: display an error message */

#if defined(_WIN32) && defined(GDO_USE_MESSAGE_BOX)
    gdo_win32_show_last_error_in_messagebox(function, symbol);
#elif defined(_WIN32) && defined(_UNICODE)
    /* Windows: output to console (wide characters) */
    fwprintf(stderr, L"error in wrapper function `%hs' for symbol `%ls':\n%ls\n",
        function, symbol, gdo_last_error());
#else
    /* default: UTF-8 output to console (any operating system) */
    fprintf(stderr, "error in wrapper function `%s' for symbol `%s':\n%s\n",
        function, symbol, gdo_last_error());
#endif //_WIN32 && GDO_USE_MESSAGE_BOX

    /* free library handle and exit */
    gdo_free_lib();
    exit(1);
}

#else

#define gdo_quick_load(a,b)  /**/

#endif //GDO_ENABLE_AUTOLOAD
/***************************************************************************/


/* wrapped functions */

#ifdef GDO_WRAP_FUNCTIONS
@
GDO_VISIBILITY GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
    gdo_quick_load(__FUNCTION__, _T("GDO_SYMBOL"));@
    GDO_RET gdo_hndl.GDO_SYMBOL_ptr_(GDO_NOTYPE_ARGS);@
}

#endif //GDO_WRAP_FUNCTIONS
