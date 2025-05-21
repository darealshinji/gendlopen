/*****************************************************************************/
/*                           C API implementation                            */
/*****************************************************************************/

#ifdef GDO_USE_MESSAGE_BOX
# if !defined(_WIN32)
#  undef GDO_USE_MESSAGE_BOX
# elif defined(_MSC_VER)
#  pragma comment(lib, "user32.lib")
# endif
#endif

#ifdef _WIN32
# include <assert.h>
#endif
#ifdef _AIX
# include <errno.h>
#endif
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef GDO_WINAPI
# include <tchar.h>
#endif
#ifndef _T
# define _T(x) x
#endif
#ifndef _ftprintf
# define _ftprintf fprintf
#endif
#ifndef _vsntprintf_s
# define _vsntprintf_s _vsnprintf_s
#endif
#ifndef _tcsstr
# define _tcsstr strstr
#endif

#ifndef _countof
# define _countof(array) (sizeof(array) / sizeof(array[0]))
#endif

#ifdef _GDO_TARGET_WIDECHAR
# define GDO_XS   L"%ls"
# define GDO_XHS  L"%hs"
#else
# define GDO_XS   "%s"
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


/* typedefs */
typedef void GDO_UNUSED_REF;
typedef void GDO_UNUSED_RESULT;


/* library handle */
GDO_OBJ_LINKAGE gdo_handle_t gdo_hndl;


/* forward declarations */
GDO_INLINE void _gdo_load_library(const gdo_char_t *filename, int flags, bool new_namespace);

GDO_INLINE void *_gdo_sym(const char *symbol, const gdo_char_t *msg)
    GDO_ATTR (nonnull);

#if !defined(GDO_WINAPI) && !defined(GDO_HAVE_DLINFO)
GDO_INLINE char *_gdo_dladdr_get_fname(const void *ptr)
    GDO_ATTR (warn_unused_result);
#endif


/* GDO_SNPRINTF */
#define GDO_SNPRINTF(dst, fmt, ...) \
    _gdo_snprintf(dst, _countof(dst), fmt, __VA_ARGS__)

GDO_INLINE void _gdo_snprintf(gdo_char_t *str, size_t size, const gdo_char_t *fmt, ...)
#if !defined(_GDO_TARGET_WIDECHAR)
    GDO_ATTR (format (printf, 3, 4))
#endif
    GDO_ATTR (nonnull (1, 3));


/* GDO_STRLCPY */
#define GDO_STRLCPY(dst, src) \
    _gdo_strlcpy(dst, src, _countof(dst))

GDO_INLINE void _gdo_strlcpy(gdo_char_t *dst, const gdo_char_t *src, size_t size)
    GDO_ATTR (nonnull);


/* GDO_STRDUP */
#ifdef _MSC_VER
/* MSVC warns if we don't use the one with the underscore */
# define GDO_STRDUP(x) _strdup(x)
#else
# define GDO_STRDUP(x) strdup(x)
#endif


/*****************************************************************************/
/*                                save error                                 */
/*****************************************************************************/

/* like snprintf(3), no return value */
GDO_INLINE void _gdo_snprintf(gdo_char_t *str, size_t size, const gdo_char_t *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);

#ifdef _WIN32
    _vsntprintf_s(str, size, _TRUNCATE, fmt, ap);
#else
    vsnprintf(str, size, fmt, ap);
#endif

    va_end(ap);
}

/* like strlcpy(3), no return value */
GDO_INLINE void _gdo_strlcpy(gdo_char_t *dst, const gdo_char_t *src, size_t size)
{
    if (size == 0) {
        return;
    }

    gdo_char_t *end = dst + size - 1;

    for ( ; dst != end; dst++, src++) {
        *dst = *src;

        if (*src == 0) {
            return;
        }
    }

    *end = 0;
}

/* save message to error buffer */
GDO_INLINE void _gdo_save_to_errbuf(const gdo_char_t *msg)
{
    if (msg) {
        GDO_STRLCPY(gdo_hndl.buf, msg);
    }
}

#ifdef GDO_WINAPI

/* Clear error buffers. */
GDO_INLINE void _gdo_clear_errbuf(void)
{
    gdo_hndl.buf[0] = 0;
    gdo_hndl.buf_formatted[0] = 0;
    gdo_hndl.last_errno = 0;
}

/* Save the last system error code. A message for additional information
 * can be provided too. */
GDO_INLINE void _gdo_save_GetLastError(const gdo_char_t *msg)
{
    _gdo_clear_errbuf();
    gdo_hndl.last_errno = GetLastError();
    _gdo_save_to_errbuf(msg);
}

