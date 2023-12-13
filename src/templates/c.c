#if defined _MSC_VER && defined(_$USE_MESSAGE_BOX)
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

#ifdef _$ATEXIT
_$LINKAGE void $call_free_lib();
#endif
_$LINKAGE void $clear_errbuf();

/* $sym() */
#ifdef _$WINAPI
    #define _$SYM(a, b)  $sym(a, _T(a), b)
    _$LINKAGE void *$sym(const char *symbol, const $char_t *msg, bool *b);
#else
    _$LINKAGE void *_$SYM(const char *symbol, bool *b);
#endif

#define _$BUFLEN 4096



typedef struct {
#ifdef _$WINAPI
    HMODULE handle;
    DWORD last_errno;
    /* according to MSDN the maximum is either 64k or 128k */
    $char_t buf_formatted[64*1024];
#else
    void *handle;
#endif
    bool call_free_lib_is_registered;
    bool all_symbols_loaded;
    $char_t buf[_$BUFLEN];

    /* function pointers */
    GDO_TYPE (*GDO_SYMBOL_ptr_)(GDO_ARGS);

    /* object pointers */
    GDO_OBJ_TYPE *GDO_OBJ_SYMBOL_ptr_;

} $handle_t;

_$LINKAGE $handle_t $hndl = {0};



/***************************************************************************/
/* save error */
/***************************************************************************/
#ifdef _$WINAPI
_$LINKAGE void $save_error(const $char_t *msg)
{
    $clear_errbuf();
    $hndl.last_errno = GetLastError();

    if (msg) {
        _sntprintf($hndl.buf, _$BUFLEN-1, _T("%s"), msg);
    }
}
#else
_$LINKAGE void $save_dl_error()
{
    $clear_errbuf();
    const char *ptr = dlerror();
    if (ptr) snprintf($hndl.buf, _$BUFLEN-1, "%s", ptr);
}
#endif //!_$WINAPI

_$LINKAGE void $clear_errbuf()
{
    $hndl.buf[0] = 0;
#ifdef _$WINAPI
    $hndl.buf_formatted[0] = 0;
    $hndl.last_errno = 0;
#endif
}
/***************************************************************************/



/***************************************************************************/
/* load default library with default flags */
/***************************************************************************/
#ifdef _$DEFAULT_LIB
_$LINKAGE bool $load_lib()
{
    return $load_lib_args(_$DEFAULT_LIB, _$DEFAULT_FLAGS, false);
}
#endif
/***************************************************************************/



/***************************************************************************/
/* load default library with default flags and load the symbols */
/***************************************************************************/
#ifdef _$DEFAULT_LIB
_$LINKAGE bool $load_lib_and_symbols()
{
    return ($load_lib_args(_$DEFAULT_LIB, _$DEFAULT_FLAGS, false) &&
        $load_symbols());
}
#endif
/***************************************************************************/



/***************************************************************************/
/* load library by filename with default flags */
/***************************************************************************/
_$LINKAGE bool $load_lib_name(const $char_t *filename)
{
    return $load_lib_args(filename, _$DEFAULT_FLAGS, false);
}
/***************************************************************************/



/***************************************************************************/
/* load the library */
/***************************************************************************/
_$LINKAGE bool $load_lib_args(const $char_t *filename, int flags, bool new_namespace)
{
    $clear_errbuf();

    /* library already loaded */
    if ($lib_is_loaded()) {
        return true;
    }

#ifdef _$WINAPI

    /* win32 */
    (void)new_namespace; /* unused */
    if (($hndl.handle = LoadLibraryEx(filename, NULL, flags)) == NULL) {
        $save_error(filename);
        return false;
    }

#else //!_$WINAPI

#ifdef _$NO_DLMOPEN
    /* dlmopen() disabled */
    (void)new_namespace;
    $hndl.handle = dlopen(filename, flags);
#else
    /* dlmopen() for new namespace or dlopen() */
    if (new_namespace) {
        $hndl.handle = dlmopen(LM_ID_NEWLM, filename, flags);
    } else {
        $hndl.handle = dlopen(filename, flags);
    }
#endif //_$NO_DLMOPEN

    if (!$lib_is_loaded()) {
        $save_dl_error();
        return false;
    }

#endif //!_$WINAPI

#ifdef _$ATEXIT
    if ($hndl.call_free_lib_is_registered) {
        atexit($call_free_lib);
        $hndl.call_free_lib_is_registered = false;
    }
#endif //_$ATEXIT

    return true;
}

