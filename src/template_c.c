#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef GDO_WINAPI
GDO_LINKAGE HMODULE _gdo_handle = NULL;
GDO_LINKAGE DWORD _gdo_last_errno = 0;
GDO_LINKAGE gdo_char_t _gdo_buf_formatted[64*1024] = {0};
#else
GDO_LINKAGE void *_gdo_handle = NULL;
#endif //GDO_WINAPI

GDO_LINKAGE bool _gdo_call_free_lib_is_registered = false;
GDO_LINKAGE bool _gdo_all_symbols_loaded = false;
GDO_LINKAGE gdo_char_t _gdo_buf[4*1024] = {0};

GDO_LINKAGE inline void _gdo_register_atexit();
GDO_LINKAGE bool _gdo_load_symbols_error(const gdo_char_t *msg);



/***************************************************************************/
/* typedefs */
/***************************************************************************/
typedef GDO_TYPE (*_gdo_GDO_SYMBOL_t)(GDO_ARGS);
/***************************************************************************/



/***************************************************************************/
/* function pointers */
/***************************************************************************/
GDO_LINKAGE _gdo_GDO_SYMBOL_t _gdo_GDO_SYMBOL_ptr_ = NULL;
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
/* load the library */
/***************************************************************************/
GDO_LINKAGE bool gdo_load_lib_args(const gdo_char_t *filename, int flags, bool new_namespace)
{
    _gdo_buf[0] = 0;
#ifdef GDO_WINAPI
    _gdo_buf_formatted[0] = 0;
    _gdo_last_errno = 0;
#endif

    /* library already loaded */
    if (_gdo_handle) {
        return true;
    }

#ifdef GDO_WINAPI

    /* win32 */
    if ((_gdo_handle = LoadLibraryEx(filename, NULL, flags)) == NULL) {
        _gdo_last_errno = GetLastError();
        if (filename) _tcsncpy(_gdo_buf, filename, sizeof(_gdo_buf)-1);
        return false;
    }

#else //!GDO_WINAPI

#ifdef GDO_NO_DLMOPEN

    /* disable dlmopen() */
    (void)new_namespace;
    _gdo_handle = dlopen(filename, flags);

#else //!GDO_NO_DLMOPEN

    /* dlopen() or dlmopen() for new namespace */
    if (new_namespace) {
        _gdo_handle = dlmopen(LM_ID_NEWLM, filename, flags);
    } else {
        _gdo_handle = dlopen(filename, flags);
    }

#endif //GDO_NO_DLMOPEN

    if (!_gdo_handle) {
        char *ptr = dlerror();
        if (ptr) strncpy(_gdo_buf, ptr, sizeof(_gdo_buf)-1);
        return false;
    }

#endif //GDO_WINAPI

    _gdo_register_atexit();

    return true;
}
/***************************************************************************/



/***************************************************************************
* Free the library
****************************************************************************/
GDO_LINKAGE bool gdo_free_lib()
{
    _gdo_buf[0] = 0;
#ifdef GDO_WINAPI
    _gdo_buf_formatted[0] = 0;
    _gdo_last_errno = 0;
#endif

    if (!_gdo_handle) return true;

#ifdef GDO_WINAPI

    if (FreeLibrary(_gdo_handle) == TRUE) {
        _gdo_handle = NULL;
        return true;
    }

    _gdo_last_errno = GetLastError();
    _tcscpy(_gdo_buf, _T("FreeLibrary()"));

#else //!GDO_WINAPI

    if (dlclose(_gdo_handle) == 0) {
        _gdo_handle = NULL;
        return true;
    }

    char *ptr = dlerror();
    if (ptr) strncpy(_gdo_buf, ptr, sizeof(_gdo_buf)-1);

#endif //GDO_WINAPI

    return false;
}
/***************************************************************************/



/***************************************************************************/
/* load all symbols; can safely be called multiple times.
 */
