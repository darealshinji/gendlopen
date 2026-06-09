/*****************************************************************************/
/*                           C API implementation                            */
/*****************************************************************************/

#if defined(_MSC_VER) && defined(GDO_USE_MESSAGE_BOX)
# pragma comment(lib, "user32.lib")  /* dependency for MessageBox() */
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#ifdef _GDO_TARGET_WIDECHAR
# define GDO_XHS  L"%hs"  /* narrow character string */
#else
# define GDO_XHS  "%s"
#endif


/* linkage */
#ifdef GDO_STATIC
# define GDO_LINKAGE      GDO_DECL
# define GDO_OBJ_LINKAGE  GDO_OBJ_DECL
#else
# define GDO_LINKAGE      /**/
# define GDO_OBJ_LINKAGE  /**/
#endif


/* Silence `unused reference' compiler warnings. */
#define GDO_UNUSED_REF(x) (void)(x)


#ifndef _countof
# define _countof(array)  (sizeof(array) / sizeof(array[0]))
#endif


/* library handle */
GDO_OBJ_LINKAGE gdo_handle_t gdo_hndl;


/* forward declarations */
GDO_INLINE void _gdo_load_library(const gdo_char_t *filename, int flags, bool new_namespace);
GDO_INLINE void *_gdo_sym(const char *symbol, const gdo_char_t *msg);
#ifdef GDO_WINAPI
GDO_INLINE HMODULE _gdo_load_library_ex(const gdo_char_t *filename, int flags);
#endif
#ifdef _AIX
GDO_INLINE bool _gdo_aix_origin(struct ld_info *info, uint8_t *sym, char *buf, size_t bufsize);
#endif


/* strstr() / wcsstr() */
GDO_INLINE const gdo_char_t *_gdo_tcsstr(const gdo_char_t *haystack, const gdo_char_t *needle)
{
#ifdef _WIN32
    return _tcsstr(haystack, needle);
#else
    return strstr(haystack, needle);
#endif
}


/* snprintf() / snwprintf() */
GDO_INLINE void _gdo_snprintf(gdo_char_t *dest, size_t size, const gdo_char_t *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
#ifdef _WIN32
    _vsntprintf_s(dest, size, _TRUNCATE, fmt, ap);
#else
    vsnprintf(dest, size, fmt, ap);
#endif
    va_end(ap);
}

#define GDO_SNPRINTF(DEST, FORMAT, ...) \
    _gdo_snprintf(DEST, _countof(DEST), FORMAT, __VA_ARGS__)



/*****************************************************************************/
/*                                save error                                 */
/*****************************************************************************/
#ifdef GDO_WINAPI
# define GDO_SET_LAST_ERRNO(x)  do { gdo_hndl.last_errno = x; } while(0)
#else
# define GDO_SET_LAST_ERRNO(x)  /**/
#endif

/* save message to error buffer */
GDO_INLINE void _gdo_save_to_errbuf(const gdo_char_t *msg)
{
    if (msg) {
        GDO_SNPRINTF(gdo_hndl.errbuf, _T("%s"), msg);
    } else {
        gdo_hndl.errbuf[0] = 0;
    }

#ifdef GDO_WINAPI
    gdo_hndl.formatted[0] = 0;
#endif
}

#ifdef GDO_WINAPI

/* clear error buffers */
GDO_INLINE void _gdo_clear_error(void)
{
    gdo_hndl.errbuf[0] = 0;
    gdo_hndl.formatted[0] = 0;
    gdo_hndl.last_errno = 0;
}

/* save the last system error code; a message for additional information
 * can be provided too. */
GDO_INLINE void _gdo_save_error(const gdo_char_t *msg)
{
    gdo_hndl.last_errno = GetLastError();
    _gdo_save_to_errbuf(msg);
}

/* sets the "no library was loaded" error message */
GDO_INLINE void _gdo_set_error_no_library_loaded(void)
{
    gdo_hndl.last_errno = ERROR_INVALID_HANDLE;
    _gdo_save_to_errbuf(_T("no library was loaded"));
}

#else
/*********************************** dlfcn ***********************************/

/* clear error buffers */
GDO_INLINE void _gdo_clear_error(void)
{
    dlerror();
    gdo_hndl.errbuf[0] = 0;
}

/* save the last message provided by dlerror() */
GDO_INLINE void _gdo_save_error(const gdo_char_t *msg)
{
    GDO_UNUSED_REF(msg);
    _gdo_save_to_errbuf(dlerror());
}

