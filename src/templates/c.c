#if defined _MSC_VER && defined(_$USE_MESSAGE_BOX)
    #pragma comment(lib, "user32.lib")
#endif

#ifdef _$WINAPI
    #include <tchar.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef _T
    #define _T(x) x
#endif


_$LINKAGE void $call_free_lib(void);
_$LINKAGE void $register_free_lib(void);
_$LINKAGE void $clear_errbuf(void);

_$LINKAGE void *$sym(const char *symbol, const $char_t *msg, bool *rv);

#ifdef _$WINAPI
    #define _$SYM(a, b)  $sym(a, _T(a), b)
#else
    #define _$SYM(a, b)  $sym(a, NULL, b)
#endif



/* Our library and symbols handle */
typedef struct
{
#ifdef _$WINAPI
    HMODULE handle;
    DWORD last_errno;
    /* FormatMessage: according to MSDN the maximum is either 64k or 128k */
    $char_t buf_formatted[64*1024];
#else
    void *handle;
#endif
    bool call_free_lib_is_registered;
    $char_t buf[4096];

    /* symbols */
    GDO_TYPE (*GDO_SYMBOL_ptr_)(GDO_ARGS);
    GDO_OBJ_TYPE *GDO_OBJ_SYMBOL_ptr_;

    bool GDO_SYMBOL_loaded_;
    bool GDO_OBJ_SYMBOL_loaded_;

} $handle_t;

_$LINKAGE $handle_t $hndl = {0};



/***************************************************************************/
/* save error */
/***************************************************************************/
#ifdef _$WINAPI
/* Save the last system error code. A message for additional information
 * can be provided too. */
_$LINKAGE void $save_error(const $char_t *msg)
{
    $clear_errbuf();
    $hndl.last_errno = GetLastError();

    if (msg) {
        _sntprintf_s($hndl.buf, sizeof($hndl.buf)-1, _TRUNCATE, _T("%s"), msg);
    }
}
#else
/* Save the last message provided by dlerror() */
_$LINKAGE void $save_dl_error(void)
{
    $clear_errbuf();
    const char *ptr = dlerror();
    if (ptr) snprintf($hndl.buf, sizeof($hndl.buf)-1, "%s", ptr);
}
#endif //!_$WINAPI

/* Clear error buffers. */
inline
_$LINKAGE void $clear_errbuf(void)
{
    $hndl.buf[0] = 0;
#ifdef _$WINAPI
    $hndl.buf_formatted[0] = 0;
    $hndl.last_errno = 0;
#endif
}

/* Sets the "no library was loaded" error message */
inline
_$LINKAGE void $set_error_no_library_loaded(void)
{
#ifdef _$WINAPI
        $hndl.last_errno = ERROR_INVALID_HANDLE;
        _tcscpy_s($hndl.buf, sizeof($hndl.buf)-1, _T("no library was loaded"));
#else
        strcpy($hndl.buf, "no library was loaded");
#endif
}
/***************************************************************************/



/***************************************************************************/
/* load default library with default flags */
/***************************************************************************/
#ifdef _$DEFAULT_LIB
_$LINKAGE bool $load_lib(void)
{
    return $load_lib_args(_$DEFAULT_LIB, _$DEFAULT_FLAGS, false);
}
#endif
/***************************************************************************/



/***************************************************************************/
/* load default library with default flags and load the symbols */
/***************************************************************************/
#ifdef _$DEFAULT_LIB
_$LINKAGE bool $load_lib_and_symbols(void)
{
    return ($load_lib_args(_$DEFAULT_LIB, _$DEFAULT_FLAGS, false) &&
        $load_symbols(false));
}
#endif
/***************************************************************************/



/***************************************************************************/
/* load library by filename with default flags */
/***************************************************************************/
_$LINKAGE bool $load_lib_name(const $char_t *filename)
{
    return $load_lib_args(filename, _$DEFAULT_FLAGS, false);
}
/***************************************************************************/



/***************************************************************************/
/* load the library */
/***************************************************************************/
_$LINKAGE bool $load_lib_args(const $char_t *filename, int flags, bool new_namespace)
{
    $clear_errbuf();

    /* check if the library was already loaded */
    if ($lib_is_loaded()) {
        return true;
    }

#ifdef _$WINAPI

    (void)new_namespace; /* unused */
    $hndl.handle = LoadLibraryEx(filename, NULL, flags);

    if (!$lib_is_loaded()) {
        $save_error(filename);
        return false;
    }

#else //!_$WINAPI

    /* dlfcn */

#ifdef _$NO_DLMOPEN
    /* dlmopen() disabled */
    (void)new_namespace; /* unused */
    $hndl.handle = dlopen(filename, flags);
#else
    /* call dlmopen() for new namespace, otherwise dlopen() */
    if (new_namespace) {
        $hndl.handle = dlmopen(LM_ID_NEWLM, filename, flags);
    } else {
        $hndl.handle = dlopen(filename, flags);
    }
#endif //_$NO_DLMOPEN

    /* check if dl(m)open() was successful */
    if (!$lib_is_loaded()) {
        $save_dl_error();
        return false;
    }

#endif //!_$WINAPI

    $register_free_lib();

    return true;
}

