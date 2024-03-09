#if defined _MSC_VER && defined(GDO_USE_MESSAGE_BOX)
    #pragma comment(lib, "user32.lib")
#endif

#ifdef GDO_WINAPI
    #include <tchar.h>
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _T
    #define _T(x) x
#endif

typedef void GDO_UNUSED;
typedef void GDO_UNUSED_RESULT;


/* forward declarations */
GDO_LINKAGE void gdo_call_free_lib(void);
GDO_LINKAGE void gdo_register_free_lib(void);
GDO_LINKAGE void gdo_clear_errbuf(void);

#ifdef GDO_WINAPI
GDO_LINKAGE FARPROC gdo_sym(const char *symbol, const gdo_char_t *msg, bool *rv);
#define _gdo_sym(a, b)  gdo_sym(a, _T(a), b)
#else
GDO_LINKAGE void *gdo_sym(const char *symbol, bool *rv);
#define _gdo_sym(a, b)  gdo_sym(a, b)
#endif



/*****************************************************************************/
/*                                save error                                 */
/*****************************************************************************/
#ifdef GDO_WINAPI
/* Save the last system error code. A message for additional information
 * can be provided too. */
GDO_LINKAGE void gdo_save_GetLastError(const gdo_char_t *msg)
{
    gdo_clear_errbuf();
    gdo_hndl.last_errno = GetLastError();

    if (msg) {
        _sntprintf_s(gdo_hndl.buf, sizeof(gdo_hndl.buf)-1, _TRUNCATE, _T("%s"), msg);
    }
}
#else
/* Save the last message provided by dlerror() */
GDO_LINKAGE void gdo_save_dlerror(void)
{
    gdo_clear_errbuf();
    const char *ptr = dlerror();
    if (ptr) snprintf(gdo_hndl.buf, sizeof(gdo_hndl.buf)-1, "%s", ptr);
}
#endif //!GDO_WINAPI

/* Clear error buffers. */
inline
GDO_LINKAGE void gdo_clear_errbuf(void)
{
    gdo_hndl.buf[0] = 0;
#ifdef GDO_WINAPI
    gdo_hndl.buf_formatted[0] = 0;
    gdo_hndl.last_errno = 0;
#else
    (GDO_UNUSED_RESULT) dlerror();
#endif
}

/* Sets the "no library was loaded" error message */
inline
GDO_LINKAGE void gdo_set_error_no_library_loaded(void)
{
#ifdef GDO_WINAPI
        gdo_hndl.last_errno = ERROR_INVALID_HANDLE;
        _tcscpy_s(gdo_hndl.buf, sizeof(gdo_hndl.buf)-1, _T("no library was loaded"));
#else
        strcpy(gdo_hndl.buf, "no library was loaded");
#endif
}
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

    (GDO_UNUSED) new_namespace;
    gdo_hndl.handle = LoadLibraryEx(filename, NULL, flags);

    if (!gdo_lib_is_loaded()) {
        gdo_save_GetLastError(filename);
        return false;
    }

#else //!GDO_WINAPI

    /* dlfcn */

#ifdef GDO_NO_DLMOPEN
    /* dlmopen() disabled */
    (GDO_UNUSED) new_namespace;
    gdo_hndl.handle = dlopen(filename, flags);
#else
    /* call dlmopen() for new namespace, otherwise dlopen() */
    if (new_namespace) {
        gdo_hndl.handle = dlmopen(LM_ID_NEWLM, filename, flags);
    } else {
        gdo_hndl.handle = dlopen(filename, flags);
    }
#endif //GDO_NO_DLMOPEN

    /* check if dl(m)open() was successful */
    if (!gdo_lib_is_loaded()) {
        gdo_save_dlerror();
        return false;
    }

#endif //!GDO_WINAPI

    gdo_register_free_lib();

    return true;
}

/* register our call to free the library handle with atexit()
 * so that the library will automatically be freed upon exit */