/* sets the "no library was loaded" error message */
GDO_INLINE void _gdo_set_error_no_library_loaded(void)
{
    _gdo_save_to_errbuf("no library was loaded");
}

#endif //!GDO_WINAPI
/*****************************************************************************/



/*****************************************************************************/
/*                load default library with default flags                    */
/*****************************************************************************/
#ifdef GDO_DEFAULT_LIB
GDO_LINKAGE bool gdo_load_lib(void)
{
    return gdo_load_lib_name(GDO_DEFAULT_LIB);
}
#endif
/*****************************************************************************/



/*****************************************************************************/
/*       load default library with default flags and load the symbols        */
/*****************************************************************************/
#ifdef GDO_DEFAULT_LIB
GDO_LINKAGE bool gdo_load_lib_and_symbols(void)
{
    return (gdo_load_lib() && gdo_load_all_symbols());
}
#endif
/*****************************************************************************/



/*****************************************************************************/
/*            load library by filename with default flags                    */
/*****************************************************************************/
GDO_LINKAGE bool gdo_load_lib_name(const gdo_char_t *filename)
{
    return gdo_load_lib_args(filename, GDO_DEFAULT_FLAGS, false);
}
/*****************************************************************************/



/*****************************************************************************/
/*     load library by filename with default flags and load the symbols      */
/*****************************************************************************/
GDO_LINKAGE bool gdo_load_lib_name_and_symbols(const gdo_char_t *filename)
{
    return (gdo_load_lib_name(filename) && gdo_load_all_symbols());
}
/*****************************************************************************/



/*****************************************************************************/
/*                          load the library                                 */
/*****************************************************************************/
GDO_LINKAGE bool gdo_load_lib_args(const gdo_char_t *filename, int flags, bool new_namespace)
{
    _gdo_clear_error();

    /* consider it an error if the library was already loaded */
    if (gdo_lib_is_loaded()) {
        _gdo_save_to_errbuf(_T("library already loaded"));
        return false;
    }

    /* dlfcn: an empty filename will actually return a handle to
     * the main program, but we don't want that */
    if (!filename || *filename == 0) {
        GDO_SET_LAST_ERRNO(ERROR_INVALID_NAME);
        _gdo_save_to_errbuf(_T("empty filename"));
        return false;
    }

    _gdo_load_library(filename, flags, new_namespace);

    if (!gdo_lib_is_loaded()) {
        _gdo_save_error(filename);
        return false;
    }

    return true;
}

/* call LoadLibraryEx/dlopen/dlmopen */
GDO_INLINE void _gdo_load_library(const gdo_char_t *filename, int flags, bool new_namespace)
{
#ifdef GDO_WINAPI

    /* documentation says only backward slash path separators shall
     * be used on LoadLibraryEx() */

    if (!_tcschr(filename, _T('/'))) {
        /* no forward slash found */
        gdo_hndl.handle = _gdo_load_library_ex(filename, flags);
        return;
    }

    /* copy filename and replace path separators */
    gdo_char_t *copy = _tcsdup(filename);

    for (gdo_char_t *p = copy; *p != 0; p++) {
        if (*p == _T('/')) {
            *p = _T('\\');
        }
    }

    GDO_UNUSED_REF(new_namespace);
    gdo_hndl.handle = _gdo_load_library_ex(copy, flags);
    free(copy);

#else

    gdo_hndl.handle = _gdo_call_dlopen(filename, flags, new_namespace);

#endif //!GDO_WINAPI
}

#ifdef GDO_WINAPI
# if !defined(_GDO_TARGET_WIDECHAR) && defined(GDO_CONVERT_FILENAME)
GDO_INLINE wchar_t *_gdo_mbstowcs(const char *filename)
{
    wchar_t *wcs;
    size_t len = 0;
    size_t n = 0;

    /* get length of converted string (including null terminator) */
    if (mbstowcs_s(&len, NULL, 0, filename, _TRUNCATE) == 0 && len > 1) {
        wcs = (wchar_t *)malloc(len * sizeof(wchar_t));

        /* convert string */
        if (mbstowcs_s(&n, wcs, len, filename, _TRUNCATE) == 0 && n == len) {
            return wcs;
        }

        free(wcs);
    }

    return NULL;
}
# endif //!_GDO_TARGET_WIDECHAR && GDO_CONVERT_FILENAME

