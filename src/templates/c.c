/*****************************************************************************/
/*                           C API implementation                            */
/*****************************************************************************/

#if defined(_MSC_VER) && defined(GDO_USE_MESSAGE_BOX)
# pragma comment(lib, "user32.lib")
#endif

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef GDO_WINAPI
# include <tchar.h>
# define _gdo_ftprintf     _ftprintf
# define _gdo_sntprintf    _sntprintf
# define _gdo_sntprintf_s  _sntprintf_s
# define _gdo_tcsstr       _tcsstr
# define GDO_T(x)          _T(x)
#else
/* dlfcn: use `char' API */
# define _gdo_ftprintf     fprintf
# define _gdo_sntprintf    snprintf
# define _gdo_sntprintf_s  _snprintf_s
# define _gdo_tcsstr       strstr
# define GDO_T(x)          x
#endif

#ifdef _MSC_VER
# define _gdo_strdup  _strdup
#else
# define _gdo_strdup  strdup
#endif

/* GDO_SNPRINTF macro */
#ifdef _MSC_VER
# define GDO_SNPRINTF(dst, fmt, ...) \
    _gdo_sntprintf_s(dst, _countof(dst), _TRUNCATE, fmt, __VA_ARGS__)
#else
# define GDO_SNPRINTF(dst, fmt, ...) \
    _gdo_sntprintf(dst, _countof(dst), fmt, __VA_ARGS__)
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


#ifndef _countof
# define _countof(array)  (sizeof(array) / sizeof(array[0]))
#endif


/* typedefs */
typedef void GDO_UNUSED_REF;
typedef void GDO_UNUSED_RESULT;


/* library handle */
typedef struct _gdo_handle
{
    /* symbol pointers */
    struct _gdo_ptr {
        %%type%% (*%%func_symbol%%)(%%args%%);
        %%obj_type%% *%%obj_symbol%%;
    } ptr;

    /* private */

#ifdef GDO_WINAPI
    HMODULE handle;
    DWORD last_errno;
    gdo_char_t buf[64*1024];
    gdo_char_t buf_formatted[64*1024]; /* Used by FormatMessage; MSDN says the maximum message length is 64k */
#else
    void *handle;
    gdo_char_t buf[8*1024];
#endif

    int flags;
    bool free_lib_registered;

} gdo_handle_t;

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

/* save message to error buffer */
GDO_INLINE void _gdo_save_to_errbuf(const gdo_char_t *msg)
{
    gdo_hndl.buf[0] = 0;

    if (msg) {
        GDO_SNPRINTF(gdo_hndl.buf, GDO_T("%s"), msg);
    }
}

#ifdef GDO_WINAPI

/* Clear error buffers. */
GDO_INLINE void _gdo_clear_error(void)
{
    gdo_hndl.buf[0] = 0;
    gdo_hndl.buf_formatted[0] = 0;
    gdo_hndl.last_errno = 0;
}

/* Save the last system error code. A message for additional information
 * can be provided too. */
GDO_INLINE void _gdo_save_GetLastError(const gdo_char_t *msg)
{
    _gdo_clear_error();
    gdo_hndl.last_errno = GetLastError();
    _gdo_save_to_errbuf(msg);
}

/* Sets the "no library was loaded" error message */
GDO_INLINE void _gdo_set_error_no_library_loaded(void)
{
    _gdo_clear_error();
    gdo_hndl.last_errno = ERROR_INVALID_HANDLE;
    _gdo_save_to_errbuf(GDO_T("no library was loaded"));
}

#else
/*********************************** dlfcn ***********************************/

/* Clear error buffers. */
GDO_INLINE void _gdo_clear_error(void)
{
    dlerror();
    gdo_hndl.buf[0] = 0;
}

/* Save the last message provided by dlerror() */
GDO_INLINE void _gdo_save_dlerror(void)
{
    _gdo_save_to_errbuf(dlerror());
}

/* Sets the "no library was loaded" error message */
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

#ifdef GDO_WINAPI
    /* empty filename */
    if (!filename || *filename == 0) {
        gdo_hndl.last_errno = ERROR_INVALID_NAME;
        _gdo_save_to_errbuf(GDO_T("empty filename"));
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
        _gdo_save_to_errbuf("empty filename");
        return false;
    }

    _gdo_load_library(filename, flags, new_namespace);

    if (!gdo_lib_is_loaded()) {
        _gdo_save_dlerror();
        return false;
    }

