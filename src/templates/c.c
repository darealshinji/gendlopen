#if defined _MSC_VER && defined(GDO_USE_MESSAGE_BOX)
# pragma comment(lib, "user32.lib")
#endif

#ifdef GDO_WINAPI
# include <tchar.h>
#endif
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _T
# define _T(x) x
#endif

#ifdef GDO_WINAPI
# ifdef _UNICODE
#  define GDO_XS L"%ls"
# else
#  define GDO_XS "%s"
# endif
#endif


/* typedefs */
typedef void GDO_UNUSED_REF;
typedef void GDO_UNUSED_RESULT;


/* library handle */
GDO_LINKAGE gdo_handle_t gdo_hndl = {0};


/* forward declarations */
GDO_LINKAGE void gdo_load_library(const gdo_char_t *filename, int flags, bool new_namespace);
GDO_LINKAGE void gdo_register_free_lib(void);
GDO_LINKAGE void *gdo_sym(const char *symbol, const gdo_char_t *msg);
#if !defined(GDO_WINAPI) && !defined(GDO_HAVE_DLINFO)
GDO_LINKAGE char *gdo_dladdr_get_fname(const void *ptr);
#endif



/*****************************************************************************/
/*                                save error                                 */
/*****************************************************************************/
#ifdef GDO_WINAPI

GDO_LINKAGE void gdo_save_to_errbuf(const gdo_char_t *msg)
{
    if (msg) {
        _sntprintf_s(gdo_hndl.buf, sizeof(gdo_hndl.buf)-1, _TRUNCATE, GDO_XS, msg);
    }
}

/* Clear error buffers. */
GDO_LINKAGE void gdo_clear_errbuf(void)
{
    gdo_hndl.buf[0] = 0;
    gdo_hndl.buf_formatted[0] = 0;
    gdo_hndl.last_errno = 0;
}

/* Save the last system error code. A message for additional information
 * can be provided too. */
GDO_LINKAGE void gdo_save_GetLastError(const gdo_char_t *msg)
{
    gdo_clear_errbuf();
    gdo_hndl.last_errno = GetLastError();
    gdo_save_to_errbuf(msg);
}

/* Sets the "no library was loaded" error message */
GDO_LINKAGE void gdo_set_error_no_library_loaded(void)
{
    gdo_clear_errbuf();
    gdo_hndl.last_errno = ERROR_INVALID_HANDLE;
    gdo_save_to_errbuf(_T("no library was loaded"));
}

#else
/*********************************** dlfcn ***********************************/

GDO_LINKAGE void gdo_save_to_errbuf(const gdo_char_t *msg)
{
    if (msg) {
        snprintf(gdo_hndl.buf, sizeof(gdo_hndl.buf)-1, "%s", msg);
    }
}

/* Clear error buffers. */
GDO_LINKAGE void gdo_clear_errbuf(void)
{
    gdo_hndl.buf[0] = 0;
}

/* Save the last message provided by dlerror() */
GDO_LINKAGE void gdo_save_dlerror(void)
{
    gdo_clear_errbuf();
    gdo_save_to_errbuf(dlerror());
}

/* Sets the "no library was loaded" error message */
GDO_LINKAGE void gdo_set_error_no_library_loaded(void)
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
        gdo_load_symbols(false));
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
        gdo_clear_errbuf();
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
        gdo_clear_errbuf();
        const char *ptr = (errsav == ENOEXEC) ? dlerror() : strerror(errsav);
        gdo_save_to_errbuf(ptr);
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
GDO_LINKAGE void gdo_load_library(const gdo_char_t *filename, int flags, bool new_namespace)
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
}

/* If registered with atexit() this function will be called at
 * the program's exit. Function must be of type "void (*)(void)". */
GDO_LINKAGE void gdo_call_free_lib(void)
{
    if (gdo_lib_is_loaded()) {
#ifdef GDO_WINAPI
        FreeLibrary(gdo_hndl.handle);
#else
        dlclose(gdo_hndl.handle);
#endif
    }
}

/* register our call to free the library handle with atexit()
 * so that the library will automatically be freed upon exit */
GDO_LINKAGE void gdo_register_free_lib(void)
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
/*          Free the library handle and set pointers to NULL                 */
/*****************************************************************************/
GDO_LINKAGE bool gdo_free_lib(void)
{
    gdo_clear_errbuf();

    if (!gdo_lib_is_loaded()) {
        /* nothing to free */
        return true;
    }

#ifdef GDO_WINAPI
    if (FreeLibrary(gdo_hndl.handle) == FALSE) {
        gdo_save_GetLastError(_T("FreeLibrary()"));
        return false;
    }
#else
    if (dlclose(gdo_hndl.handle) != 0) {
        gdo_save_dlerror();
        return false;
    }
#endif

    /* set pointers back to NULL */
    gdo_hndl.handle = NULL;
    gdo_hndl.%%symbol%%_ptr_ = NULL;

    return true;
}
/*****************************************************************************/