#ifdef _$ATEXIT
_$LINKAGE void $call_free_lib()
{
#ifdef _$WINAPI
    FreeLibrary($hndl.handle);
#else
    dlclose($hndl.handle);
#endif //!_$WINAPI
}
#endif //_$ATEXIT
/***************************************************************************/



/***************************************************************************/
/* whether the library is currently loaded */
/***************************************************************************/
_$LINKAGE bool $lib_is_loaded()
{
    return ($hndl.handle != NULL);
}
/***************************************************************************/



/***************************************************************************
* Free the library
****************************************************************************/
_$LINKAGE bool $free_lib()
{
    $clear_errbuf();

    if (!$lib_is_loaded()) return true;

#ifdef _$WINAPI
    if (FreeLibrary($hndl.handle) == FALSE) {
        $save_error(_T("FreeLibrary()"));
        return false;
    }
#else
    if (dlclose($hndl.handle) != 0) {
        $save_dl_error();
        return false;
    }
#endif //!_$WINAPI

    /* set pointers back to NULL */

    $hndl.handle = NULL;

    $hndl.GDO_SYMBOL_ptr_ = NULL;
    $hndl.GDO_OBJ_SYMBOL_ptr_ = NULL;

    return true;
}
/***************************************************************************/



/***************************************************************************/
/* load all symbols; can safely be called multiple times.
 */
/***************************************************************************/
_$LINKAGE bool $load_symbols()
{
    bool b = true;

    $clear_errbuf();

    /* already loaded all symbols */
    if ($hndl.all_symbols_loaded) {
        return true;
    }

    /* no library was loaded */
    if (!$lib_is_loaded()) {
#ifdef _$WINAPI
        $hndl.last_errno = ERROR_INVALID_HANDLE;
        _tcscpy($hndl.buf, _T("no library was loaded"));
#else
        strcpy($hndl.buf, "no library was loaded");
#endif
        return false;
    }

    /* load function pointer addresses */

    $hndl.GDO_SYMBOL_ptr_ = (GDO_TYPE (*)(GDO_ARGS))@
        _$SYM("GDO_SYMBOL", &b);@
    if (!b) return false;@


    /* load object pointer addresses */

    $hndl.GDO_OBJ_SYMBOL_ptr_ = (GDO_OBJ_TYPE*)@
        _$SYM("GDO_OBJ_SYMBOL", &b);@
    if (!b) return false;@


    $hndl.all_symbols_loaded = true;

    return true;
}

#ifdef _$WINAPI
_$LINKAGE void *$sym(const char *symbol, const $char_t *msg, bool *ret)
{
    void *ptr = (void *)GetProcAddress($hndl.handle, symbol);

    if (ptr) {
        *ret = true;
    } else {
        *ret = false;
        $save_error(msg);
        $free_lib();
    }

    return ptr;
}
#else //!_$WINAPI
_$LINKAGE void *_$SYM(const char *symbol, bool *ret)
{
    /* clear buffer */
    (void)dlerror();

    void *ptr = dlsym($hndl.handle, symbol);

    /* NULL can be a valid value (unusual but possible),
     * so call dlerror() to check for errors */
    const char *p = dlerror();

    if (p) {
        *ret = false;
        $clear_errbuf();
        snprintf($hndl.buf, _$BUFLEN-1, "%s", p);
        $free_lib();
    } else {
        *ret = true;
    }

    return ptr;
}
#endif //!_$WINAPI
/***************************************************************************/



/***************************************************************************/
/* retrieve the last saved error message */
/***************************************************************************/
_$LINKAGE const $char_t *$last_error()
{
#ifdef _$WINAPI
    /* message was already saved */
    if ($hndl.buf_formatted[0] != 0) {
        return $hndl.buf_formatted;
    }

    $hndl.buf_formatted[0] = 0;

    const size_t bufmax = (sizeof($hndl.buf_formatted) / sizeof($char_t)) - 1;
    $char_t *buf = NULL;
    $char_t *msg = $hndl.buf;
    $char_t *bufFmt = $hndl.buf_formatted;

    /* format the message */
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_IGNORE_INSERTS |
                    FORMAT_MESSAGE_MAX_WIDTH_MASK,
                NULL,
                $hndl.last_errno,
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
            $hndl.last_errno);
    }

    return bufFmt;
#else
    /* simply return the buffer */
    return $hndl.buf;
