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
GDO_INLINE void gdo_load_library(const gdo_char_t *filename, int flags, bool new_namespace);
GDO_INLINE void gdo_register_free_lib(void);

GDO_INLINE void *gdo_sym(const char *symbol, const gdo_char_t *msg)
    GDO_ATTR (nonnull);

#if !defined(GDO_WINAPI) && !defined(GDO_HAVE_DLINFO)
GDO_INLINE char *gdo_dladdr_get_fname(const void *ptr)
    GDO_ATTR (warn_unused_result);
#endif


/* GDO_SNPRINTF */
#define GDO_SNPRINTF(dst, fmt, ...) \
    gdo_snprintf(dst, _countof(dst), fmt, __VA_ARGS__)

GDO_INLINE void gdo_snprintf(gdo_char_t *str, size_t size, const gdo_char_t *fmt, ...)
#if !defined(_GDO_TARGET_WIDECHAR)
    GDO_ATTR (format (printf, 3, 4))
#endif
    GDO_ATTR (nonnull (1, 3));


/* GDO_STRLCPY */
#define GDO_STRLCPY(dst, src) \
    gdo_strlcpy(dst, src, _countof(dst))

GDO_INLINE void gdo_strlcpy(gdo_char_t *dst, const gdo_char_t *src, size_t size)
    GDO_ATTR (nonnull);


/* GDO_STRDUP */
#ifdef _MSC_VER
# define GDO_STRDUP(x) _strdup(x)
#else
# define GDO_STRDUP(x) strdup(x)
#endif


/*****************************************************************************/
/*                                save error                                 */
/*****************************************************************************/

/* like snprintf(3), no return value */
GDO_INLINE void gdo_snprintf(gdo_char_t *str, size_t size, const gdo_char_t *fmt, ...)
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
GDO_INLINE void gdo_strlcpy(gdo_char_t *dst, const gdo_char_t *src, size_t size)
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
GDO_INLINE void gdo_save_to_errbuf(const gdo_char_t *msg)
{
    if (msg) {
        GDO_STRLCPY(gdo_hndl.buf, msg);
    }
}

#ifdef GDO_WINAPI

/* Clear error buffers. */
GDO_INLINE void gdo_clear_errbuf(void)
{
    gdo_hndl.buf[0] = 0;
    gdo_hndl.buf_formatted[0] = 0;
    gdo_hndl.last_errno = 0;
}

/* Save the last system error code. A message for additional information
 * can be provided too. */
GDO_INLINE void gdo_save_GetLastError(const gdo_char_t *msg)
{
    gdo_clear_errbuf();
    gdo_hndl.last_errno = GetLastError();
    gdo_save_to_errbuf(msg);
}

/* Sets the "no library was loaded" error message */
GDO_INLINE void gdo_set_error_no_library_loaded(void)
{
    gdo_clear_errbuf();
    gdo_hndl.last_errno = ERROR_INVALID_HANDLE;
    gdo_save_to_errbuf(_T("no library was loaded"));
}

#else
/*********************************** dlfcn ***********************************/

/* Clear error buffers. */
GDO_INLINE void gdo_clear_errbuf(void)
{
    gdo_hndl.buf[0] = 0;
}

/* Save the last message provided by dlerror() */
GDO_INLINE void gdo_save_dlerror(void)
{
    gdo_clear_errbuf();
    gdo_save_to_errbuf(dlerror());
}