/* Sets the "no library was loaded" error message */
GDO_INLINE void _gdo_set_error_no_library_loaded(void)
{
    _gdo_clear_errbuf();
    gdo_hndl.last_errno = ERROR_INVALID_HANDLE;
    _gdo_save_to_errbuf(_T("no library was loaded"));
}

#else
/*********************************** dlfcn ***********************************/

/* Clear error buffers. */
GDO_INLINE void _gdo_clear_errbuf(void)
{
    gdo_hndl.buf[0] = 0;
}

/* Save the last message provided by dlerror() */
GDO_INLINE void _gdo_save_dlerror(void)
{
    _gdo_clear_errbuf();
    _gdo_save_to_errbuf(dlerror());
}

/* Sets the "no library was loaded" error message */
GDO_INLINE void _gdo_set_error_no_library_loaded(void)
{
    _gdo_clear_errbuf();
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
    return gdo_load_lib_args(GDO_DEFAULT_LIB, GDO_DEFAULT_FLAGS, false);
}
#endif
/*****************************************************************************/



/*****************************************************************************/
/*       load default library with default flags and load the symbols        */
/*****************************************************************************/
#ifdef GDO_DEFAULT_LIB
GDO_LINKAGE bool gdo_load_lib_and_symbols(void)
{
    return (gdo_load_lib_args(GDO_DEFAULT_LIB, GDO_DEFAULT_FLAGS, false) &&
        gdo_load_all_symbols());
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
    _gdo_clear_errbuf();

    /* check if the library was already loaded */
    if (gdo_lib_is_loaded()) {
        return true;
    }

#ifdef GDO_WINAPI
    /* empty filename */
    if (!filename || *filename == 0) {
        gdo_hndl.last_errno = ERROR_INVALID_NAME;
        _gdo_save_to_errbuf(_T("empty filename"));
        return false;
    }

    _gdo_load_library(filename, flags, new_namespace);

    if (!gdo_lib_is_loaded()) {
        _gdo_save_GetLastError(filename);
        return false;
    }

#else /* dlfcn */

    /* an empty filename will actually return a handle to the main program,
     * but we don't want that */
    if (!filename || *filename == 0) {
        _gdo_clear_errbuf();
        _gdo_save_to_errbuf("empty filename");
        return false;
    }

#ifdef _AIX
    errno = 0;
    _gdo_load_library(filename, flags, new_namespace);
    int errsav = errno;

    if (!gdo_lib_is_loaded()) {
        const char *ptr = (errsav == ENOEXEC) ? dlerror() : strerror(errsav);
        _gdo_save_to_errbuf(ptr);
        return false;
    }
#else
    _gdo_load_library(filename, flags, new_namespace);

    if (!gdo_lib_is_loaded()) {
        _gdo_save_dlerror();
        return false;
    }
#endif //!_AIX

#endif //!GDO_WINAPI

    return true;
}

/* call LoadLibraryEx/dlopen/dlmopen */
GDO_INLINE void _gdo_load_library(const gdo_char_t *filename, int flags, bool new_namespace)
{
#ifdef GDO_WINAPI

    (GDO_UNUSED_REF) new_namespace;
    gdo_hndl.handle = LoadLibraryEx(filename, NULL, flags);

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

    gdo_hndl.flags = flags;
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
/*                 return the flags used to load the library                 */
/*****************************************************************************/
GDO_LINKAGE int gdo_lib_flags(void)
{
    return gdo_hndl.flags;
}
/*****************************************************************************/



/*****************************************************************************/
/*             free the library handle and set pointers to NULL              */
/*****************************************************************************/
GDO_LINKAGE bool gdo_free_lib(void)
{
    bool rv = true;

    _gdo_clear_errbuf();

    if (gdo_lib_is_loaded()) {
#ifdef GDO_WINAPI
        if (FreeLibrary(gdo_hndl.handle) == FALSE) {
            _gdo_save_GetLastError(_T("FreeLibrary()"));
            rv = false;
        }
#else
        if (dlclose(gdo_hndl.handle) != 0) {
            _gdo_save_dlerror();
            rv = false;
        }
#endif
    }

    /* set pointers back to NULL */
    gdo_hndl.handle = NULL;
    gdo_hndl.ptr.%%symbol%% = NULL;

    return rv;
}
/*****************************************************************************/



/*****************************************************************************/
/*                   automatically free library upon exit                    */
/*****************************************************************************/
GDO_LINKAGE bool gdo_enable_autorelease(void)
{
    _gdo_clear_errbuf();

    if (!gdo_hndl.free_lib_registered) {
        if (atexit((void (*)(void))gdo_free_lib) == 0) {
            gdo_hndl.free_lib_registered = true;
        } else {
            _gdo_save_to_errbuf(_T("atexit(): failed to register `gdo_free_lib()'"));
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
    _gdo_clear_errbuf();

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
                _gdo_sym("%%symbol%%", _T("%%symbol%%"))) == NULL) {@
        return false;@
    }@

    _gdo_clear_errbuf();

    return gdo_all_symbols_loaded();
}

GDO_INLINE void *_gdo_sym(const char *symbol, const gdo_char_t *msg)
{
    _gdo_clear_errbuf();

#ifdef GDO_WINAPI

    void *ptr = (void *)GetProcAddress(gdo_hndl.handle, symbol);

    if (!ptr) {
        _gdo_save_GetLastError(msg);
    }

#else

    (GDO_UNUSED_REF) msg;

    void *ptr = dlsym(gdo_hndl.handle, symbol);

    if (!ptr) {
        _gdo_save_dlerror();
    }

#endif //!GDO_WINAPI

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
    _gdo_clear_errbuf();

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
                    _gdo_sym("%%symbol%%", _T("%%symbol%%"));@
        }@
        return (gdo_hndl.ptr.%%symbol%% != NULL);@

    default:
        break;
    }