GDO_INLINE HMODULE _gdo_load_library_ex(const gdo_char_t *filename, int flags)
{
# if !defined(_GDO_TARGET_WIDECHAR) && defined(GDO_CONVERT_FILENAME)
    /* convert filename to wchar_t and use it on LoadLibraryExW() */
    wchar_t *wcs = _gdo_mbstowcs(filename);

    if (wcs) {
        HMODULE handle = LoadLibraryExW(wcs, NULL, flags);
        free(wcs);
        return handle;
    }

    GDO_SET_LAST_ERRNO(ERROR_INVALID_NAME);
    _gdo_save_to_errbuf("mbstowcs_s: failed to convert filename");
    return NULL;
# else
    return LoadLibraryEx(filename, NULL, flags);
# endif //!_GDO_TARGET_WIDECHAR && GDO_CONVERT_FILENAME
}
#endif //GDO_WINAPI
/*****************************************************************************/



/*****************************************************************************/
/*                whether the library is currently loaded                    */
/*****************************************************************************/
GDO_LINKAGE bool gdo_lib_is_loaded(void)
{
    return (gdo_hndl.handle != NULL);
}
/*****************************************************************************/



/*****************************************************************************/
/*             free the library handle and set pointers to NULL              */
/*****************************************************************************/
GDO_LINKAGE bool gdo_free_lib(void)
{
    _gdo_clear_error();

    if (gdo_lib_is_loaded()) {
        if (!_gdo_call_dlclose(gdo_hndl.handle))
        {
#ifdef GDO_WINAPI
            _gdo_save_error(_T("FreeLibrary()"));
#else
            _gdo_save_error(NULL);
#endif
            return false;
        }
    }

    /* set pointers back to NULL */
    gdo_hndl.handle = NULL;
    GDO_RAWPTR_%%symbol%% = NULL;

    return true;
}
/*****************************************************************************/



/*****************************************************************************/
/*    free the library handle and set pointers to NULL (no error checks)     */
/*****************************************************************************/
GDO_LINKAGE void gdo_force_free_lib(void)
{
    _gdo_clear_error();

    if (gdo_lib_is_loaded()) {
        _gdo_call_dlclose(gdo_hndl.handle);
    }

    /* set pointers back to NULL */
    gdo_hndl.handle = NULL;
    GDO_RAWPTR_%%symbol%% = NULL;
}
/*****************************************************************************/



/*****************************************************************************/
/*                   automatically free library upon exit                    */
/*****************************************************************************/
GDO_LINKAGE bool gdo_enable_autorelease(void)
{
    _gdo_clear_error();

    if (!gdo_hndl.free_lib_reg) {
        if (atexit(gdo_force_free_lib) == 0) {
            gdo_hndl.free_lib_reg = true;
        } else {
            _gdo_save_to_errbuf(_T("atexit(): failed to register `gdo_force_free_lib()'"));
        }
    }

    return gdo_hndl.free_lib_reg;
}
/*****************************************************************************/



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
    void *ptr = _gdo_call_dlsym(gdo_hndl.handle, symbol);

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

    /* check symbol prefix first */
    const char pfx[] = GDO_COMMON_PREFIX;
    const size_t pfxlen = sizeof(pfx) - 1;

    if (pfxlen == 0 || strncmp(symbol, pfx, pfxlen) == 0) {
        const size_t len = strlen(symbol);
        const char *curr;
        size_t curr_len;
@
        curr = "%%symbol%%";@
        curr_len = sizeof("%%symbol%%") - 1;@
        if (len == curr_len && strcmp(symbol + pfxlen, curr + pfxlen) == 0) {@
            if (!GDO_RAWPTR_%%symbol%%) {@
                GDO_RAWPTR_%%symbol%% =@
                    (%%sym_type%%)@
                        _gdo_sym("%%symbol%%", _T("%%symbol%%"));@
            }@
            return (GDO_RAWPTR_%%symbol%% != NULL);@
        }
    }

    GDO_SET_LAST_ERRNO(ERROR_NOT_FOUND);
    GDO_SNPRINTF(gdo_hndl.errbuf, _T("unknown symbol: ") GDO_XHS, symbol);

    return false;
}
/*****************************************************************************/



