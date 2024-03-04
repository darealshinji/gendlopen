#if defined _MSC_VER && defined(GDO_USE_MESSAGE_BOX)
    #pragma comment(lib, "user32.lib")
#endif

#ifdef GDO_WINAPI
    #include <tchar.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _T
    #define _T(x) x
#endif


GDO_LINKAGE void gdo_call_free_lib(void);
GDO_LINKAGE void gdo_register_free_lib(void);
GDO_LINKAGE void gdo_clear_errbuf(void);

GDO_LINKAGE void *gdo_sym(const char *symbol, const gdo_char_t *msg, bool *rv);

#ifdef GDO_WINAPI
    #define GDO_SYM(a, b)  gdo_sym(a, _T(a), b)
#else
    #define GDO_SYM(a, b)  gdo_sym(a, NULL, b)
#endif



/* Our library and symbols handle */
typedef struct
{
#ifdef GDO_WINAPI
    HMODULE handle;
    DWORD last_errno;
    /* FormatMessage: according to MSDN the maximum is either 64k or 128k */
    gdo_char_t buf_formatted[64*1024];
#else
    void *handle;
#endif
    bool call_free_lib_is_registered;
    gdo_char_t buf[4096];

    /* symbols */
    GDO_TYPE (*GDO_SYMBOL_ptr_)(GDO_ARGS);
    GDO_OBJ_TYPE *GDO_OBJ_SYMBOL_ptr_;

    bool GDO_SYMBOL_loaded_;
    bool GDO_OBJ_SYMBOL_loaded_;

} gdo_handle_t;

GDO_LINKAGE gdo_handle_t gdo_hndl = {0};



/***************************************************************************/
/* save error */
/***************************************************************************/
#ifdef GDO_WINAPI
/* Save the last system error code. A message for additional information
 * can be provided too. */
GDO_LINKAGE void gdo_save_error(const gdo_char_t *msg)
{
    gdo_clear_errbuf();
    gdo_hndl.last_errno = GetLastError();

    if (msg) {
        _sntprintf_s(gdo_hndl.buf, sizeof(gdo_hndl.buf)-1, _TRUNCATE, _T("%s"), msg);
    }
}
#else
/* Save the last message provided by dlerror() */
GDO_LINKAGE void gdo_save_dl_error(void)
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
/***************************************************************************/



/***************************************************************************/
/* load default library with default flags */
/***************************************************************************/
#ifdef GDO_DEFAULT_LIB
GDO_LINKAGE bool gdo_load_lib(void)
{
    return gdo_load_lib_args(GDO_DEFAULT_LIB, GDO_DEFAULT_FLAGS, false);
}
#endif
/***************************************************************************/



