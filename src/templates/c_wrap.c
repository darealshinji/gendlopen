


/***************************************************************************/
/* autoload functions */
/***************************************************************************/
#ifdef _$ENABLE_AUTOLOAD

#if !defined(_$DEFAULT_LIB)
#error "You need to define _$DEFAULT_LIB if you want to make use of _$ENABLE_AUTOLOAD"
#endif


#ifdef _WIN32 // not _$WINAPI !!

#ifdef _UNICODE
/* convert narrow to wide characters */
_$LINKAGE wchar_t *$convert_str_to_wcs(const char *str)
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

#ifdef _$USE_MESSAGE_BOX
/* Windows: show message in a MessageBox window */
_$LINKAGE void $win32_show_last_error_in_messagebox(const char *function, const $char_t *symbol)
{
    const $char_t *err = $last_error();
    const $char_t *pfunc;
    /* double newline at end */
    const $char_t *fmt = _T("error in wrapper function `%s' for symbol `%s':\n\n%s");

#ifdef _UNICODE
    /* convert function name to wide characters
     * (we cannot receive the function name in a wide character format) */
    pfunc = $convert_str_to_wcs(function);
#else
    pfunc = function;
#endif //_UNICODE

    /* allocate message buffer */
    const size_t len = _tcslen(fmt) + _tcslen(pfunc) + _tcslen(symbol) + _tcslen(err);
    $char_t *buf = malloc((len + 1) * sizeof($char_t));

    /* save message to buffer */
    _sntprintf_s(buf, len, _TRUNCATE, fmt, pfunc, symbol, err);

    /* show message */
    MessageBox(NULL, buf, _T("Error"), MB_OK | MB_ICONERROR);

    /* free buffers */
#ifdef _UNICODE
    free(($char_t *)pfunc);
#endif
    free(buf);
}
#endif //_$USE_MESSAGE_BOX

#endif //_WIN32


/* This function is used by the wrapper functions to perform the loading
 * and handle errors. */
_$LINKAGE void $quick_load(const char *function, const $char_t *symbol)
{
    /* load library+symbols and return if successful */
    if ($load_lib_and_symbols()) {
        return;
    }

    /* an error has occured: display an error message */

#if defined(_WIN32) && defined(_$USE_MESSAGE_BOX)
    $win32_show_last_error_in_messagebox(function, symbol);
#elif defined(_WIN32) && defined(_UNICODE)
    /* Windows: output to console (wide characters) */
    fwprintf(stderr, L"error in wrapper function `%hs' for symbol `%ls':\n%ls\n",
        function, symbol, $last_error());
#else
    /* default: UTF-8 output to console (any operating system) */
    fprintf(stderr, "error in wrapper function `%s' for symbol `%s':\n%s\n",
        function, symbol, $last_error());
#endif //_WIN32 && _$USE_MESSAGE_BOX

    /* free library handle and exit */
    $free_lib();
    exit(1);
}

#else

#define $quick_load(a,b)  /**/

#endif //_$ENABLE_AUTOLOAD
/***************************************************************************/


/* wrapped functions */

#ifdef _$WRAP_FUNCTIONS
@
_$VISIBILITY GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
    $quick_load(__FUNCTION__, _T("GDO_SYMBOL"));@
    GDO_RET $hndl.GDO_SYMBOL_ptr_(GDO_NOTYPE_ARGS);@
}

#endif //_$WRAP_FUNCTIONS
