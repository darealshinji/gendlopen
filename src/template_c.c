#if defined _MSC_VER && defined(GDO_USE_MESSAGE_BOX)
    #pragma comment(lib, "user32.lib")
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _T
    #define _T(x) x
#endif

#ifdef GDO_ATEXIT
GDO_LINKAGE void _gdo_call_free_lib();
#endif
GDO_LINKAGE bool _gdo_free_lib();
GDO_LINKAGE void _gdo_clear_errbuf();

/* _gdo_sym() */
#ifdef GDO_WINAPI
    #define _GDO_SYM(a, b)  _gdo_sym(a, _T(a), b)
    GDO_LINKAGE void *_gdo_sym(const char *symbol, const gdo_char_t *msg, bool *b);
#else
    GDO_LINKAGE void *_GDO_SYM(const char *symbol, bool *b);
#endif

#define _GDO_BUFLEN 4096



typedef struct {
#ifdef GDO_WINAPI
    HMODULE handle;
    DWORD last_errno;
    /* according to MSDN the maximum is either 64k or 128k */
    gdo_char_t buf_formatted[64*1024];
#else
    void *handle;
#endif
    bool call_free_lib_is_registered;
    bool all_symbols_loaded;
    gdo_char_t buf[_GDO_BUFLEN];

    /* function pointers */
    GDO_TYPE (*GDO_SYMBOL_ptr_)(GDO_ARGS);

    /* object pointers */
    GDO_OBJ_TYPE *GDO_OBJ_SYMBOL_ptr_;

} _gdo_handle_t;

GDO_LINKAGE _gdo_handle_t _gdo_hndl = {0};



/***************************************************************************/
/* save error */
/***************************************************************************/
#ifdef GDO_WINAPI
GDO_LINKAGE void _gdo_save_error(const gdo_char_t *msg)
{
    _gdo_clear_errbuf();
    _gdo_hndl.last_errno = GetLastError();

    if (msg) {
        _sntprintf(_gdo_hndl.buf, _GDO_BUFLEN-1, _T("%s"), msg);
    }
}
#else
GDO_LINKAGE void _gdo_save_dl_error()
{
    _gdo_clear_errbuf();
    const char *ptr = dlerror();
    if (ptr) snprintf(_gdo_hndl.buf, _GDO_BUFLEN-1, "%s", ptr);
}
#endif //!GDO_WINAPI

GDO_LINKAGE void _gdo_clear_errbuf()
{
    _gdo_hndl.buf[0] = 0;
#ifdef GDO_WINAPI
    _gdo_hndl.buf_formatted[0] = 0;
    _gdo_hndl.last_errno = 0;
#endif
}
/***************************************************************************/



/***************************************************************************/
/* load default library with default flags */
/***************************************************************************/
#ifdef GDO_DEFAULT_LIB
GDO_LINKAGE bool gdo_load_lib()
{
    return gdo_load_lib_args(GDO_DEFAULT_LIB, GDO_DEFAULT_FLAGS, false);
}
#endif
/***************************************************************************/



/***************************************************************************/
/* load default library with default flags and load the symbols */
/***************************************************************************/
#ifdef GDO_DEFAULT_LIB
GDO_LINKAGE bool gdo_load_lib_and_symbols()
{
    return (gdo_load_lib_args(GDO_DEFAULT_LIB, GDO_DEFAULT_FLAGS, false) &&
        gdo_load_symbols());
}
#endif
/***************************************************************************/



/***************************************************************************/
/* load library by filename with default flags */
/***************************************************************************/
GDO_LINKAGE bool gdo_load_lib_name(const gdo_char_t *filename)
{
    return gdo_load_lib_args(filename, GDO_DEFAULT_FLAGS, false);
}
/***************************************************************************/



