/*****************************************************************************/
/*                           C API implementation                            */
/*****************************************************************************/

#if defined(_MSC_VER) && defined(GDO_USE_MESSAGE_BOX)
# pragma comment(lib, "user32.lib")
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef GDO_WINAPI
# define _gdo_ftprintf     _ftprintf
# define _gdo_sntprintf    _sntprintf
# define _gdo_sntprintf_s  _sntprintf_s
# define _gdo_tcsstr       _tcsstr
#else
/* dlfcn: use `char' API */
# define _gdo_ftprintf     fprintf
# define _gdo_sntprintf    snprintf
# define _gdo_sntprintf_s  _snprintf_s
# define _gdo_tcsstr       strstr
#endif

#ifdef _MSC_VER
# define _gdo_strdup  _strdup
#else
# define _gdo_strdup  strdup
#endif

#ifdef _GDO_TARGET_WIDECHAR
# define GDO_XHS  L"%hs"  /* always type LPSTR */
#else
# define GDO_XHS  "%s"
#endif


/* linkage */
#ifdef GDO_STATIC
# define GDO_LINKAGE      static inline
# define GDO_OBJ_LINKAGE  static
#else
# define GDO_LINKAGE      /**/
# define GDO_OBJ_LINKAGE  /**/
#endif

#define GDO_INLINE  static inline


/* see GetLastError() */
#ifdef GDO_WINAPI
# define GDO_SET_LAST_ERRNO(x)  do { gdo_hndl.last_errno = x; } while(0)
#else
# define GDO_SET_LAST_ERRNO(x)  /**/
#endif

#ifndef _countof
# define _countof(array)  (sizeof(array) / sizeof(array[0]))
#endif


typedef void GDO_UNUSED_REF;


/* library handle */
GDO_OBJ_LINKAGE gdo_handle_t gdo_hndl;


/* forward declarations */
GDO_INLINE void _gdo_load_library(const gdo_char_t *filename, int flags, bool new_namespace);
GDO_INLINE void *_gdo_sym(const char *symbol, const gdo_char_t *msg) GDO_ATTR (nonnull);
#if !defined(GDO_WINAPI) && !defined(GDO_HAVE_DLINFO)
GDO_INLINE char *_gdo_dladdr_get_fname(const void *ptr) GDO_ATTR (warn_unused_result);
#endif



/*****************************************************************************/
/*                                save error                                 */
/*****************************************************************************/

/* GDO_SNPRINTF: use as a macro so we can use __VA_ARGS__ directly;
 * don't trust implementations and always explicitly null-termintate the string */
#ifdef _MSC_VER
# define GDO_SNPRINTF(DEST, FORMAT, ...) \
    do { /* make MSVC happy by using the *_s function */ \
        _gdo_sntprintf_s(DEST, _countof(DEST), _TRUNCATE, FORMAT, __VA_ARGS__); \
        DEST[_countof(DEST) - 1] = 0; \
    } while (0)
#else
# define GDO_SNPRINTF(DEST, FORMAT, ...) \
    do { \
        _gdo_sntprintf(DEST, _countof(DEST), FORMAT, __VA_ARGS__); \
        DEST[_countof(DEST) - 1] = 0; \
    } while (0)
#endif

/* save message to error buffer */
GDO_INLINE void _gdo_save_to_errbuf(const gdo_char_t *msg)
{
#ifdef GDO_WINAPI
    gdo_hndl.buf_formatted[0] = 0;
#endif

    if (msg) {
        GDO_SNPRINTF(gdo_hndl.buf, GDO_T("%s"), msg);
    } else {
        gdo_hndl.buf[0] = 0;
    }
}

#ifdef GDO_WINAPI

/* clear error buffers */
GDO_INLINE void _gdo_clear_error(void)
{
    gdo_hndl.buf[0] = 0;
    gdo_hndl.buf_formatted[0] = 0;
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
    _gdo_save_to_errbuf(GDO_T("no library was loaded"));
}

#else
/*********************************** dlfcn ***********************************/

/* clear error buffers */
GDO_INLINE void _gdo_clear_error(void)
{
    dlerror();
    gdo_hndl.buf[0] = 0;
}