/* Sets the "no library was loaded" error message */
GDO_INLINE void gdo_set_error_no_library_loaded(void)
{
    gdo_clear_errbuf();
    gdo_save_to_errbuf("no library was loaded");
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
    gdo_clear_errbuf();

    /* check if the library was already loaded */
    if (gdo_lib_is_loaded()) {
        return true;
    }

#ifdef GDO_WINAPI
    /* empty filename */
    if (!filename || *filename == 0) {
        gdo_hndl.last_errno = ERROR_INVALID_NAME;
        gdo_save_to_errbuf(_T("empty filename"));
        return false;
    }

    gdo_load_library(filename, flags, new_namespace);

    if (!gdo_lib_is_loaded()) {
        gdo_save_GetLastError(filename);
        return false;
    }

#else /* dlfcn */

    /* an empty filename will actually return a handle to the main program,
     * but we don't want that */
    if (!filename || *filename == 0) {
        gdo_clear_errbuf();
        gdo_save_to_errbuf("empty filename");
        return false;
    }

#ifdef _AIX
    errno = 0;
    gdo_load_library(filename, flags, new_namespace);
    int errsav = errno;

    if (!gdo_lib_is_loaded()) {
        const char *ptr = (errsav == ENOEXEC) ? dlerror() : strerror(errsav);
        gdo_save_to_errbuf(ptr);
        return false;
    }
#else
    gdo_load_library(filename, flags, new_namespace);

    if (!gdo_lib_is_loaded()) {
        gdo_save_dlerror();
        return false;
    }
#endif //!_AIX

#endif //!GDO_WINAPI

    gdo_register_free_lib();

    return true;
}

/* call LoadLibraryEx/dlopen/dlmopen */
GDO_INLINE void gdo_load_library(const gdo_char_t *filename, int flags, bool new_namespace)
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

/* If registered with atexit() this function will be called at
 * the program's exit. Function must be of type "void (*)(void)". */
#ifdef GDO_AUTO_RELEASE
GDO_INLINE void gdo_call_free_lib(void)
{
    if (gdo_lib_is_loaded()) {
#ifdef GDO_WINAPI
        FreeLibrary(gdo_hndl.handle);
#else
        dlclose(gdo_hndl.handle);
#endif
    }
}
#endif //GDO_AUTO_RELEASE

/* register our call to free the library handle with atexit()
 * so that the library will automatically be freed upon exit */