/* register our call to free the library handle with atexit()
 * so that the library will automatically be freed upon exit */
inline
_$LINKAGE void $register_free_lib(void)
{
#ifdef _$AUTO_RELEASE
    if (!$hndl.call_free_lib_is_registered) {
        atexit($call_free_lib);
        $hndl.call_free_lib_is_registered = true;
    }
#endif
}

/* If registered with atexit() this function will be called at
 * the program's exit. Function must be of type "void (*)(void)". */
_$LINKAGE void $call_free_lib(void)
{
    if ($lib_is_loaded()) {
#ifdef _$WINAPI
        FreeLibrary($hndl.handle);
#else
        dlclose($hndl.handle);
#endif
    }
}
/***************************************************************************/



/***************************************************************************/
/* whether the library is currently loaded */
/***************************************************************************/
_$LINKAGE bool $lib_is_loaded(void)
{
    return ($hndl.handle != NULL);
}
/***************************************************************************/



/***************************************************************************/
/* Free the library handle and set pointers to NULL */
/***************************************************************************/
_$LINKAGE bool $free_lib(void)
{
    $clear_errbuf();

    if (!$lib_is_loaded()) {
        /* nothing to free */
        return true;
    }

#ifdef _$WINAPI
    if (FreeLibrary($hndl.handle) == FALSE) {
        $save_error(_T("FreeLibrary()"));
        return false;
    }
#else
    if (dlclose($hndl.handle) != 0) {
        $save_dl_error();
        return false;
    }
#endif

    /* set pointers back to NULL */
    $hndl.handle = NULL;
    $hndl.GDO_SYMBOL_ptr_ = NULL;
    $hndl.GDO_OBJ_SYMBOL_ptr_ = NULL;

    /* set back to false */
    $hndl.GDO_SYMBOL_loaded_ = false;
    $hndl.GDO_OBJ_SYMBOL_loaded_ = false;

    return true;
}
/***************************************************************************/



/***************************************************************************/
/* check if all symbols are loaded */
/***************************************************************************/
_$LINKAGE bool $symbols_loaded(void)
{
    if (true
        && $hndl.GDO_SYMBOL_loaded_
        && $hndl.GDO_OBJ_SYMBOL_loaded_
    ) {
        return true;
    }

    return false;
}
/***************************************************************************/