/* save the last message provided by dlerror() */
GDO_INLINE void _gdo_save_error(const gdo_char_t *msg)
{
    (GDO_UNUSED_REF) msg;
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
        _gdo_save_to_errbuf(GDO_T("library already loaded"));
        return false;
    }

    /* dlfcn: an empty filename will actually return a handle to
     * the main program, but we don't want that */
    if (!filename || *filename == 0) {
        GDO_SET_LAST_ERRNO(ERROR_INVALID_NAME);
        _gdo_save_to_errbuf(GDO_T("empty filename"));
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
    _gdo_clear_error();

#ifdef GDO_WINAPI

    /* documentation says only backward slash path separators shall
     * be used on LoadLibraryEx() */

    if (_tcschr(filename, GDO_T('/')) == NULL) {
        /* no forward slash found */
        gdo_hndl.handle = LoadLibraryEx(filename, NULL, flags);
        return;
    }

    gdo_char_t *copy = _tcsdup(filename);

    for (gdo_char_t *p = copy; *p != 0; p++) {
        if (*p == GDO_T('/')) {
            *p = GDO_T('\\');
        }
    }

    (GDO_UNUSED_REF) new_namespace;
    gdo_hndl.handle = LoadLibraryEx(copy, NULL, flags);
    free(copy);

#elif defined(GDO_HAVE_DLMOPEN)

    /* call dlmopen() for new namespace, otherwise dlopen() */
    if (new_namespace) {
        gdo_hndl.handle = dlmopen(LM_ID_NEWLM, filename, flags);
    } else {
        gdo_hndl.handle = dlopen(filename, flags);
    }

#else

    /* no dlmopen() */
    (GDO_UNUSED_REF) new_namespace;
    gdo_hndl.handle = dlopen(filename, flags);

#endif //!GDO_WINAPI
}
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
    bool ret;
    const gdo_char_t *msg = NULL;

    _gdo_clear_error();

    if (gdo_lib_is_loaded()) {
#ifdef GDO_WINAPI
        ret = (FreeLibrary(gdo_hndl.handle) == TRUE);
        msg = GDO_T("FreeLibrary()");
#else
        ret = (dlclose(gdo_hndl.handle) == 0);
#endif

        if (!ret) {
            _gdo_save_error(msg);
            return false;
        }
    }

    /* set pointers back to NULL */
    gdo_hndl.handle = NULL;
    gdo_hndl.ptr.%%symbol%% = NULL;

    return true;
}
/*****************************************************************************/



/*****************************************************************************/
/*    free the library handle and set pointers to NULL (no error checks)     */
/*****************************************************************************/
GDO_LINKAGE void gdo_force_free_lib(void)
{
    if (gdo_lib_is_loaded()) {
#ifdef GDO_WINAPI
        FreeLibrary(gdo_hndl.handle);
#else
        dlclose(gdo_hndl.handle);
#endif
    }

    _gdo_clear_error();

    /* set pointers back to NULL */
    gdo_hndl.handle = NULL;
    gdo_hndl.ptr.%%symbol%% = NULL;
}
/*****************************************************************************/



/*****************************************************************************/
/*                   automatically free library upon exit                    */
/*****************************************************************************/
GDO_LINKAGE bool gdo_enable_autorelease(void)
{
    _gdo_clear_error();

    if (!gdo_hndl.free_lib_registered) {
        if (atexit(gdo_force_free_lib) == 0) {
            gdo_hndl.free_lib_registered = true;
        } else {
            _gdo_save_to_errbuf(GDO_T("atexit(): failed to register `gdo_force_free_lib()'"));
        }
    }

    return gdo_hndl.free_lib_registered;
}
/*****************************************************************************/