/***************************************************************************/
/* load default library with default flags and load the symbols */
/***************************************************************************/
#ifdef GDO_DEFAULT_LIB
GDO_LINKAGE bool gdo_load_lib_and_symbols(void)
{
    return (gdo_load_lib_args(GDO_DEFAULT_LIB, GDO_DEFAULT_FLAGS, false) &&
        gdo_load_symbols(false));
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
    gdo_clear_errbuf();

    /* check if the library was already loaded */
    if (gdo_lib_is_loaded()) {
        return true;
    }

#ifdef GDO_WINAPI

    (void)new_namespace; /* unused */
    gdo_hndl.handle = LoadLibraryEx(filename, NULL, flags);

    if (!gdo_lib_is_loaded()) {
        gdo_save_error(filename);
        return false;
    }

#else //!GDO_WINAPI

    /* dlfcn */

#ifdef GDO_NO_DLMOPEN
    /* dlmopen() disabled */
    (void)new_namespace; /* unused */
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
        gdo_save_dl_error();
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
/***************************************************************************/



/***************************************************************************/
/* whether the library is currently loaded */
/***************************************************************************/
GDO_LINKAGE bool gdo_lib_is_loaded(void)
{
    return (gdo_hndl.handle != NULL);
}
/***************************************************************************/



/***************************************************************************/
/* Free the library handle and set pointers to NULL */
/***************************************************************************/
GDO_LINKAGE bool gdo_free_lib(void)
{
    gdo_clear_errbuf();

    if (!gdo_lib_is_loaded()) {
        /* nothing to free */
        return true;
    }

#ifdef GDO_WINAPI
    if (FreeLibrary(gdo_hndl.handle) == FALSE) {
        gdo_save_error(_T("FreeLibrary()"));
        return false;
    }
#else
    if (dlclose(gdo_hndl.handle) != 0) {
        gdo_save_dl_error();
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
/***************************************************************************/



/***************************************************************************/
/* check if all symbols are loaded */
/***************************************************************************/
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
/***************************************************************************/



/***************************************************************************/
/* load all symbols; can safely be called multiple times. */
/***************************************************************************/
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
            GDO_SYM("GDO_SYMBOL", &gdo_hndl.GDO_SYMBOL_loaded_);@
    if (!gdo_hndl.GDO_SYMBOL_loaded_ && !ignore_errors) {@
        return false;@
    }
@
    /* GDO_OBJ_SYMBOL */@
    gdo_hndl.GDO_OBJ_SYMBOL_ptr_ = (GDO_OBJ_TYPE *)@
            GDO_SYM("GDO_OBJ_SYMBOL", &gdo_hndl.GDO_OBJ_SYMBOL_loaded_);@
    if (!gdo_hndl.GDO_OBJ_SYMBOL_loaded_ && !ignore_errors) {@
        return false;@
    }

    gdo_clear_errbuf();

    return gdo_symbols_loaded();
}

GDO_LINKAGE void *gdo_sym(const char *symbol, const gdo_char_t *msg, bool *rv)
{
    *rv = false;

#ifdef GDO_WINAPI
    void *ptr = (void *)GetProcAddress(gdo_hndl.handle, symbol);

    if (!ptr) {
        gdo_save_error(msg);
        return NULL;
    }
#else //!GDO_WINAPI
    (void)msg; /* unused */
    (void)dlerror(); /* clear buffer */

    void *ptr = dlsym(gdo_hndl.handle, symbol);

    /* NULL can be a valid value (unusual but possible),
     * so call dlerror() to check for errors */
    const char *err = dlerror();

    if (err) {
        /* must save our error message manually instead of
         * invoking gdo_save_dl_error() */
        gdo_clear_errbuf();
        snprintf(gdo_hndl.buf, sizeof(gdo_hndl.buf)-1, "%s", err);
        return NULL;
    }
#endif //!GDO_WINAPI

    *rv = true;
    return ptr;
}
/***************************************************************************/



/***************************************************************************/
/* load a specific symbol;
 * The main intention is to check if a certain symbol is present in a library
 * so you can conditionally enable or disable features in your program. */
/***************************************************************************/
GDO_LINKAGE bool gdo_load_symbol(const char *symbol)
{
    gdo_clear_errbuf();

    /* no library was loaded */
    if (!gdo_lib_is_loaded()) {
        gdo_set_error_no_library_loaded();
        return false;
    }

    if (!symbol || !*symbol) {
        return false;
    }

    /* get symbol address */
@
    /* GDO_SYMBOL */@
    if (strcmp("GDO_SYMBOL", symbol) == 0) {@
        gdo_hndl.GDO_SYMBOL_ptr_ =@
            (GDO_TYPE (*)(GDO_ARGS))@
                GDO_SYM("GDO_SYMBOL", &gdo_hndl.GDO_SYMBOL_loaded_);@
        return gdo_hndl.GDO_SYMBOL_loaded_;@
    }
@
    /* GDO_OBJ_SYMBOL */@
    if (strcmp("GDO_OBJ_SYMBOL", symbol) == 0) {@
        gdo_hndl.GDO_OBJ_SYMBOL_ptr_ = (GDO_OBJ_TYPE *)@
                GDO_SYM("GDO_OBJ_SYMBOL", &gdo_hndl.GDO_OBJ_SYMBOL_loaded_);@
        return gdo_hndl.GDO_OBJ_SYMBOL_loaded_;@
    }

    return false;
}
/***************************************************************************/



/***************************************************************************/
/* retrieve the last saved error message (can be an empty buffer);
 * On Windows the message will be generated from an error code. */
/***************************************************************************/
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
/***************************************************************************/



/***************************************************************************/
/* get the full library path;
 * Result must be deallocated with free(), returns NULL on error. */
/***************************************************************************/
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

    if (!origin) {
        gdo_save_error(_T("malloc"));
        return NULL;
    }

    /* receive path from handle */
    if (GetModuleFileName(gdo_hndl.handle, origin, len-1) == 0) {
        gdo_save_error(_T("GetModuleFileName"));
        free(origin);
        return NULL;
    }

    /* https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
     * technically the path could exceed 260 characters, but in reality
     * it's practically still stuck at the old MAX_PATH value */
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        len = 32*1024;
        origin = (gdo_char_t *)realloc(origin, len * sizeof(gdo_char_t));

        if (GetModuleFileName(gdo_hndl.handle, origin, len-1) == 0) {
            gdo_save_error(_T("GetModuleFileName"));
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
        gdo_save_dl_error();
        return NULL;
    }

    if (lm->l_name) {
        /* copy string */
        return strdup(lm->l_name);
    }
#endif //GDO_WINAPI

    return NULL;
}
/***************************************************************************/