#endif //!GDO_WINAPI

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
    _gdo_clear_error();

    if (gdo_lib_is_loaded()) {
#ifdef GDO_WINAPI
        if (FreeLibrary(gdo_hndl.handle) == FALSE) {
            _gdo_save_GetLastError(GDO_T("FreeLibrary()"));
            return false;
        }
#else
        if (dlclose(gdo_hndl.handle) != 0) {
            _gdo_save_dlerror();
            return false;
        }
#endif
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
    _gdo_clear_error();

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

#ifdef GDO_WINAPI
    gdo_hndl.last_errno = ERROR_NOT_FOUND;
#endif

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
#ifdef GDO_WINAPI
        gdo_hndl.last_errno = ERROR_INVALID_PARAMETER;
#endif
        _gdo_save_to_errbuf(GDO_T("empty symbol name"));
    } else {
        /* jumps to `GDO_JUMP_<..>' label if symbol was found */
        GDO_CHECK_SYMBOL_NAME(symbol);

#ifdef GDO_WINAPI
        gdo_hndl.last_errno = ERROR_NOT_FOUND;
#endif
        GDO_SNPRINTF(gdo_hndl.buf, GDO_T("unknown symbol: ") GDO_XHS, symbol);
    }

    return false;

    /* jump labels */
@
    /* %%symbol%% */@
GDO_JUMP_%%symbol%%:@
    if (!gdo_hndl.ptr.%%symbol%%) {@
        gdo_hndl.ptr.%%symbol%% =@
            (%%sym_type%%)@
                _gdo_sym("%%symbol%%", GDO_T("%%symbol%%"));@
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
            GDO_SNPRINTF(gdo_hndl.buf_formatted, GDO_T("%s: %s"), msg, buf);
        } else {
            GDO_SNPRINTF(gdo_hndl.buf_formatted, GDO_T("%s"), buf);
        }
        LocalFree(buf);
    } else {
        /* FormatMessage() failed, save the error code */
        GDO_SNPRINTF(gdo_hndl.buf_formatted, GDO_T("Last saved error code: %lu"), gdo_hndl.last_errno);
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
        _gdo_save_GetLastError(GDO_T("GetModuleFileName"));
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
        _gdo_save_dlerror();
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

GDO_INLINE void _gdo_print_MessageBox(const gdo_char_t *fmt, ...)
{
    size_t len;
    gdo_char_t *msg;
    va_list ap;

    /* get message length in characters */
    len = _tcslen(fmt);
    va_start(ap, fmt);
    len += _tcslen(va_arg(ap, const gdo_char_t *));
    va_end(ap);

    /* save message */
    msg = (gdo_char_t *)malloc(len * sizeof(gdo_char_t)); /* gdo_char_t can be wide chars! */
    _vsntprintf_s(msg, len, _TRUNCATE, fmt, ap);

    /* show window */
    MessageBox(NULL, msg, GDO_T("Error"), MB_OK | MB_ICONERROR);
    free(msg);
}

#endif


#if defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)


GDO_INLINE void _gdo_wrap_check_if_loaded(bool sym_loaded, const gdo_char_t *sym)
{
    const bool lib_loaded = gdo_lib_is_loaded();

    if (lib_loaded && sym_loaded) {
        return;
    }

    /* error */

    const gdo_char_t *fmt = lib_loaded ?
          GDO_T("fatal error: %s: symbol not loaded\n")
        : GDO_T("fatal error: %s: library not loaded\n");

#if defined(_WIN32) && defined(GDO_USE_MESSAGE_BOX)
    _gdo_print_MessageBox(fmt, sym);
#else
    _gdo_ftprintf(stderr, fmt, sym);
#endif

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

    const gdo_char_t *fmt;
    const gdo_char_t *msg = gdo_last_error();

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

#if defined(_WIN32) && defined(GDO_USE_MESSAGE_BOX)
    _gdo_print_MessageBox(fmt, sym, msg);
#else
    _gdo_ftprintf(stderr, fmt, sym, msg);
#endif

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
    _gdo_print_MessageBox \
    _gdo_quick_load \
    _gdo_save_GetLastError \
    _gdo_save_dlerror \
    _gdo_save_to_errbuf \
    _gdo_set_error_no_library_loaded \
    _gdo_sym \
    _gdo_wrap_check_if_loaded
#endif //__GNUC__