/*****************************************************************************/
/*                    check if ALL symbols were loaded                       */
/*****************************************************************************/
GDO_LINKAGE bool gdo_all_symbols_loaded(void)
{
    if (true
        && gdo_hndl.ptr.%%symbol%% != NULL
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
        && gdo_hndl.ptr.%%symbol%% == NULL
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
        || gdo_hndl.ptr.%%symbol%% != NULL
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

    /* already loaded all symbols */
    if (gdo_all_symbols_loaded()) {
        return true;
    }

    /* no library was loaded */
    if (!gdo_lib_is_loaded()) {
        _gdo_set_error_no_library_loaded();
        return false;
    }

    /* get symbol addresses */

    /* %%symbol%% */@
    if ((gdo_hndl.ptr.%%symbol%% =@
            (%%sym_type%%)@
                _gdo_sym("%%symbol%%", GDO_T("%%symbol%%"))) == NULL) {@
        return false;@
    }@

    return gdo_all_symbols_loaded();
}

GDO_INLINE void *_gdo_sym(const char *symbol, const gdo_char_t *msg)
{
    void *ptr;

    _gdo_clear_error();

#ifdef GDO_WINAPI
    ptr = (void *)GetProcAddress(gdo_hndl.handle, symbol);
#else
    ptr = dlsym(gdo_hndl.handle, symbol);
#endif

    if (!ptr) {
        _gdo_save_error(msg);
    }

    return ptr;
}
/*****************************************************************************/



/*****************************************************************************/
/*                        load a specific symbol                             */
/*                                                                           */
/* The main intention is to check if a certain symbol is present in a        */
/* library so that you can conditionally enable or disable features.         */
/* `symbol_num' is an enumeration value `GDO_LOAD_<symbol_name>'             */
/*****************************************************************************/
GDO_LINKAGE bool gdo_load_symbol(int symbol_num)
{
    _gdo_clear_error();

    /* no library was loaded */
    if (!gdo_lib_is_loaded()) {
        _gdo_set_error_no_library_loaded();
        return false;
    }

    switch (symbol_num)
    {
    /* %%symbol%% */@
    case GDO_LOAD_%%symbol%%:@
        if (!gdo_hndl.ptr.%%symbol%%) {@
            gdo_hndl.ptr.%%symbol%% =@
                (%%sym_type%%)@
                    _gdo_sym("%%symbol%%", GDO_T("%%symbol%%"));@
        }@
        return (gdo_hndl.ptr.%%symbol%% != NULL);@

    default:
        break;
    }

    GDO_SET_LAST_ERRNO(ERROR_NOT_FOUND);

    GDO_SNPRINTF(gdo_hndl.buf, GDO_T("unknown symbol number: %d"), symbol_num);

    return false;
}
/*****************************************************************************/



/*****************************************************************************/
/*                    load a specific symbol by name                         */
/*                                                                           */
/* The main intention is to check if a certain symbol is present in a        */
/* library so that you can conditionally enable or disable features.         */
/*****************************************************************************/
GDO_LINKAGE bool gdo_load_symbol_name(const char *symbol)
{
    _gdo_clear_error();

    /* no library was loaded */
    if (!gdo_lib_is_loaded()) {
        _gdo_set_error_no_library_loaded();
        return false;
    }

    if (!symbol || *symbol == 0) {
        GDO_SET_LAST_ERRNO(ERROR_INVALID_PARAMETER);
        _gdo_save_to_errbuf(GDO_T("empty symbol name"));
        return false;
    }

    /* check symbol prefix */
    const size_t n = sizeof(GDO_COMMON_PREFIX) - 1;

    if (n > 0 && strncmp(symbol, GDO_COMMON_PREFIX, n) != 0) {
        GDO_SET_LAST_ERRNO(ERROR_NOT_FOUND);
        GDO_SNPRINTF(gdo_hndl.buf, GDO_T("unknown symbol: ") GDO_XHS, symbol);
        return false;
    }

    /* symbols */
    const size_t len = strlen(symbol);
    const char *ptr;

    ptr = "%%symbol%%";@
    @
    if (len == sizeof("%%symbol%%") - 1 &&@
        strcmp(symbol + n, ptr + n) == 0)@
    {@
        if (!gdo_hndl.ptr.%%symbol%%) {@
            gdo_hndl.ptr.%%symbol%% =@
                (%%sym_type%%)@
                    _gdo_sym("%%symbol%%", GDO_T("%%symbol%%"));@
        }@
        return (gdo_hndl.ptr.%%symbol%% != NULL);@
    }@

    GDO_SET_LAST_ERRNO(ERROR_NOT_FOUND);
    GDO_SNPRINTF(gdo_hndl.buf, GDO_T("unknown symbol: ") GDO_XHS, symbol);

    return false;
}
/*****************************************************************************/



/*****************************************************************************/
/*                   retrieve the last saved error message                   */
/*                                                                           */
/* For WinAPI the message will be generated from an error code.              */
/*****************************************************************************/
GDO_LINKAGE const gdo_char_t *gdo_last_error(void)
{
#ifdef GDO_WINAPI

    /* message was already saved */
    if (gdo_hndl.buf_formatted[0] != 0) {
        return gdo_hndl.buf_formatted;
    }

    gdo_char_t *buf = NULL;
    gdo_char_t *msg = gdo_hndl.buf;

    /* format the message */
    FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_MAX_WIDTH_MASK,
        NULL,
        gdo_hndl.last_errno,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&buf,
        0, NULL);

    if (buf) {
        if (msg[0] != 0) {
            /* put custom message in front of system error message */
            GDO_SNPRINTF(gdo_hndl.buf_formatted, GDO_T("%s: %s"), msg, buf);
        } else {
            GDO_SNPRINTF(gdo_hndl.buf_formatted, GDO_T("%s"), buf);
        }

        LocalFree(buf);
    } else {
        /* FormatMessage() failed, save the error code */
        GDO_SNPRINTF(gdo_hndl.buf_formatted,
            GDO_T("Last saved error code: %lu"), gdo_hndl.last_errno);
    }

    return gdo_hndl.buf_formatted;

#else

    if (gdo_hndl.buf[0] == 0) {
        GDO_SNPRINTF(gdo_hndl.buf, GDO_T("%s"), "no error");
    }

    return gdo_hndl.buf;

#endif //GDO_WINAPI
}
/*****************************************************************************/