/***************************************************************************/
/* load the library */
/***************************************************************************/
GDO_LINKAGE bool gdo_load_lib_args(const gdo_char_t *filename, int flags, bool new_namespace)
{
    _gdo_clear_errbuf();

    /* library already loaded */
    if (gdo_lib_is_loaded()) {
        return true;
    }

#ifdef GDO_WINAPI

    /* win32 */
    (void)new_namespace; /* unused */
    if ((_gdo_hndl.handle = LoadLibraryEx(filename, NULL, flags)) == NULL) {
        _gdo_save_error(filename);
        return false;
    }

#else //!GDO_WINAPI

#ifdef GDO_NO_DLMOPEN
    /* dlmopen() disabled */
    (void)new_namespace;
    _gdo_hndl.handle = dlopen(filename, flags);
#else
    /* dlmopen() for new namespace or dlopen() */
    if (new_namespace) {
        _gdo_hndl.handle = dlmopen(LM_ID_NEWLM, filename, flags);
    } else {
        _gdo_hndl.handle = dlopen(filename, flags);
    }
#endif //GDO_NO_DLMOPEN

    if (!gdo_lib_is_loaded()) {
        _gdo_save_dl_error();
        return false;
    }

#endif //!GDO_WINAPI

#ifdef GDO_ATEXIT
    if (_gdo_hndl.call_free_lib_is_registered) {
        atexit(_gdo_call_free_lib);
        _gdo_hndl.call_free_lib_is_registered = false;
    }
#endif //GDO_ATEXIT

    return true;
}

#ifdef GDO_ATEXIT
GDO_LINKAGE void _gdo_call_free_lib()
{
    _gdo_free_lib();
}
#endif //GDO_ATEXIT
/***************************************************************************/



/***************************************************************************/
/* whether the library is currently loaded */
/***************************************************************************/
GDO_LINKAGE bool gdo_lib_is_loaded()
{
    return (_gdo_hndl.handle != NULL);
}
/***************************************************************************/



/***************************************************************************
* Free the library
****************************************************************************/
GDO_LINKAGE bool gdo_free_lib()
{
    _gdo_clear_errbuf();

    if (!gdo_lib_is_loaded()) return true;

    if (!_gdo_free_lib()) {
#ifdef GDO_WINAPI
        _gdo_save_error(_T("FreeLibrary()"));
#else
        _gdo_save_dl_error();
#endif
        return false;
    }

    return true;
}

GDO_LINKAGE bool _gdo_free_lib()
{
#ifdef GDO_WINAPI
    if (FreeLibrary(_gdo_hndl.handle) == FALSE)
#else
    if (dlclose(_gdo_hndl.handle) != 0)
#endif
    {
        return false;
    }

    /* set pointers back to NULL */

    _gdo_hndl.handle = NULL;

    _gdo_hndl.GDO_SYMBOL_ptr_ = NULL;
    _gdo_hndl.GDO_OBJ_SYMBOL_ptr_ = NULL;

    return true;
}
/***************************************************************************/



/***************************************************************************/
/* load all symbols; can safely be called multiple times.
 */
/***************************************************************************/
GDO_LINKAGE bool gdo_load_symbols()
{
    bool b = true;

    _gdo_clear_errbuf();

    /* already loaded all symbols */
    if (_gdo_hndl.all_symbols_loaded) {
        return true;
    }

    /* no library was loaded */
    if (!gdo_lib_is_loaded()) {
#ifdef GDO_WINAPI
        _gdo_hndl.last_errno = ERROR_INVALID_HANDLE;
        _tcscpy(_gdo_hndl.buf, _T("no library was loaded"));
#else
        strcpy(_gdo_hndl.buf, "no library was loaded");
#endif
        return false;
    }

    /* load function pointer addresses */

    _gdo_hndl.GDO_SYMBOL_ptr_ = (GDO_TYPE (*)(GDO_ARGS))@
        _GDO_SYM("GDO_SYMBOL", &b);@
    if (!b) return false;@


    /* load object pointer addresses */

    _gdo_hndl.GDO_OBJ_SYMBOL_ptr_ = (GDO_OBJ_TYPE*)@
        _GDO_SYM("GDO_OBJ_SYMBOL", &b);@
    if (!b) return false;@


    _gdo_hndl.all_symbols_loaded = true;

    return true;
}