GDO_INLINE void gdo_register_free_lib(void)
{
#ifdef GDO_AUTO_RELEASE
    if (!gdo_hndl.call_free_lib_is_registered) {
        atexit(gdo_call_free_lib);
        gdo_hndl.call_free_lib_is_registered = true;
    }
#endif
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

    gdo_clear_errbuf();

    if (gdo_lib_is_loaded()) {
#ifdef GDO_WINAPI
        if (FreeLibrary(gdo_hndl.handle) == FALSE) {
            gdo_save_GetLastError(_T("FreeLibrary()"));
            rv = false;
        }
#else
        if (dlclose(gdo_hndl.handle) != 0) {
            gdo_save_dlerror();
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
    gdo_clear_errbuf();

    /* already loaded all symbols */
    if (gdo_all_symbols_loaded()) {
        return true;
    }

    /* no library was loaded */
    if (!gdo_lib_is_loaded()) {
        gdo_set_error_no_library_loaded();
        return false;
    }

    /* get symbol addresses */

    /* %%symbol%% */@
    if ((gdo_hndl.ptr.%%symbol%% =@
            (%%sym_type%%)@
                gdo_sym("%%symbol%%", _T("%%symbol%%"))) == NULL) {@
        return false;@
    }@

    gdo_clear_errbuf();

    return gdo_all_symbols_loaded();
}

GDO_INLINE void *gdo_sym(const char *symbol, const gdo_char_t *msg)
{
    gdo_clear_errbuf();

#ifdef GDO_WINAPI

    void *ptr = (void *)GetProcAddress(gdo_hndl.handle, symbol);

    if (!ptr) {
        gdo_save_GetLastError(msg);
    }

#else

    (GDO_UNUSED_REF) msg;

    void *ptr = dlsym(gdo_hndl.handle, symbol);

    if (!ptr) {
        gdo_save_dlerror();
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
    gdo_clear_errbuf();

    /* no library was loaded */
    if (!gdo_lib_is_loaded()) {
        gdo_set_error_no_library_loaded();
        return false;
    }

    switch (symbol_num)
    {
    /* %%symbol%% */@
    case GDO_LOAD_%%symbol%%:@
        if (!gdo_hndl.ptr.%%symbol%%) {@
            gdo_hndl.ptr.%%symbol%% =@
                (%%sym_type%%)@
                    gdo_sym("%%symbol%%", _T("%%symbol%%"));@
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
    gdo_clear_errbuf();

    /* no library was loaded */
    if (!gdo_lib_is_loaded()) {
        gdo_set_error_no_library_loaded();
        return false;
    }

    if (!symbol || *symbol == 0) {
#ifdef GDO_WINAPI
        gdo_hndl.last_errno = ERROR_INVALID_PARAMETER;
#endif
        gdo_save_to_errbuf(_T("empty symbol name"));
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
                gdo_sym("%%symbol%%", _T("%%symbol%%"));@
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
    gdo_clear_errbuf();

    /* check if library was loaded */
    if (!gdo_lib_is_loaded()) {
        gdo_set_error_no_library_loaded();
        return NULL;
    }

#ifdef GDO_WINAPI

    gdo_char_t *origin;
    DWORD len = 260; /* MAX_PATH */

    /* allocate enough space */
    origin = (gdo_char_t *)malloc(len * sizeof(gdo_char_t));
    assert(origin != NULL);

    /* receive path from handle */
    if (GetModuleFileName(gdo_hndl.handle, origin, len-1) == 0) {
        gdo_save_GetLastError(_T("GetModuleFileName"));
        free(origin);
        return NULL;
    }

    /* https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation */
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        len = 32*1024;
        origin = (gdo_char_t *)realloc(origin, len * sizeof(gdo_char_t));
        assert(origin != NULL);

        if (GetModuleFileName(gdo_hndl.handle, origin, len-1) == 0) {
            gdo_save_GetLastError(_T("GetModuleFileName"));
            free(origin);
            return NULL;
        }
    }

    return origin;

#elif defined(GDO_HAVE_DLINFO)

    /* use dlinfo() to get a link map */
    struct link_map *lm = NULL;

    if (dlinfo(gdo_hndl.handle, RTLD_DI_LINKMAP, &lm) == -1) {
        gdo_save_dlerror();
        return NULL;
    }

    return lm->l_name ? GDO_STRDUP(lm->l_name) : NULL;

#else

    /* use dladdr() to get the library path from a symbol pointer */
    char *fname;

    if (gdo_no_symbols_loaded()) {
        gdo_save_to_errbuf("no symbols were loaded");
        return NULL;
    }

    fname = gdo_dladdr_get_fname((void *)gdo_hndl.ptr.%%symbol%%);@
    if (fname) return fname;

    gdo_save_to_errbuf("dladdr() failed to get library path");

    return NULL;

#endif //GDO_WINAPI
}

#if !defined(GDO_WINAPI) && !defined(GDO_HAVE_DLINFO)
GDO_INLINE char *gdo_dladdr_get_fname(const void *ptr)
{
    Dl_info info;

    if (ptr && dladdr(ptr, &info) != 0 && info.dli_fname) {
        return GDO_STRDUP(info.dli_fname);
    }

    return NULL;
}
#endif // !GDO_WINAPI && !GDO_HAVE_DLINFO
/*****************************************************************************/
%PARAM_SKIP_REMOVE_BEGIN%



/*****************************************************************************/
/*                                wrap code                                  */
/*****************************************************************************/

/* #define empty hooks by default */
#ifndef GDO_HOOK_%%func_symbol%%@
#define GDO_HOOK_%%func_symbol%%(...) /**/@
#endif

#if defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)


GDO_INLINE void gdo_wrap_check_if_loaded(bool func_loaded, const gdo_char_t *func_msg)
{
    const gdo_char_t *msg;

    if (!gdo_lib_is_loaded()) {
        msg = _T("error: library not loaded");
    } else if (!func_loaded) {
        msg = func_msg;
    } else {
        /* everything loaded */
        return;
    }

#ifdef GDO_USE_MESSAGE_BOX
    MessageBox(NULL, msg, _T("Error"), MB_OK | MB_ICONERROR);
#else
    _ftprintf(stderr, GDO_XS _T("\n"), msg);
#endif

    gdo_free_lib();
    exit(1);
}


/* function wrappers (functions with `...' arguments are omitted) */

GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    const bool func_loaded = (gdo_hndl.ptr.%%func_symbol%% != NULL);@
    gdo_wrap_check_if_loaded(func_loaded, _T("error: symbol `%%func_symbol%%' not loaded"));@
    GDO_HOOK_%%func_symbol%%(%%notype_args%%);@
    %%return%% gdo_hndl.ptr.%%func_symbol%%(%%notype_args%%);@
}@


#elif defined(GDO_ENABLE_AUTOLOAD)


GDO_INLINE void gdo_error_exit(const gdo_char_t *fmt, const gdo_char_t *name, const gdo_char_t *msg)
    GDO_ATTR (nonnull)
    GDO_ATTR (noreturn);

GDO_INLINE void gdo_error_exit(const gdo_char_t *fmt, const gdo_char_t *name, const gdo_char_t *msg)
{
#ifdef GDO_USE_MESSAGE_BOX
    /* Windows: show message in a MessageBox window */
    const size_t buflen = _tcslen(fmt) + _tcslen(name) + _tcslen(msg) + 1;
    gdo_char_t *buf = (gdo_char_t *)malloc(buflen * sizeof(gdo_char_t));
    assert(buf != NULL);

    gdo_snprintf(buf, buflen, fmt, name, msg);
    MessageBox(NULL, buf, _T("Error"), MB_OK | MB_ICONERROR);

    free(buf);
#else
    _ftprintf(stderr, fmt, name, msg);
#endif

    gdo_free_lib();
    exit(1);
}


/* This function is used by the autoload functions to perform the loading
 * and to handle errors. */
GDO_INLINE void gdo_quick_load(int symbol_num, const gdo_char_t *symbol)
{
    const gdo_char_t *fmt, *msg;

    /* load library */
    if (!gdo_load_lib()) {
        msg = gdo_last_error();

        if (_tcsstr(msg, GDO_DEFAULT_LIB)) {
            /* library name is already part of error message */
            fmt = _T("error loading library:\n")
                GDO_XS            /* empty */
                GDO_XS _T("\n");  /* msg */
        } else {
            fmt = _T("error loading library:\n")
                GDO_DEFAULT_LIB _T(":\n")
                GDO_XS            /* empty */
                GDO_XS _T("\n");  /* msg */
        }

        gdo_error_exit(fmt, _T(""), msg);
    }

    /* load requested symbol or all symbols */
#ifdef GDO_DELAYLOAD
    if (gdo_load_symbol(symbol_num)) {
        return;
    }
#else
    (GDO_UNUSED_REF) symbol_num;

    if (gdo_load_all_symbols()) {
        return;
    }
#endif

    msg = gdo_last_error();

    if (_tcsstr(msg, GDO_DEFAULT_LIB)) {
        /* library name is already part of error message */
        fmt = _T("error in auto-loading wrapper function `") GDO_XS _T("':\n")
            GDO_XS _T("\n");
    } else {
        fmt = _T("error in auto-loading wrapper function `") GDO_XS _T("':\n")
            GDO_DEFAULT_LIB _T(":\n")
            GDO_XS _T("\n");
    }

    gdo_error_exit(fmt, symbol, msg);
}


/* autoload function wrappers (functions with `...' arguments are omitted) */

GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    gdo_quick_load(GDO_LOAD_%%func_symbol%%, _T("%%func_symbol%%"));@
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