/***************************************************************************/
GDO_LINKAGE bool gdo_load_symbols()
{
    /* already loaded all symbols */
    if (_gdo_all_symbols_loaded) {
        return true;
    }

    /* no library was loaded */
    if (!_gdo_handle) {
#ifdef GDO_WINAPI
        _gdo_buf_formatted[0] = 0;
        _gdo_last_errno = ERROR_INVALID_HANDLE;
        _tcsncpy(_gdo_buf, _T("no library was loaded"), sizeof(_gdo_buf)-1);
#else
        strncpy(_gdo_buf, "no library was loaded", sizeof(_gdo_buf)-1);
#endif //GDO_WINAPI

        return false;
    }

#ifdef GDO_WINAPI
    #define LOAD_SYMBOL(TYPE, PTR, SYMBOL) \
        if (!PTR) PTR = (TYPE) GetProcAddress(_gdo_handle, SYMBOL); \
        if (!PTR) return _gdo_load_symbols_error( _T( SYMBOL ) )
#else
    #define LOAD_SYMBOL(TYPE, PTR, SYMBOL) \
        if (!PTR) PTR = (TYPE) dlsym(_gdo_handle, SYMBOL); \
        if (!PTR) return _gdo_load_symbols_error( SYMBOL )
#endif

    /* load function pointer addresses */
    LOAD_SYMBOL(_gdo_GDO_SYMBOL_t, _gdo_GDO_SYMBOL_ptr_, "GDO_SYMBOL");

#undef LOAD_SYMBOL

    _gdo_all_symbols_loaded = true;

    return true;
}

GDO_LINKAGE bool _gdo_load_symbols_error(const gdo_char_t *msg)
{
#ifdef GDO_WINAPI
    _gdo_buf_formatted[0] = 0;
    _gdo_last_errno = GetLastError();
    _tcsncpy(_gdo_buf, msg, sizeof(_gdo_buf)-1);
    FreeLibrary(_gdo_handle);
#else
    char *ptr = dlerror();
    strncpy(_gdo_buf, ptr ? ptr : msg, sizeof(_gdo_buf)-1);
    dlclose(_gdo_handle);
#endif
    _gdo_handle = NULL;

    /* always return false */
    return false;
}
/***************************************************************************/



/***************************************************************************/
/* whether the library is currently loaded */
/***************************************************************************/
GDO_LINKAGE bool gdo_lib_is_loaded()
{
    return (_gdo_handle != NULL);
}
/***************************************************************************/



/***************************************************************************/
/* retrieve the last saved error message */
/***************************************************************************/
GDO_LINKAGE const gdo_char_t *gdo_last_error()
{
#ifdef GDO_WINAPI

    /* message was already saved */
    if (_gdo_buf_formatted[0] != 0) {
        return _gdo_buf_formatted;
    }

#ifdef _UNICODE
    const WCHAR *fmt = L"%ls: %ls";
#else
    const char *fmt = "%s: %s";
#endif

    gdo_char_t *msg = NULL;

    const DWORD dwFlags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_MAX_WIDTH_MASK;

    /* format the message */
    DWORD ret = FormatMessage(dwFlags, NULL, _gdo_last_errno,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&msg, 0, NULL);

    if (ret == 0 || !msg) {
        /* FormatMessage failed */
        _sntprintf(_gdo_buf_formatted, sizeof(_gdo_buf_formatted)-1,
            _T("Last saved error code: %d"), (int)_gdo_last_errno);

        if (msg) LocalFree(msg);
    } else if (msg) {
        if (_gdo_buf[0] == 0) {
            _tcsncpy(_gdo_buf_formatted, msg, sizeof(_gdo_buf_formatted)-1);
        } else {
            _sntprintf(_gdo_buf_formatted, sizeof(_gdo_buf_formatted)-1,
                fmt, _gdo_buf, msg);
        }

        LocalFree(msg);
    }

    return _gdo_buf_formatted;
#else
    /* simply return the buffer */
    return _gdo_buf;
#endif //GDO_WINAPI
}
/***************************************************************************/