#ifdef GDO_WINAPI
    gdo_hndl.last_errno = ERROR_NOT_FOUND;
#endif

    GDO_SNPRINTF(gdo_hndl.buf, _T("unknown symbol number: %d"), symbol_num);

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
    _gdo_clear_errbuf();

    /* no library was loaded */
    if (!gdo_lib_is_loaded()) {
        _gdo_set_error_no_library_loaded();
        return false;
    }

    if (!symbol || *symbol == 0) {
#ifdef GDO_WINAPI
        gdo_hndl.last_errno = ERROR_INVALID_PARAMETER;
#endif
        _gdo_save_to_errbuf(_T("empty symbol name"));
    } else {
        /* jumps to `GDO_JUMP_<..>' label if symbol was found */
        GDO_CHECK_SYMBOL_NAME(symbol);

#ifdef GDO_WINAPI
        gdo_hndl.last_errno = ERROR_NOT_FOUND;
#endif
        GDO_SNPRINTF(gdo_hndl.buf, _T("unknown symbol: %s"), symbol);
    }

    return false;

    /* jump labels */
@
    /* %%symbol%% */@
GDO_JUMP_%%symbol%%:@
    if (!gdo_hndl.ptr.%%symbol%%) {@
        gdo_hndl.ptr.%%symbol%% =@
            (%%sym_type%%)@
                _gdo_sym("%%symbol%%", _T("%%symbol%%"));@
    }@
    return (gdo_hndl.ptr.%%symbol%% != NULL);
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

    const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_IGNORE_INSERTS |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK;

    /* format the message */
    FormatMessage(flags, NULL, gdo_hndl.last_errno, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&buf, 0, NULL);

    if (buf) {
        /* put custom message in front of system error message */
        if (msg[0] != 0 && (_tcslen(buf) + _tcslen(msg) + 2) < _countof(gdo_hndl.buf_formatted)) {
            GDO_SNPRINTF(gdo_hndl.buf_formatted, GDO_XS _T(": ") GDO_XS, msg, buf);
        } else {
            GDO_STRLCPY(gdo_hndl.buf_formatted, buf);
        }
        LocalFree(buf);
    } else {
        /* FormatMessage() failed, save the error code */
        GDO_SNPRINTF(gdo_hndl.buf_formatted, _T("Last saved error code: %lu"), gdo_hndl.last_errno);
    }

    return gdo_hndl.buf_formatted;