#ifdef GDO_WINAPI
GDO_LINKAGE void *_gdo_sym(const char *symbol, const gdo_char_t *msg, bool *ret)
{
    void *ptr = (void *)GetProcAddress(_gdo_hndl.handle, symbol);

    if (ptr) {
        *ret = true;
    } else {
        *ret = false;
        _gdo_save_error(msg);
        _gdo_free_lib();
    }

    return ptr;
}
#else /* !GDO_WINAPI */
GDO_LINKAGE void *_GDO_SYM(const char *symbol, bool *ret)
{
    /* clear buffer */
    (void)dlerror();

    void *ptr = dlsym(_gdo_hndl.handle, symbol);

    /* NULL can be a valid value (unusual but possible),
     * so call dlerror() to check for errors */
    const char *p = dlerror();

    if (p) {
        *ret = false;
        _gdo_clear_errbuf();
        snprintf(_gdo_hndl.buf, _GDO_BUFLEN-1, "%s", p);
        _gdo_free_lib();
    } else {
        *ret = true;
    }

    return ptr;
}
#endif /* !GDO_WINAPI */
/***************************************************************************/



/***************************************************************************/
/* retrieve the last saved error message */
/***************************************************************************/
GDO_LINKAGE const gdo_char_t *gdo_last_error()
{
#ifdef GDO_WINAPI
    /* message was already saved */
    if (_gdo_hndl.buf_formatted[0] != 0) {
        return _gdo_hndl.buf_formatted;
    }

    _gdo_hndl.buf_formatted[0] = 0;

    const size_t bufmax = (sizeof(_gdo_hndl.buf_formatted) / sizeof(gdo_char_t)) - 1;
    gdo_char_t *buf = NULL;
    gdo_char_t *msg = _gdo_hndl.buf;
    gdo_char_t *bufFmt = _gdo_hndl.buf_formatted;

    /* format the message */
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS |
                    FORMAT_MESSAGE_MAX_WIDTH_MASK,
                NULL,
                _gdo_hndl.last_errno,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&buf,
                0,
                NULL);

    if (buf) {
        /* put custom message in front of system error message */
        if (msg[0] != 0 && (_tcslen(buf) + _tcslen(msg) + 3) < bufmax) {
            _sntprintf(bufFmt, bufmax, _T("%s: %s"), msg, buf);
        } else {
            _sntprintf(bufFmt, bufmax, _T("%s"), buf);
        }
        LocalFree(buf);
    } else {
        /* FormatMessage() failed, just print the error code */
        _sntprintf(bufFmt, bufmax, _T("Last saved error code: %lu"),
            _gdo_hndl.last_errno);
    }

    return bufFmt;
#else
    /* simply return the buffer */
    return _gdo_hndl.buf;
#endif //GDO_WINAPI
}
/***************************************************************************/



/***************************************************************************/
/* get the full library path - returns NULL on error */
/***************************************************************************/
GDO_LINKAGE gdo_char_t *gdo_lib_origin()
{
    _gdo_clear_errbuf();

#ifdef GDO_WINAPI
    gdo_char_t *origin;
    size_t len = MAX_PATH;

    if (!gdo_lib_is_loaded()) {
        _gdo_hndl.last_errno = ERROR_INVALID_HANDLE;
        _tcscpy(_gdo_hndl.buf, _T("no library was loaded"));
        return NULL;
    }

    origin = (gdo_char_t *)malloc(len * sizeof(gdo_char_t));

    if (!origin) {
        _gdo_save_error(_T("malloc"));
        return NULL;
    }

    if (GetModuleFileName(_gdo_hndl.handle, origin, len-1) == 0) {
        _gdo_save_error(_T("GetModuleFileName"));
        free(origin);
        return NULL;
    }

    /* technically the path could exceed 260 characters, but in reality
     * it's practically still stuck at the old MAX_PATH value */
    while (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        len += 1024;
        origin = (gdo_char_t *)realloc(origin, len * sizeof(gdo_char_t));

        if (GetModuleFileName(_gdo_hndl.handle, origin, len-1) == 0) {
            _gdo_save_error(_T("GetModuleFileName"));
            free(origin);
            return NULL;
        }
    }

    return origin;
#else
    struct link_map *lm = NULL;

    if (!gdo_lib_is_loaded()) {
        strcpy(_gdo_hndl.buf, "no library was loaded");
        return NULL;
    } else if (dlinfo(_gdo_hndl.handle, RTLD_DI_LINKMAP, &lm) == -1) {
        _gdo_save_dl_error();
        return NULL;
    }

    if (lm->l_name) {
        return strdup(lm->l_name);
    }
#endif //GDO_WINAPI

    return NULL;
}
/***************************************************************************/