inline
GDO_LINKAGE void gdo_register_free_lib(void)
{
#ifdef GDO_AUTO_RELEASE
    if (!gdo_hndl.call_free_lib_is_registered) {
        atexit(gdo_call_free_lib);
        gdo_hndl.call_free_lib_is_registered = true;
    }
#endif
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
    gdo_hndl.GDO_SYMBOL_ptr_ = NULL;
    gdo_hndl.GDO_OBJ_SYMBOL_ptr_ = NULL;

    /* set back to false */
    gdo_hndl.GDO_SYMBOL_loaded_ = false;
    gdo_hndl.GDO_OBJ_SYMBOL_loaded_ = false;

    return true;
}
/*****************************************************************************/



/*****************************************************************************/
/*                    check if all symbols are loaded                        */
/*****************************************************************************/
GDO_LINKAGE bool gdo_symbols_loaded(void)
{
    if (true
        && gdo_hndl.GDO_SYMBOL_loaded_
        && gdo_hndl.GDO_OBJ_SYMBOL_loaded_
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
    if (gdo_symbols_loaded()) {
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
    /* GDO_SYMBOL */@
    gdo_hndl.GDO_SYMBOL_ptr_ = @
        (GDO_TYPE (*)(GDO_ARGS))@
            _gdo_sym("GDO_SYMBOL", &gdo_hndl.GDO_SYMBOL_loaded_);@
    if (!gdo_hndl.GDO_SYMBOL_loaded_ && !ignore_errors) {@
        return false;@
    }
@
    /* GDO_OBJ_SYMBOL */@
    gdo_hndl.GDO_OBJ_SYMBOL_ptr_ = (GDO_OBJ_TYPE *)@
            _gdo_sym("GDO_OBJ_SYMBOL", &gdo_hndl.GDO_OBJ_SYMBOL_loaded_);@
    if (!gdo_hndl.GDO_OBJ_SYMBOL_loaded_ && !ignore_errors) {@
        return false;@
    }

    gdo_clear_errbuf();

    return gdo_symbols_loaded();
}

#ifdef GDO_WINAPI

GDO_LINKAGE FARPROC gdo_sym(const char *symbol, const gdo_char_t *msg, bool *rv)
{
    gdo_clear_errbuf();

    FARPROC ptr = GetProcAddress(gdo_hndl.handle, symbol);

    if (!ptr) {
        gdo_save_GetLastError(msg);
        *rv = false;
        return NULL;
    }

    *rv = true;
    return ptr;
}

#else //!GDO_WINAPI

GDO_LINKAGE void *gdo_sym(const char *symbol, bool *rv)
{
    gdo_clear_errbuf();

    void *ptr = dlsym(gdo_hndl.handle, symbol);

    /* NULL can be a valid value (unusual but possible),
     * so call dlerror() to check for errors */
    if (!ptr) {
        const char *err = dlerror();

        if (err) {
            /* must save our error message manually instead of
             * invoking gdo_save_dlerror() */
            gdo_clear_errbuf();
            snprintf(gdo_hndl.buf, sizeof(gdo_hndl.buf)-1, "%s", err);

            *rv = false;
            return NULL;
        }
    }

    *rv = true;
    return ptr;
}

#endif //!GDO_WINAPI
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

    /* get symbol address */
    if (symbol && *symbol) {
@
        /* GDO_SYMBOL */@
        if (strcmp("GDO_SYMBOL", symbol) == 0) {@
            gdo_hndl.GDO_SYMBOL_ptr_ =@
                (GDO_TYPE (*)(GDO_ARGS))@
                    _gdo_sym("GDO_SYMBOL", &gdo_hndl.GDO_SYMBOL_loaded_);@
            return gdo_hndl.GDO_SYMBOL_loaded_;@
        }
@
        /* GDO_OBJ_SYMBOL */@
        if (strcmp("GDO_OBJ_SYMBOL", symbol) == 0) {@
            gdo_hndl.GDO_OBJ_SYMBOL_ptr_ = (GDO_OBJ_TYPE *)@
                    _gdo_sym("GDO_OBJ_SYMBOL", &gdo_hndl.GDO_OBJ_SYMBOL_loaded_);@
            return gdo_hndl.GDO_OBJ_SYMBOL_loaded_;@
        }
    }

#ifdef GDO_WINAPI
    gdo_hndl.last_errno = ERROR_NOT_FOUND;
    _sntprintf_s(gdo_hndl.buf, sizeof(gdo_hndl.buf)-1, _TRUNCATE,
        _T("symbol not among lookup list: %s"), symbol);
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
            _sntprintf_s(out, bufmax, _TRUNCATE, _T("%s: %s"), msg, buf);
        } else {
            _sntprintf_s(out, bufmax, _TRUNCATE, _T("%s"), buf);
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

#ifdef GDO_WINAPI
    gdo_char_t *origin;
    DWORD len = 260; /* MAX_PATH */

    /* check if library was loaded */
    if (!gdo_lib_is_loaded()) {
        gdo_set_error_no_library_loaded();
        return NULL;
    }

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
#else
    /* use dlinfo() to get a link map */
    struct link_map *lm = NULL;

    if (!gdo_lib_is_loaded()) {
        gdo_set_error_no_library_loaded();
        return NULL;
    } else if (dlinfo(gdo_hndl.handle, RTLD_DI_LINKMAP, &lm) == -1) {
        /* dlinfo() failed */
        gdo_save_dlerror();
        return NULL;
    }

    if (lm->l_name) {
        /* copy string */
        return strdup(lm->l_name);
    }
#endif //GDO_WINAPI

    return NULL;
}
/*****************************************************************************/



/*****************************************************************************/
/*                                wrap code                                  */
/*****************************************************************************/
#ifdef GDO_HAS_WRAP_CODE

/* autoload functions */
#ifdef GDO_ENABLE_AUTOLOAD

#if !defined(GDO_DEFAULT_LIB)
#error "You need to define GDO_DEFAULT_LIB if you want to make use of GDO_ENABLE_AUTOLOAD"
#endif


#if defined(GDO_OS_WIN32) && defined(GDO_USE_MESSAGE_BOX)
/* Windows: show message in a MessageBox window */
GDO_LINKAGE void gdo_win32_last_error_messagebox(const gdo_char_t *symbol)
{
    const gdo_char_t *fmt = _T("error in wrapper function for symbol `%s':\n\n%s");
    const gdo_char_t *err = gdo_last_error();

    const size_t len = _tcslen(fmt) + _tcslen(symbol) + _tcslen(err);
    gdo_char_t *buf = (gdo_char_t *)malloc((len + 1) * sizeof(gdo_char_t));
    assert(buf != NULL);

    _sntprintf_s(buf, len, _TRUNCATE, fmt, symbol, err);
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
    (GDO_UNUSED) function;

    /* return immediately if everything is already loaded,
     * otherwise load library + all symbols */
    if (gdo_symbols_loaded() || gdo_load_lib_and_symbols()) {
        return;
    }
#endif

    /* an error has occured: display an error message */

#if defined(GDO_OS_WIN32) && defined(GDO_USE_MESSAGE_BOX)
    /* Windows: popup message box window */
    gdo_win32_last_error_messagebox(symbol);
#elif defined(GDO_OS_WIN32) && defined(_UNICODE)
    /* Windows: output to console (wide characters) */
    fwprintf(stderr, L"error in wrapper function for symbol `%s':\n%s\n",
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

#else //!GDO_ENABLE_AUTOLOAD

#define gdo_quick_load(a,b)  /**/

#endif //!GDO_ENABLE_AUTOLOAD
/*****************************************************************************/


/* wrapped functions
 * (creating wrapped symbols doesn't work well with pointers to objects) */

#ifdef GDO_WRAP_FUNCTIONS
@
GDO_VISIBILITY GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
    gdo_quick_load("GDO_SYMBOL", _T("GDO_SYMBOL"));@
    GDO_RET gdo_hndl.GDO_SYMBOL_ptr_(GDO_NOTYPE_ARGS);@
}

#endif //GDO_WRAP_FUNCTIONS


#endif //GDO_HAS_WRAP_CODE
/***************************** end of wrap code ******************************/