/*****************************************************************************/
/*                   retrieve the last saved error message                   */
/*****************************************************************************/
GDO_LINKAGE const gdo_char_t *gdo_last_error(void)
{
#ifdef GDO_WINAPI

    /* formatted message was already saved */
    if (gdo_hndl.formatted[0] != 0) {
        return gdo_hndl.formatted;
    }

    gdo_char_t *buf = NULL;

    FormatMessage(GDO_FORMAT_MESSAGE_FLAGS,
                  NULL,
                  gdo_hndl.last_errno,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (gdo_char_t *)&buf,
                  0,
                  NULL);

    if (buf) {
        if (gdo_hndl.errbuf[0] != 0) {
            /* put custom message in front */
            GDO_SNPRINTF(gdo_hndl.formatted, _T("%s: %s"), gdo_hndl.errbuf, buf);
        } else {
            GDO_SNPRINTF(gdo_hndl.formatted, _T("%s"), buf);
        }

        LocalFree(buf);
    } else {
        /* FormatMessage() failed, save the error code */
        if (gdo_hndl.errbuf[0] != 0) {
            /* put custom message in front */
            GDO_SNPRINTF(gdo_hndl.formatted, _T("%s: error code: %zu"), gdo_hndl.errbuf,
                (size_t)gdo_hndl.last_errno);
        } else {
            GDO_SNPRINTF(gdo_hndl.formatted, _T("error code: %zu"), (size_t)gdo_hndl.last_errno);
        }
    }

    return gdo_hndl.formatted;

#else

    if (gdo_hndl.errbuf[0] == 0) {
        _gdo_save_to_errbuf("no error");
    }

    return gdo_hndl.errbuf;

#endif //GDO_WINAPI
}
/*****************************************************************************/



/*****************************************************************************/
/*                        get the loaded library path                        */
/*****************************************************************************/
GDO_LINKAGE const gdo_char_t *gdo_lib_origin(void)
{
    _gdo_clear_error();

    if (!gdo_lib_is_loaded()) {
        _gdo_set_error_no_library_loaded();
        return NULL;
    }

#ifdef GDO_WINAPI

    /* use GetModuleFileName() */

    const gdo_char_t *msg = (sizeof(gdo_char_t) == 1)
        ? _T("GetModuleFileNameA()")
        : _T("GetModuleFileNameW()");

    static gdo_char_t buf[GDO_BUFLEN];
    DWORD nSize = GetModuleFileName(gdo_hndl.handle, buf, GDO_BUFLEN);

    if (nSize == 0 || nSize == GDO_BUFLEN) {
        _gdo_save_error(msg);
        return NULL;
    }

    return buf;

#elif defined(GDO_DLFCN_WIN32)

    /* dlfcn-win32:
     * The handle returned by dlopen() is a `HMODULE' casted to `void *'.
     * We can directly use GetModuleFileNameA() to receive the DLL path
     * and don't need to invoke dladdr() on a loaded symbol address. */

    static char buf[GDO_BUFLEN];
    DWORD nSize = GetModuleFileNameA((HMODULE)gdo_hndl.handle, buf, GDO_BUFLEN);

    if (nSize == 0) {
        _gdo_save_to_errbuf("failed to get the library path");
        return NULL;
    } else if (nSize == GDO_BUFLEN) {
        _gdo_save_to_errbuf("buffer is too small to hold the library path");
        return NULL;
    }

    return buf;

#elif defined(_AIX)

    /* AIX's equivalent of dladdr() */
    static char buf[GDO_BUFLEN];
    uint8_t q[GDO_AIX_LOADQUERY_BUFLEN];

    if (gdo_no_symbols_loaded()) {
        _gdo_save_to_errbuf("no symbols were loaded");
        return NULL;
    }

    if (loadquery(L_GETINFO, q, sizeof(q)) != -1) {
        struct ld_info *info = (struct ld_info *)q;
@
        if (_gdo_aix_origin(info, (uint8_t *)GDO_RAWPTR_%%symbol%%, buf, GDO_BUFLEN)) {@
            return buf;@
        }
    }

    _gdo_save_to_errbuf("failed to get the library path");
    return NULL;

#elif defined(GDO_HAVE_DLINFO)

    static char buf[GDO_BUFLEN];

    /* use dlinfo() to get a link map */
    struct link_map *lm = NULL;

    if (dlinfo(gdo_hndl.handle, RTLD_DI_LINKMAP, &lm) == -1) {
        _gdo_save_error(NULL);
        return NULL;
    }

    if (!lm->l_name || lm->l_name[0] == 0) {
        _gdo_save_to_errbuf("dlinfo() failed to get library path");
        return NULL;
    }

# ifdef __linux__
    /* try to get the full library path from process map */
    if (lm->l_name[0] != '/' && _gdo_fullpath_procmap(lm, buf, GDO_BUFLEN)) {
        return buf;
    }
# endif

    size_t len = strlen(lm->l_name);

    if (len < GDO_BUFLEN) {
        memcpy(buf, lm->l_name, len + 1);
        return buf;
    }

    return NULL;

#elif defined(GDO_HAVE_DLADDR)

    /* use dladdr() to get the library path from a symbol pointer */
    static Dl_info info;

    if (gdo_no_symbols_loaded()) {
        _gdo_save_to_errbuf("no symbols were loaded");
        return NULL;
    }

    if (_gdo_call_dladdr((void *)GDO_RAWPTR_%%symbol%%, &info)) {@
        return info.dli_fname;@
    }@

    _gdo_save_to_errbuf("dladdr() failed to get library path");
    return NULL;

#else

    _gdo_save_to_errbuf("function not implemented");
    return NULL;

#endif
}