/***************************************************************************/
/* autoload wrapper functions */
/***************************************************************************/
#ifdef GDO_USE_WRAPPER

#if !defined(GDO_DEFAULT_LIB)
#error "You need to define GDO_DEFAULT_LIB if you want to make use of GDO_USE_WRAPPER"
#endif


#if defined(_WIN32) && defined(_UNICODE)
GDO_LINKAGE wchar_t *_gdo_convert_utf8_to_wcs(const char *lpStr)
{
    int mbslen = (int)strlen(lpStr);
    int wlen = MultiByteToWideChar(CP_UTF8, 0, lpStr, mbslen, NULL, 0);
    if (wlen < 1) return NULL;

    wchar_t *pwBuf = malloc((wlen + 1) * sizeof(wchar_t));
    if (!pwBuf) return NULL;

    if (MultiByteToWideChar(CP_UTF8, 0, lpStr, mbslen, pwBuf, wlen) < 1) {
        free(pwBuf);
        return NULL;
    }

    pwBuf[wlen] = L'\0';
    return pwBuf;
}
#endif //_WIN32 && _UNICODE


GDO_LINKAGE void _gdo_quick_load(const char *function, const gdo_char_t *symbol)
{
    /* load and return if successful */
    if (gdo_load_lib_and_symbols()) {
        return;
    }

    /* print error message */

#if defined(_WIN32) && defined(GDO_USE_MESSAGE_BOX)
    /* Windows with MessageBox */
    const gdo_char_t *err = gdo_last_error();
    const gdo_char_t *pfunc;
    /* double newline at end */
    const gdo_char_t *fmt = _T("error in wrapper function `%s' for symbol `%s':\n\n%s");

#ifdef _UNICODE
    /* convert function name to wide characters */
    wchar_t *wfunc = _gdo_convert_utf8_to_wcs(function);
    pfunc = wfunc;
#else
    pfunc = function;
#endif //_UNICODE

    const size_t len = _tcslen(fmt) +  _tcslen(pfunc) + _tcslen(symbol) + _tcslen(err);
    gdo_char_t *buf = malloc((len + 1) * sizeof(gdo_char_t));
    _sntprintf(buf, len, fmt, pfunc, symbol, err);
    MessageBox(NULL, buf, _T("Error"), MB_OK | MB_ICONERROR);
#ifdef _UNICODE
    free(wfunc);
#endif
    free(buf);

#elif defined(_WIN32) && defined(_UNICODE)

    /* Windows with support for wide character output to console */
    fwprintf(stderr, L"error in wrapper function `%hs' for symbol `%ls':\n%ls\n",
        function, symbol, gdo_last_error());

#else

    /* default: UTF-8 output to console for Windows and other systems */
    fprintf(stderr, "error in wrapper function `%s' for symbol `%s':\n%s\n",
        function, symbol, gdo_last_error());

#endif //_WIN32 && GDO_USE_MESSAGE_BOX

    /* free library handle and exit */

    if (gdo_lib_is_loaded()) {
        _gdo_free_lib();
    }

    exit(1);
}


/* wrapper functions */

GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
    _gdo_quick_load(__FUNCTION__, _T("GDO_SYMBOL"));@
    GDO_RET _gdo_hndl.GDO_SYMBOL_ptr_(GDO_NOTYPE_ARGS);@
}@

#undef _GDO_WRAP

#endif //GDO_USE_WRAPPER
/***************************************************************************/


#ifdef __cplusplus
} /* extern "C" */
#endif