/*****************************************************************************/
/*                    check if ALL symbols were loaded                       */
/*****************************************************************************/
GDO_LINKAGE bool gdo_all_symbols_loaded(void)
{
    if (true
        && gdo_hndl.%%symbol%%_ptr_ != NULL
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
        && gdo_hndl.%%symbol%%_ptr_ == NULL
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
        || gdo_hndl.%%symbol%%_ptr_ != NULL
    ) {
        return true;
    }

    return false;
}
/*****************************************************************************/



/*****************************************************************************/
/*          load all symbols; can safely be called multiple times            */
/*****************************************************************************/
GDO_LINKAGE bool gdo_load_symbols(bool ignore_errors)
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

    /* We can ignore errors in which case dlsym() or GetProcAddress()
     * is called for each symbol and continue to do so even if it fails.
     * The function will however in the end still return false if 1 or more
     * symbols failed to load.
     * If we do not ignore errors the function will simply return false on
     * the first error it encounters. */

    /* get symbol addresses */
@
    /* %%symbol%% */@
    gdo_hndl.%%symbol%%_ptr_ = @
        (%%sym_type%%)@
            gdo_sym("%%symbol%%", _T("%%symbol%%"));@
    if (!gdo_hndl.%%symbol%%_ptr_ && !ignore_errors) {@
        return false;@
    }

    gdo_clear_errbuf();

    return gdo_all_symbols_loaded();
}

GDO_LINKAGE void *gdo_sym(const char *symbol, const gdo_char_t *msg)
{
    gdo_clear_errbuf();

#ifdef GDO_WINAPI

    /* cast to void* to supress compiler warnings */
    void *ptr = (void *)GetProcAddress(gdo_hndl.handle, symbol);

    if (!ptr) {
        gdo_save_GetLastError(msg);
    }

#else

    (GDO_UNUSED_REF) msg;

    void *ptr = dlsym(gdo_hndl.handle, symbol);

    /**
     * Linux man page mentions cases where NULL pointer is a valid address.
     * These however seem to be edge-cases that are irrelevant to us.
     * Furthermore this is contradicting POSIX which says a NULL pointer shall
     * be returned on an error.
     */
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
/*****************************************************************************/
GDO_LINKAGE bool gdo_load_symbol(const char *symbol)
{
    gdo_clear_errbuf();

    /* no library was loaded */
    if (!gdo_lib_is_loaded()) {
        gdo_set_error_no_library_loaded();
        return false;
    }

    if (!symbol) {
        gdo_save_to_errbuf(_T("symbol == NULL"));
        return false;
    }

    if (symbol[0] == 0) {
        gdo_save_to_errbuf(_T("'symbol' is empty"));
        return false;
    }

    /* get symbol address */
@
    /* %%symbol%% */@
    if (strcmp("%%symbol%%", symbol) == 0) {@
        gdo_hndl.%%symbol%%_ptr_ =@
            (%%sym_type%%)@
                gdo_sym("%%symbol%%", _T("%%symbol%%"));@
        return (gdo_hndl.%%symbol%%_ptr_ != NULL);@
    }

#ifdef GDO_WINAPI
    gdo_hndl.last_errno = ERROR_NOT_FOUND;
    _sntprintf_s(gdo_hndl.buf, sizeof(gdo_hndl.buf)-1, _TRUNCATE,
        _T("symbol not among lookup list: %hs"), symbol);
#else
    snprintf(gdo_hndl.buf, sizeof(gdo_hndl.buf)-1,
        "symbol not among lookup list: %s", symbol);
#endif

    return false;
}
/*****************************************************************************/



/*****************************************************************************/
/* retrieve the last saved error message (can be an empty buffer)            */
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

    const size_t bufmax = _countof(gdo_hndl.buf_formatted) - 1;
    gdo_char_t *buf = NULL;
    gdo_char_t *msg = gdo_hndl.buf;
    gdo_char_t *out = gdo_hndl.buf_formatted;

    /* format the message */
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_MAX_WIDTH_MASK,
                NULL, gdo_hndl.last_errno, 0, (LPTSTR)&buf, 0, NULL);

    if (buf) {
        /* put custom message in front of system error message */
        if (msg[0] != 0 && (_tcslen(buf) + _tcslen(msg) + 3) < bufmax) {
            _sntprintf_s(out, bufmax, _TRUNCATE, GDO_XS _T(": ") GDO_XS, msg, buf);
        } else {
            _sntprintf_s(out, bufmax, _TRUNCATE, GDO_XS, buf);
        }
        LocalFree(buf);
    } else {
        /* FormatMessage() failed, just print the error code */
        _sntprintf_s(out, bufmax, _TRUNCATE, _T("Last saved error code: %lu"),
            gdo_hndl.last_errno);
    }

    return out;
#else
    /* simply return the buffer */
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

    /* https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
     * technically the path could exceed 260 characters, but in reality
     * it's practically still stuck at the old MAX_PATH value */
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
    //%DNL% //fprintf(stderr, "DEBUG: using dlinfo()\n");

    if (dlinfo(gdo_hndl.handle, RTLD_DI_LINKMAP, &lm) == -1) {
        gdo_save_dlerror();
        return NULL;
    }

    return lm->l_name ? strdup(lm->l_name) : NULL;