/*****************************************************************************/
/*                       get the full library path                           */
/*                                                                           */
/* Result must be deallocated with free(), returns NULL on error.            */
/*****************************************************************************/
GDO_LINKAGE gdo_char_t *gdo_lib_origin(void)
{
    _gdo_clear_error();

    /* check if library was loaded */
    if (!gdo_lib_is_loaded()) {
        _gdo_set_error_no_library_loaded();
        return NULL;
    }

#ifdef GDO_WINAPI

    gdo_char_t buf[32*1024];
    DWORD nSize = GetModuleFileName(gdo_hndl.handle, buf, _countof(buf));

    if (nSize == 0 || nSize == _countof(buf)) {
        _gdo_save_error(GDO_T("GetModuleFileName"));
        return NULL;
    }

    return _tcsdup(buf);

#elif defined(_WIN32)

    /* dlfcn-win32:
     * The handle returned by dlopen() is a `HMODULE' casted to `void *'.
     * We can directly use GetModuleFileNameA() to receive the DLL path
     * and don't need to invoke dladdr() on a loaded symbol address. */

    char buf[32*1024];
    DWORD nSize = GetModuleFileNameA((HMODULE)gdo_hndl.handle, buf, sizeof(buf));

    if (nSize == 0 || nSize == sizeof(buf)) {
        _gdo_save_to_errbuf("GetModuleFileNameA() failed to get library path");
        return NULL;
    }

    return _gdo_strdup(buf);

#elif defined(GDO_HAVE_DLINFO)

    /* use dlinfo() to get a link map */
    struct link_map *lm = NULL;

    if (dlinfo(gdo_hndl.handle, RTLD_DI_LINKMAP, &lm) == -1) {
        _gdo_save_error(NULL);
        return NULL;
    }

    return lm->l_name ? _gdo_strdup(lm->l_name) : NULL;

#else

    /* use dladdr() to get the library path from a symbol pointer */
    char *fname;

    if (gdo_no_symbols_loaded()) {
        _gdo_save_to_errbuf("no symbols were loaded");
        return NULL;
    }

    fname = _gdo_dladdr_get_fname((void *)gdo_hndl.ptr.%%symbol%%);@
    if (fname) return fname;

    _gdo_save_to_errbuf("dladdr() failed to get library path");

    return NULL;

#endif //GDO_WINAPI
}

#if !defined(GDO_WINAPI) && !defined(GDO_HAVE_DLINFO)
GDO_INLINE char *_gdo_dladdr_get_fname(const void *ptr)
{
    _GDO_Dl_info info;

    if (ptr && dladdr(ptr, &info) != 0 && info.dli_fname) {
        return _gdo_strdup(info.dli_fname);
    }

    return NULL;
}
#endif //!GDO_WINAPI && !GDO_HAVE_DLINFO
/*****************************************************************************/
%PARAM_SKIP_REMOVE_BEGIN%



/*****************************************************************************/
/*                                wrap code                                  */
/*****************************************************************************/

/* #define empty hooks by default */
#ifndef GDO_HOOK_%%func_symbol%%@
#define GDO_HOOK_%%func_symbol%%(...) /**/@
#endif


#if (defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)) && \
    defined(_WIN32) && defined(GDO_USE_MESSAGE_BOX)

GDO_INLINE void _gdo_show_MessageBox(const gdo_char_t *fmt, const gdo_char_t *sym, const gdo_char_t *msg)
{
    gdo_char_t buf[GDO_BUFLEN];
    GDO_SNPRINTF(buf, fmt, sym, msg);
    MessageBox(NULL, buf, GDO_T("Error"), MB_OK | MB_ICONERROR);
}

