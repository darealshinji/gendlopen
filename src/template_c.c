#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef __cplusplus
extern "C" {
#endif

#ifndef _T
    #define _T(x) x
#endif

#ifndef _GDO_ARRSZ
    #define _GDO_ARRSZ(x) (sizeof(x) / sizeof(*(x)))
#endif

#ifdef GDO_ATEXIT
GDO_LINKAGE void _gdo_call_free_lib();
#endif
GDO_LINKAGE bool _gdo_free_lib();
GDO_LINKAGE void *_gdo_sym(const char *symbol, const gdo_char_t *msg, bool *b);



typedef struct {
#ifdef GDO_WINAPI
    HMODULE handle;
    DWORD last_errno;
    gdo_char_t buf_formatted[64*1024];
#else
    void *handle;
#endif
    bool call_free_lib_is_registered;
    bool all_symbols_loaded;
    gdo_char_t buf[4*1024];

    /* function pointers */
    GDO_TYPE (*GDO_SYMBOL_ptr_)(GDO_ARGS);

    /* object pointers */
    GDO_OBJ_TYPE *GDO_OBJ_SYMBOL_ptr_;

} _gdo_handle_t;

GDO_LINKAGE _gdo_handle_t _gdo_handle = {0};



/***************************************************************************/
/* save error */
/***************************************************************************/
GDO_LINKAGE void _gdo_save_error(const gdo_char_t *msg, const char *errptr)
{
    _gdo_handle.buf[0] = 0;
#ifdef GDO_WINAPI
    (void)errptr;
    _gdo_handle.buf_formatted[0] = 0;
    _gdo_handle.last_errno = GetLastError();
    if (msg) _tcsncpy(_gdo_handle.buf, msg, _GDO_ARRSZ(_gdo_handle.buf)-1);
#else
    (void)msg;

    if (errptr) {
        /* copy string first, then call dlerror() to clear buffer */
        strncpy(_gdo_handle.buf, errptr, sizeof(_gdo_handle.buf)-1);
        (void)dlerror();
    } else {
        const char *ptr = dlerror();
        if (ptr) strncpy(_gdo_handle.buf, ptr, sizeof(_gdo_handle.buf)-1);
    }
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
    _gdo_handle.buf[0] = 0;
#ifdef GDO_WINAPI
    _gdo_handle.buf_formatted[0] = 0;
    _gdo_handle.last_errno = 0;
#endif

    /* library already loaded */
    if (gdo_lib_is_loaded()) {
        return true;
    }

#ifdef GDO_WINAPI

    /* win32 */
    if ((_gdo_handle.handle = LoadLibraryEx(filename, NULL, flags)) == NULL) {
        _gdo_save_error(filename, NULL);
        return false;
    }

#else //!GDO_WINAPI

#ifdef GDO_NO_DLMOPEN
    /* dlmopen() disabled */
    (void)new_namespace;
    _gdo_handle.handle = dlopen(filename, flags);
#else
    /* dlmopen() for new namespace or dlopen() */
    if (new_namespace) {
        _gdo_handle.handle = dlmopen(LM_ID_NEWLM, filename, flags);
    } else {
        _gdo_handle.handle = dlopen(filename, flags);
    }
#endif //GDO_NO_DLMOPEN

    if (!gdo_lib_is_loaded()) {
        _gdo_save_error(NULL, NULL);
        return false;
    }

#endif //GDO_WINAPI

#ifdef GDO_ATEXIT
    if (_gdo_handle.call_free_lib_is_registered) {
        atexit(_gdo_call_free_lib);
        _gdo_handle.call_free_lib_is_registered = false;
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
    return (_gdo_handle.handle != NULL);
}
/***************************************************************************/



/***************************************************************************
* Free the library
****************************************************************************/
GDO_LINKAGE bool gdo_free_lib()
{
    _gdo_handle.buf[0] = 0;
#ifdef GDO_WINAPI
    _gdo_handle.buf_formatted[0] = 0;
    _gdo_handle.last_errno = 0;
    const gdo_char_t *msg = _T("FreeLibrary()");
#else
    const char *msg = NULL;
#endif

    if (!gdo_lib_is_loaded()) return true;

    if (!_gdo_free_lib()) {
        _gdo_save_error(msg, NULL);
        return false;
    }

    return true;
}

GDO_LINKAGE bool _gdo_free_lib()
{
#ifdef GDO_WINAPI
    if (FreeLibrary(_gdo_handle.handle) == FALSE)
#else
    if (dlclose(_gdo_handle.handle) != 0)
#endif
    {
        return false;
    }

    /* set pointers back to NULL */

    _gdo_handle.handle = NULL;

    _gdo_handle.GDO_SYMBOL_ptr_ = NULL;
    _gdo_handle.GDO_OBJ_SYMBOL_ptr_ = NULL;

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

    _gdo_handle.buf[0] = 0;
#ifdef GDO_WINAPI
    _gdo_handle.buf_formatted[0] = 0;
    _gdo_handle.last_errno = 0;
#endif

    /* already loaded all symbols */
    if (_gdo_handle.all_symbols_loaded) {
        return true;
    }

    /* no library was loaded */
    if (!gdo_lib_is_loaded()) {
#ifdef GDO_WINAPI
        _gdo_handle.last_errno = ERROR_INVALID_HANDLE;
        _tcsncpy(_gdo_handle.buf, _T("no library was loaded"), _GDO_ARRSZ(_gdo_handle.buf)-1);
#else
        strncpy(_gdo_handle.buf, "no library was loaded", sizeof(_gdo_handle.buf)-1);
#endif
        return false;
    }


    /* load function pointer addresses */

    _gdo_handle.GDO_SYMBOL_ptr_ = (GDO_TYPE (*)(GDO_ARGS))@
        _gdo_sym("GDO_SYMBOL", _T( "GDO_SYMBOL" ), &b);@
    if (!b) return false;@


    /* load object pointer addresses */

    _gdo_handle.GDO_OBJ_SYMBOL_ptr_ = (GDO_OBJ_TYPE*)@
        _gdo_sym("GDO_OBJ_SYMBOL", _T( "GDO_OBJ_SYMBOL" ), &b);@
    if (!b) return false;@


    _gdo_handle.all_symbols_loaded = true;

    return true;
}

GDO_LINKAGE void *_gdo_sym(const char *symbol, const gdo_char_t *msg, bool *b)
{
    *b = true;

#ifdef GDO_WINAPI
    void *ptr = (void *)GetProcAddress(_gdo_handle.handle, symbol);

    if (!ptr) {
        *b = false;
        _gdo_save_error(msg, NULL);
        _gdo_free_lib();
    }
#else
    (void)dlerror(); /* clear */
    void *ptr = dlsym(_gdo_handle.handle, symbol);

    /* NULL can be a valid value (unusual but possible),
     * so call dlerror() to check for errors */
    const char *p = dlerror();

    if (p) {
        *b = false;
        _gdo_save_error(msg, p);
        _gdo_free_lib();
    }
#endif /* !GDO_WINAPI */

    return ptr;
}
/***************************************************************************/



/***************************************************************************/
/* retrieve the last saved error message */
/***************************************************************************/
GDO_LINKAGE const gdo_char_t *gdo_last_error()
{
#ifdef GDO_WINAPI
    /* message was already saved */
    if (_gdo_handle.buf_formatted[0] != 0) {
        return _gdo_handle.buf_formatted;
    }

    gdo_char_t *msg = NULL;

    const DWORD dwFlags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_MAX_WIDTH_MASK;

    /* format the message */
    FormatMessage(dwFlags,
                NULL,
                _gdo_handle.last_errno,
                MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                (LPTSTR)&msg,
                0,
                NULL);

    if (msg) {
        _sntprintf(_gdo_handle.buf_formatted, _GDO_ARRSZ(_gdo_handle.buf_formatted)-1,
            _T("%ls: %ls"), _gdo_handle.buf, msg);
        LocalFree(msg);
    } else {
        _sntprintf(_gdo_handle.buf_formatted, _GDO_ARRSZ(_gdo_handle.buf_formatted)-1,
            _T("Last saved error code: %d"), (int)_gdo_handle.last_errno);
    }

    return _gdo_handle.buf_formatted;
#else
    /* simply return the buffer */
    return _gdo_handle.buf;
#endif //GDO_WINAPI
}
/***************************************************************************/



/***************************************************************************/
/* get the full library path - returns NULL on error */
/***************************************************************************/
GDO_LINKAGE gdo_char_t *gdo_lib_origin()
{
    _gdo_handle.buf[0] = 0;

#ifdef GDO_WINAPI
    gdo_char_t *origin;
    size_t len = 1024;
    _gdo_handle.buf_formatted[0] = 0;
    _gdo_handle.last_errno = 0;

    if (!gdo_lib_is_loaded()) {
        _gdo_handle.last_errno = ERROR_INVALID_HANDLE;
        _tcsncpy(_gdo_handle.buf, _T("no library was loaded"), _GDO_ARRSZ(_gdo_handle.buf)-1);
        return NULL;
    }

    origin = (gdo_char_t *)malloc(len * sizeof(gdo_char_t));

    if (!origin) {
        _gdo_save_error(_T("malloc"), NULL);
        return NULL;
    }

    if (GetModuleFileName(_gdo_handle.handle, origin, len-1) == 0) {
        _gdo_save_error(_T("GetModuleFileName"), NULL);
        free(origin);
        return NULL;
    }

    while (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        len += 1024;
        origin = (gdo_char_t *)realloc(origin, len * sizeof(gdo_char_t));

        if (GetModuleFileName(_gdo_handle.handle, origin, len-1) == 0) {
            _gdo_save_error(_T("GetModuleFileName"), NULL);
            free(origin);
            return NULL;
        }
    }

    return origin;
#else
    struct link_map *lm = NULL;

    if (!gdo_lib_is_loaded()) {
        strncpy(_gdo_handle.buf, "no library was loaded", sizeof(_gdo_handle.buf)-1);
        return NULL;
    } else if (dlinfo(_gdo_handle.handle, RTLD_DI_LINKMAP, &lm) == -1) {
        _gdo_save_error(NULL, NULL);
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

GDO_LINKAGE void _gdo_quick_load(const char *function, const char *symbol)
{
    const char *fmt = "error in wrapper function `%s' for symbol `%s':\n%s\n";

    if (gdo_load_lib_and_symbols()) {
        return;
    }

    /* print error message */

#if defined(_WIN32) && defined(GDO_USE_MESSAGE_BOX)
    const char *err = gdo_last_error();

    if (!err) err = "";
    if (!function) function = "";
    if (!symbol) symbol = "";

    const size_t len = strlen(fmt) + strlen(function) + strlen(symbol) + strlen(err);
    char *buf = (gdo_char_t *)malloc(len + 1);

    _snprintf(buf, fmt, function, symbol, err);
    MessageBoxA(NULL, buf, "Error", MB_OK | MB_ICONERROR);
    free(buf);
#else
    fprintf(stderr, fmt, function, symbol, gdo_last_error());
#endif

    /* free library handle and exit */

    if (gdo_lib_is_loaded()) {
        _gdo_free_lib();
    }

    exit(1);
}


/* wrapper functions */

GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
    _gdo_quick_load(__FUNCTION__, "GDO_SYMBOL");@
    GDO_RET _gdo_handle.GDO_SYMBOL_ptr_(GDO_NOTYPE_ARGS);@
}@

#undef _GDO_WRAP

#endif //GDO_USE_WRAPPER
/***************************************************************************/


#ifdef __cplusplus
} /* extern "C" */
#endif