/***************************************************************************/
/* get the full library path - returns NULL on error */
/***************************************************************************/
GDO_LINKAGE const gdo_char_t *gdo_lib_origin()
{
    _gdo_buf[0] = 0;

#ifdef GDO_WINAPI
    _gdo_buf_formatted[0] = 0;
    _gdo_last_errno = 0;

    static gdo_char_t origin[36*1024] = {0};

    if (!_gdo_handle) {
        _gdo_last_errno = ERROR_INVALID_HANDLE;
        return NULL;
    } else if (GetModuleFileName(_gdo_handle, (LPTSTR)&origin, sizeof(origin)-1) == 0) {
        _gdo_last_errno = GetLastError();
        return NULL;
    }
#else
    static char origin[4*1024] = {0};
    struct link_map *lm = NULL;

    if (!_gdo_handle) {
        strncpy(_gdo_buf, "no library was loaded", sizeof(_gdo_buf)-1);
        return NULL;
    } else if (dlinfo(_gdo_handle, RTLD_DI_LINKMAP, &lm) == -1) {
        char *ptr = dlerror();
        if (ptr) strncpy(_gdo_buf, ptr, sizeof(_gdo_buf)-1);
        return NULL;
    }

    if (lm->l_name) {
        strncpy(origin, lm->l_name, sizeof(origin)-1);
    }
#endif //GDO_WINAPI

    return origin;
}
/***************************************************************************/


#ifdef GDO_ATEXIT
GDO_LINKAGE void _gdo_call_free_lib();
#endif


/***************************************************************************/
/* register dlcose() call with atexit() */
/***************************************************************************/
GDO_LINKAGE inline void _gdo_register_atexit()
{
#ifdef GDO_ATEXIT
    if (_gdo_call_free_lib_is_registered) {
        atexit(_gdo_call_free_lib);
        _gdo_call_free_lib_is_registered = false;
    }
#endif //GDO_ATEXIT
}

#ifdef GDO_ATEXIT
GDO_LINKAGE void _gdo_call_free_lib()
{
#ifdef GDO_WINAPI
    if (_gdo_handle) FreeLibrary(_gdo_handle);
#else
    if (_gdo_handle) dlclose(_gdo_handle);
#endif
}
#endif //GDO_ATEXIT
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
    if (gdo_load_lib() && gdo_load_symbols()) {
        return;
    }

    /* print error message */

#if defined(_WIN32) && defined(GDO_USE_MESSAGE_BOX)
    const char *err = gdo_last_error();

    if (!err) err = "";
    if (!function) function = "";
    if (!symbol) symbol = "";

    const char *fmt = "error in wrapper function `%s' for symbol `%s':\n%s\n";
    const size_t len = strlen(fmt) + strlen(function) + strlen(symbol) + strlen(err);
    char *buf = malloc(len + 1);

    _snprintf(buf, fmt, function, symbol, err);
    MessageBoxA(NULL, buf, "Error", MB_OK | MB_ICONERROR);
    free(buf);
#else
    fprintf(stderr, "error in wrapper function `%s' for symbol `%s':\n", function, symbol);
    fprintf(stderr, "%s\n", gdo_last_error());
#endif

    /* free library handle and exit */

#ifdef GDO_WINAPI
    if (_gdo_handle) FreeLibrary(_gdo_handle);
#else
    if (_gdo_handle) dlclose(_gdo_handle);
#endif

    exit(1);
}


/* wrapper functions */

GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
    _gdo_quick_load(__FUNCTION__, "GDO_SYMBOL");@
    GDO_RET _gdo_GDO_SYMBOL_ptr_(GDO_NOTYPE_ARGS);@
}@

#undef _GDO_WRAP

#endif //GDO_USE_WRAPPER
/***************************************************************************/