#else

    if (gdo_hndl.buf[0] == 0) {
        GDO_STRLCPY(gdo_hndl.buf, "no error");
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
    _gdo_clear_errbuf();

    /* check if library was loaded */
    if (!gdo_lib_is_loaded()) {
        _gdo_set_error_no_library_loaded();
        return NULL;
    }

#ifdef GDO_WINAPI

    gdo_char_t buf[32*1024];
    DWORD nSize = GetModuleFileName(gdo_hndl.handle, buf, _countof(buf));

    if (nSize == 0 || nSize == _countof(buf)) {
        _gdo_save_GetLastError(_T("GetModuleFileName"));
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

    return GDO_STRDUP(buf);

#elif defined(GDO_HAVE_DLINFO)

    /* use dlinfo() to get a link map */
    struct link_map *lm = NULL;

    if (dlinfo(gdo_hndl.handle, RTLD_DI_LINKMAP, &lm) == -1) {
        _gdo_save_dlerror();
        return NULL;
    }

    return lm->l_name ? GDO_STRDUP(lm->l_name) : NULL;

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
    Dl_info info;

    if (ptr && dladdr(ptr, &info) != 0 && info.dli_fname) {
        return GDO_STRDUP(info.dli_fname);
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


/* show error message and exit program */
#ifdef _MSC_VER
GDO_INLINE __declspec(noreturn)
    void _gdo_error_exit(const gdo_char_t *msg);
#else
GDO_INLINE void _gdo_error_exit(const gdo_char_t *msg)
    GDO_ATTR (nonnull)
    GDO_ATTR (noreturn);
#endif

GDO_INLINE void _gdo_error_exit(const gdo_char_t *msg)
{
#ifdef GDO_USE_MESSAGE_BOX
    MessageBox(NULL, msg, _T("Error"), MB_OK | MB_ICONERROR);
#else
    _ftprintf(stderr, GDO_XS _T("\n"), msg);
#endif

    gdo_free_lib();
    exit(1);
}


#if defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)


GDO_INLINE void _gdo_wrap_check_if_loaded(bool sym_loaded, const gdo_char_t *msg)
{
    gdo_char_t buf[256];

    if (!gdo_lib_is_loaded()) {
        GDO_SNPRINTF(buf, GDO_XS _T("library not loaded\n"), msg);
        _gdo_error_exit(buf);
    } else if (!sym_loaded) {
        GDO_SNPRINTF(buf, GDO_XS _T("symbol not loaded\n"), msg);
        _gdo_error_exit(buf);
    }
}


/* function wrappers (functions with `...' arguments are omitted) */

GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    const bool sym_loaded = (gdo_hndl.ptr.%%func_symbol%% != NULL);@
    _gdo_wrap_check_if_loaded(sym_loaded, "fatal error: %%func_symbol%%: ");@
    GDO_HOOK_%%func_symbol%%(%%notype_args%%);@
    %%return%% gdo_hndl.ptr.%%func_symbol%%(%%notype_args%%);@
}@


#elif defined(GDO_ENABLE_AUTOLOAD)


#ifdef _MSC_VER
GDO_INLINE __declspec(noreturn)
    void _gdo_quick_load_error_exit(const gdo_char_t *msg);
#else
GDO_INLINE void _gdo_quick_load_error_exit(const gdo_char_t *msg)
    GDO_ATTR (nonnull)
    GDO_ATTR (noreturn);
#endif

GDO_INLINE void _gdo_quick_load_error_exit(const gdo_char_t *msg)
{
#ifdef _WIN32
    gdo_char_t buf[64*1024];
#else
    gdo_char_t buf[8*1024];
#endif

    const gdo_char_t *perr = gdo_last_error();

    if (_tcsstr(perr, GDO_DEFAULT_LIB)) {
        /* library name is already part of error message */
        GDO_SNPRINTF(buf, GDO_XS GDO_XS _T("\n"), msg, perr);
    } else {
        GDO_SNPRINTF(buf, GDO_XS GDO_DEFAULT_LIB _T(": ") GDO_XS _T("\n"), msg, perr);
    }

    _gdo_error_exit(buf);
}


/* This function is used by the autoload functions to perform the loading
 * and to handle errors. */
GDO_INLINE void _gdo_quick_load(int symbol_num, const gdo_char_t *msg)
{
    /* set auto-release, ignore errors */
    gdo_enable_autorelease();

#ifdef GDO_DELAYLOAD
    /* load a specific symbol */
    if (!gdo_load_lib() || !gdo_load_symbol(symbol_num)) {
        _gdo_quick_load_error_exit(msg);
    }
#else
    (GDO_UNUSED_REF) symbol_num;

    /* load all symbols */
    if (!gdo_load_lib_and_symbols()) {
        _gdo_quick_load_error_exit(msg);
    }
#endif
}


/* autoload function wrappers (functions with `...' arguments are omitted) */

GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    _gdo_quick_load(GDO_LOAD_%%func_symbol%%, _T("error: %%func_symbol%%: "));@
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
    _gdo_clear_errbuf \
    _gdo_dladdr_get_fname \
    _gdo_error_exit \
    _gdo_load_library \
    _gdo_quick_load \
    _gdo_quick_load_error_exit \
    _gdo_save_GetLastError \
    _gdo_save_dlerror \
    _gdo_save_to_errbuf \
    _gdo_set_error_no_library_loaded \
    _gdo_snprintf \
    _gdo_strlcpy \
    _gdo_sym \
    _gdo_wrap_check_if_loaded
#endif

#undef GDO_SNPRINTF
#undef GDO_STRDUP
#undef GDO_STRLCPY