#else
    /* use dladdr() to get the library path from a symbol pointer */
    char *fname;
    //%DNL% //fprintf(stderr, "DEBUG: using dladdr()\n");

    if (gdo_no_symbols_loaded()) {
        gdo_save_to_errbuf("no symbols were loaded");
        return NULL;
    }

    fname = gdo_dladdr_get_fname((void *)gdo_hndl.%%symbol%%_ptr_);@
    if (fname) return fname;

    gdo_save_to_errbuf("dladdr() failed to get library path");

    return NULL;
#endif //GDO_WINAPI
}

#if !defined(GDO_WINAPI) && !defined(GDO_HAVE_DLINFO)
GDO_LINKAGE char *gdo_dladdr_get_fname(const void *ptr)
{
    Dl_info info;

    if (ptr && dladdr(ptr, &info) != 0 && info.dli_fname) {
        return strdup(info.dli_fname);
    }

    return NULL;
}
#endif // !GDO_WINAPI && !GDO_HAVE_DLINFO
/*****************************************************************************/



/*****************************************************************************/
/*                                wrap code                                  */
/*****************************************************************************/
%SKIP_PARAM_UNUSED_BEGIN%
//%DNL%//  comment out this whole section if "--skip-param" was set
#if defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)


GDO_LINKAGE void gdo_error_exit(const gdo_char_t *msg)
{
#if defined(GDO_OS_WIN32) && defined(GDO_USE_MESSAGE_BOX)
    MessageBox(NULL, msg, _T("Error"), MB_OK | MB_ICONERROR);
#elif defined(GDO_OS_WIN32) && defined(_UNICODE)
    fwprintf(stderr, L"%ls\n", msg);
#else
    fprintf(stderr, "%s\n", msg);
#endif

    gdo_free_lib();
    exit(1);
}


/* function wrappers */
@
GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    if (!gdo_hndl.%%func_symbol%%_ptr_) {@
        gdo_error_exit("error: symbol `%%func_symbol%%' was not loaded");@
    }@
    %%return%% gdo_hndl.%%func_symbol%%_ptr_(%%notype_args%%);@
}


#elif defined(GDO_ENABLE_AUTOLOAD)


#if defined(GDO_OS_WIN32) && defined(GDO_USE_MESSAGE_BOX)
/* Windows: show message in a MessageBox window */
GDO_LINKAGE void gdo_win32_last_error_messagebox(const gdo_char_t *symbol)
{
    const gdo_char_t *fmt = _T("error in wrapper function for symbol")
        _T("`") GDO_XS _T("':\n\n") GDO_XS;

    const gdo_char_t *err = gdo_last_error();

    const size_t len = _tcslen(fmt) + _tcslen(symbol) + _tcslen(err) + 1;
    gdo_char_t *buf = (gdo_char_t *)malloc(len * sizeof(gdo_char_t));
    assert(buf != NULL);

    _sntprintf_s(buf, len - 1, _TRUNCATE, fmt, symbol, err);
    MessageBox(NULL, buf, _T("Error"), MB_OK | MB_ICONERROR);

    free(buf);
}
#endif //GDO_OS_WIN32 && GDO_USE_MESSAGE_BOX


/* This function is used by the wrapper functions to perform the loading
 * and handle errors. */
GDO_LINKAGE void gdo_quick_load(const char *function, const gdo_char_t *symbol)
{
#ifdef GDO_DELAYLOAD
    /* load library + requested symbol */
    if (gdo_load_lib() && gdo_load_symbol(function)) {
        return;
    }
#else
    (GDO_UNUSED_REF) function;

    /* return immediately if everything is already loaded,
     * otherwise load library + all symbols */
    if (gdo_all_symbols_loaded() || gdo_load_lib_and_symbols()) {
        return;
    }
#endif

    /* an error has occured: display an error message */

#if defined(GDO_OS_WIN32) && defined(GDO_USE_MESSAGE_BOX)
    /* Windows: popup message box window */
    gdo_win32_last_error_messagebox(symbol);
#elif defined(GDO_OS_WIN32) && defined(_UNICODE)
    /* Windows: output to console (wide characters) */
    fwprintf(stderr, L"error in wrapper function for symbol `%ls':\n%ls\n",
        symbol, gdo_last_error());
#else
    /* default: UTF-8 output to console (any operating system) */
    fprintf(stderr, "error in wrapper function for symbol `%s':\n%s\n",
        symbol, gdo_last_error());
#endif //GDO_OS_WIN32 && GDO_USE_MESSAGE_BOX

    /* free library handle and exit */
    gdo_free_lib();
    exit(1);
}


/* autoload function wrappers */
@
GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    gdo_quick_load("%%func_symbol%%", _T("%%func_symbol%%"));@
    %%return%% gdo_hndl.%%func_symbol%%_ptr_(%%notype_args%%);@
}

#endif //GDO_ENABLE_AUTOLOAD
%SKIP_PARAM_UNUSED_END%
/***************************** end of wrap code ******************************/