#ifdef _AIX
GDO_INLINE bool _gdo_aix_origin(struct ld_info *info, uint8_t *sym, char *buf, size_t bufsize)
{
    size_t len;
    const char *member = NULL;
    const char *path = _gdo_aix_parse_ldinfo(info, sym, (const char **)&member);

    if (!path) {
        return false;
    }

    /* check for an archive member name */
    if (member && member[0] != 0) {
        len = strlen(path) + strlen(member) + 3;

        if (len > bufsize) {
            return false;
        }

        snprintf(buf, len, "%s(%s)", path, member);
    } else {
        /* path is not an archive */
        len = strlen(path) + 1;

        if (len > bufsize) {
            return false;
        }

        memcpy(buf, path, len);
    }

    return true;
}
#endif
/*****************************************************************************/
%PARAM_SKIP_REMOVE_BEGIN%



/*****************************************************************************/
/*                     helper used by wrapper functions                      */
/*****************************************************************************/
#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

GDO_INLINE void _gdo_print_error(const gdo_char_t *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

#ifdef _WIN32

# ifdef GDO_USE_MESSAGE_BOX
    /* we can safely use gdo_hndl.errbuf, the last error message
     * was saved in gdo_hndl.formatted */
    _vsntprintf_s(gdo_hndl.errbuf, GDO_BUFLEN, _TRUNCATE, fmt, ap);
    MessageBox(NULL, gdo_hndl.errbuf, _T("Error"), MB_OK | MB_ICONERROR);
    gdo_hndl.errbuf[0] = 0;
# else
    _vftprintf_s(stderr, fmt, ap);
    _fputtc(_T('\n'), stderr);
# endif //GDO_USE_MESSAGE_BOX

#else //!_WIN32

    vfprintf(stderr, fmt, ap);
    fputc('\n', stderr);

#endif //!_WIN32

    va_end(ap);
}

/* used by wrapper functions */
GDO_LINKAGE void _gdo_wrap_check_loaded(void *symptr, int load, const gdo_char_t *sym)
{
    const gdo_char_t *msg;

    /* symbol was already loaded */
    if (symptr != NULL) {
        return;
    }

#ifdef GDO_ENABLE_AUTOLOAD
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
    GDO_UNUSED_REF(load);

    /* load all symbols */
    if (gdo_load_all_symbols()) {
        return;
    }
# endif

    msg = gdo_last_error();

    if (_gdo_tcsstr(msg, GDO_DEFAULT_LIB)) {
        /* library name is already part of error message */
        _gdo_print_error(_T("error: %s: %s"), sym, msg);
    } else {
        _gdo_print_error(_T("error: ") GDO_DEFAULT_LIB _T(": %s: %s"), sym, msg);
    }

    gdo_force_free_lib();
    exit(1);

#else //!GDO_ENABLE_AUTOLOAD

    /* don't load anything, print an error message and abort */

    if (!gdo_lib_is_loaded()) {
        msg = _T("library not loaded");
    } else {
        msg = _T("symbol not loaded");
    }

    GDO_UNUSED_REF(load);
    _gdo_print_error(_T("fatal error: %s: %s"), sym, msg);

    abort();

#endif //!GDO_ENABLE_AUTOLOAD
}

#endif // GDO_WRAP_FUNCTIONS || GDO_ENABLE_AUTOLOAD
/*****************************************************************************/
%PARAM_SKIP_END%


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