#endif //_$WINAPI
}
/***************************************************************************/



/***************************************************************************/
/* get the full library path - returns NULL on error */
/***************************************************************************/
_$LINKAGE $char_t *$lib_origin()
{
    $clear_errbuf();

#ifdef _$WINAPI
    $char_t *origin;
    size_t len = 260;

    if (!$lib_is_loaded()) {
        $hndl.last_errno = ERROR_INVALID_HANDLE;
        _tcscpy($hndl.buf, _T("no library was loaded"));
        return NULL;
    }

    origin = ($char_t *)malloc(len * sizeof($char_t));

    if (!origin) {
        $save_error(_T("malloc"));
        return NULL;
    }

    if (GetModuleFileName($hndl.handle, origin, len-1) == 0) {
        $save_error(_T("GetModuleFileName"));
        free(origin);
        return NULL;
    }

    /* technically the path could exceed 260 characters, but in reality
     * it's practically still stuck at the old MAX_PATH value */
    while (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        len += 1024;
        origin = ($char_t *)realloc(origin, len * sizeof($char_t));

        if (GetModuleFileName($hndl.handle, origin, len-1) == 0) {
            $save_error(_T("GetModuleFileName"));
            free(origin);
            return NULL;
        }
    }

    return origin;
#else
    struct link_map *lm = NULL;

    if (!$lib_is_loaded()) {
        strcpy($hndl.buf, "no library was loaded");
        return NULL;
    } else if (dlinfo($hndl.handle, RTLD_DI_LINKMAP, &lm) == -1) {
        $save_dl_error();
        return NULL;
    }

    if (lm->l_name) {
        return strdup(lm->l_name);
    }
#endif //_$WINAPI

    return NULL;
}
/***************************************************************************/



/***************************************************************************/
/* autoload wrapper functions */
/***************************************************************************/
#ifdef _$USE_WRAPPER

#if !defined(_$DEFAULT_LIB)
#error "You need to define _$DEFAULT_LIB if you want to make use of _$USE_WRAPPER"
#endif


#if defined(_WIN32) && defined(_UNICODE)
_$LINKAGE wchar_t *$convert_utf8_to_wcs(const char *lpStr)
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


_$LINKAGE void $quick_load(const char *function, const $char_t *symbol)
{
    /* load and return if successful */
    if ($load_lib_and_symbols()) {
        return;
    }

    /* print error message */

#if defined(_WIN32) && defined(_$USE_MESSAGE_BOX)
    /* Windows with MessageBox */
    const $char_t *err = $last_error();
    const $char_t *pfunc;
    /* double newline at end */
    const $char_t *fmt = _T("error in wrapper function `%s' for symbol `%s':\n\n%s");

#ifdef _UNICODE
    /* convert function name to wide characters */
    wchar_t *wfunc = $convert_utf8_to_wcs(function);
    pfunc = wfunc;
#else
    pfunc = function;
#endif //_UNICODE

    const size_t len = _tcslen(fmt) +  _tcslen(pfunc) + _tcslen(symbol) + _tcslen(err);
    $char_t *buf = malloc((len + 1) * sizeof($char_t));
    _sntprintf(buf, len, fmt, pfunc, symbol, err);
    MessageBox(NULL, buf, _T("Error"), MB_OK | MB_ICONERROR);
#ifdef _UNICODE
    free(wfunc);
#endif
    free(buf);

#elif defined(_WIN32) && defined(_UNICODE)

    /* Windows with support for wide character output to console */
    fwprintf(stderr, L"error in wrapper function `%hs' for symbol `%ls':\n%ls\n",
        function, symbol, $last_error());

#else

    /* default: UTF-8 output to console for Windows and other systems */
    fprintf(stderr, "error in wrapper function `%s' for symbol `%s':\n%s\n",
        function, symbol, $last_error());

#endif //_WIN32 && _$USE_MESSAGE_BOX

    /* free library handle and exit */

    if ($lib_is_loaded()) {
        $free_lib();
    }

    exit(1);
}


/* wrapper functions */

GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
    $quick_load(__FUNCTION__, _T("GDO_SYMBOL"));@
    GDO_RET $hndl.GDO_SYMBOL_ptr_(GDO_NOTYPE_ARGS);@
}@

#undef $WRAP

#endif //_$USE_WRAPPER
/***************************************************************************/


#ifdef __cplusplus
} /* extern "C" */
#endif