/***************************************************************************/
/* load all symbols; can safely be called multiple times. */
/***************************************************************************/
_$LINKAGE bool $load_symbols(bool ignore_errors)
{
    $clear_errbuf();

    /* already loaded all symbols */
    if ($symbols_loaded()) {
        return true;
    }

    /* no library was loaded */
    if (!$lib_is_loaded()) {
        $set_error_no_library_loaded();
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
    $hndl.GDO_SYMBOL_ptr_ = @
        (GDO_TYPE (*)(GDO_ARGS))@
            _$SYM("GDO_SYMBOL", &$hndl.GDO_SYMBOL_loaded_);@
    if (!$hndl.GDO_SYMBOL_loaded_ && !ignore_errors) {@
        return false;@
    }
@
    /* GDO_OBJ_SYMBOL */@
    $hndl.GDO_OBJ_SYMBOL_ptr_ = (GDO_OBJ_TYPE *)@
            _$SYM("GDO_OBJ_SYMBOL", &$hndl.GDO_OBJ_SYMBOL_loaded_);@
    if (!$hndl.GDO_OBJ_SYMBOL_loaded_ && !ignore_errors) {@
        return false;@
    }

    $clear_errbuf();

    return $symbols_loaded();
}

_$LINKAGE void *$sym(const char *symbol, const $char_t *msg, bool *rv)
{
    *rv = false;

#ifdef _$WINAPI
    void *ptr = (void *)GetProcAddress($hndl.handle, symbol);

    if (!ptr) {
        $save_error(msg);
        return NULL;
    }
#else //!_$WINAPI
    (void)msg; /* unused */
    (void)dlerror(); /* clear buffer */

    void *ptr = dlsym($hndl.handle, symbol);

    /* NULL can be a valid value (unusual but possible),
     * so call dlerror() to check for errors */
    const char *err = dlerror();

    if (err) {
        /* must save our error message manually instead of
         * invoking $save_dl_error() */
        $clear_errbuf();
        snprintf($hndl.buf, sizeof($hndl.buf)-1, "%s", err);
        return NULL;
    }
#endif //!_$WINAPI

    *rv = true;
    return ptr;
}
/***************************************************************************/



/***************************************************************************/
/* load a specific symbol;
 * The main intention is to check if a certain symbol is present in a library
 * so you can conditionally enable or disable features in your program. */
/***************************************************************************/
_$LINKAGE bool $load_symbol(const char *symbol)
{
    $clear_errbuf();

    /* no library was loaded */
    if (!$lib_is_loaded()) {
        $set_error_no_library_loaded();
        return false;
    }

    if (!symbol || !*symbol) {
        return false;
    }

    /* get symbol address */
@
    /* GDO_SYMBOL */@
    if (strcmp("GDO_SYMBOL", symbol) == 0) {@
        $hndl.GDO_SYMBOL_ptr_ =@
            (GDO_TYPE (*)(GDO_ARGS))@
                _$SYM("GDO_SYMBOL", &$hndl.GDO_SYMBOL_loaded_);@
        return $hndl.GDO_SYMBOL_loaded_;@
    }
@
    /* GDO_OBJ_SYMBOL */@
    if (strcmp("GDO_OBJ_SYMBOL", symbol) == 0) {@
        $hndl.GDO_OBJ_SYMBOL_ptr_ = (GDO_OBJ_TYPE *)@
                _$SYM("GDO_OBJ_SYMBOL", &$hndl.GDO_OBJ_SYMBOL_loaded_);@
        return $hndl.GDO_OBJ_SYMBOL_loaded_;@
    }

    return false;
}
/***************************************************************************/



/***************************************************************************/
/* retrieve the last saved error message (can be an empty buffer);
 * On Windows the message will be generated from an error code. */
/***************************************************************************/
_$LINKAGE const $char_t *$last_error(void)
{
#ifdef _$WINAPI
    /* message was already saved */
    if ($hndl.buf_formatted[0] != 0) {
        return $hndl.buf_formatted;
    }

    const size_t bufmax = _countof($hndl.buf_formatted) - 1;
    $char_t *buf = NULL;
    $char_t *msg = $hndl.buf;
    $char_t *out = $hndl.buf_formatted;

    /* format the message */
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                    FORMAT_MESSAGE_FROM_SYSTEM |
                    FORMAT_MESSAGE_MAX_WIDTH_MASK,
                NULL, $hndl.last_errno, 0, (LPTSTR)&buf, 0, NULL);

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
            $hndl.last_errno);
    }

    return out;
#else
    /* simply return the buffer */
    return $hndl.buf;
#endif //_$WINAPI
}
/***************************************************************************/



/***************************************************************************/
/* get the full library path;
 * Result must be deallocated with free(), returns NULL on error. */
/***************************************************************************/
_$LINKAGE $char_t *$lib_origin(void)
{
    $clear_errbuf();

#ifdef _$WINAPI
    $char_t *origin;
    DWORD len = 260; /* MAX_PATH */

    /* check if library was loaded */
    if (!$lib_is_loaded()) {
        $set_error_no_library_loaded();
        return NULL;
    }

    /* allocate enough space */
    origin = ($char_t *)malloc(len * sizeof($char_t));

    if (!origin) {
        $save_error(_T("malloc"));
        return NULL;
    }

    /* receive path from handle */
    if (GetModuleFileName($hndl.handle, origin, len-1) == 0) {
        $save_error(_T("GetModuleFileName"));
        free(origin);
        return NULL;
    }

    /* https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
     * technically the path could exceed 260 characters, but in reality
     * it's practically still stuck at the old MAX_PATH value */
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        len = 32*1024;
        origin = ($char_t *)realloc(origin, len * sizeof($char_t));

        if (GetModuleFileName($hndl.handle, origin, len-1) == 0) {
            $save_error(_T("GetModuleFileName"));
            free(origin);
            return NULL;
        }
    }

    return origin;
#else
    /* use dlinfo() to get a link map */
    struct link_map *lm = NULL;

    if (!$lib_is_loaded()) {
        $set_error_no_library_loaded();
        return NULL;
    } else if (dlinfo($hndl.handle, RTLD_DI_LINKMAP, &lm) == -1) {
        /* dlinfo() failed */
        $save_dl_error();
        return NULL;
    }

    if (lm->l_name) {
        /* copy string */
        return strdup(lm->l_name);
    }
#endif //_$WINAPI

    return NULL;
}
/***************************************************************************/