# define GDO_PRINT_ERROR(...)  _gdo_show_MessageBox(__VA_ARGS__)

#else

# define GDO_PRINT_ERROR(...)  _gdo_ftprintf(stderr, __VA_ARGS__)

#endif


#if defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)


GDO_INLINE void _gdo_wrap_check_if_loaded(bool sym_loaded, const gdo_char_t *sym)
{
    const gdo_char_t *msg;

    if (!gdo_lib_is_loaded()) {
        msg = GDO_T("library not loaded");
    } else if (!sym_loaded) {
        msg = GDO_T("symbol not loaded");
    } else {
        return; /* library and symbol loaded */
    }

    GDO_PRINT_ERROR(GDO_T("fatal error: %s: %s\n"), sym, msg);

    //gdo_force_free_lib();
    abort();
}


/* function wrappers (functions with `...' arguments are omitted) */

GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    const bool sym_loaded = (gdo_hndl.ptr.%%func_symbol%% != NULL);@
    _gdo_wrap_check_if_loaded(sym_loaded, GDO_T("%%func_symbol%%"));@
    GDO_HOOK_%%func_symbol%%(%%notype_args%%);@
    %%return%% gdo_hndl.ptr.%%func_symbol%%(%%notype_args%%);@
}@


#elif defined(GDO_ENABLE_AUTOLOAD)


/* This function is used by the autoload functions to perform the loading
 * and to handle errors. */
GDO_INLINE void _gdo_quick_load(int symbol_num, const gdo_char_t *sym)
{
    const gdo_char_t *fmt, *msg;

    /* set auto-release, ignore errors */
    gdo_enable_autorelease();

    /* load library */
    if (!gdo_lib_is_loaded()) {
        gdo_load_lib();
    }

#ifdef GDO_ENABLE_AUTOLOAD_LAZY
    /* load a specific symbol */
    if (gdo_load_symbol(symbol_num)) {
        return;
    }
#else
    /* load all symbols */
    (GDO_UNUSED_REF) symbol_num;

    if (gdo_load_all_symbols()) {
        return;
    }
#endif

    /* error */
    msg = gdo_last_error();

    if (_gdo_tcsstr(msg, GDO_DEFAULT_LIB)) {
        /* library name is already part of error message */
        fmt = GDO_T("error: ")
            GDO_T("%s: ")   /* sym */
            GDO_T("%s\n");  /* msg */
    } else {
        fmt = GDO_T("error: ")
            GDO_DEFAULT_LIB GDO_T(": ")
            GDO_T("%s: ")   /* sym */
            GDO_T("%s\n");  /* msg */
    }

    GDO_PRINT_ERROR(fmt, sym, msg);

    gdo_force_free_lib();
    exit(1);
}


/* autoload function wrappers (functions with `...' arguments are omitted) */

GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    _gdo_quick_load(GDO_LOAD_%%func_symbol%%, GDO_T("%%func_symbol%%"));@
    GDO_HOOK_%%func_symbol%%(%%notype_args%%);@
    %%return%% gdo_hndl.ptr.%%func_symbol%%(%%notype_args%%);@
}@

#endif //GDO_ENABLE_AUTOLOAD
/***************************** end of wrap code ******************************/
%PARAM_SKIP_END%


#if !defined(GDO_SEPARATE) /* single header file */

/* aliases to raw function pointers */
#if !defined(GDO_DISABLE_ALIASING) && !defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)
#define %%func_symbol_pad%% GDO_ALIAS_%%func_symbol%%
#endif

/* aliases to raw object pointers */
#if !defined(GDO_DISABLE_ALIASING)
#define %%obj_symbol_pad%% GDO_ALIAS_%%obj_symbol%%
#endif

#endif //!GDO_SEPARATE


/* keep these functions "private" */
#ifdef __GNUC__
#pragma GCC poison \
    _gdo_clear_error \
    _gdo_dladdr_get_fname \
    _gdo_load_library \
    _gdo_show_MessageBox \
    _gdo_quick_load \
    _gdo_save_error \
    _gdo_save_to_errbuf \
    _gdo_set_error_no_library_loaded \
    _gdo_sym \
    _gdo_wrap_check_if_loaded
#endif //__GNUC__

#undef GDO_SET_LAST_ERRNO
#undef GDO_SNPRINTF

