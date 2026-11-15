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
GDO_INLINE void _gdo_load_library(const gdo_char_t *filename, int flags, bool new_namespace);
#ifdef GDO_WINAPI
GDO_INLINE HMODULE _gdo_load_library_ex(const gdo_char_t *filename, int flags);
#else
GDO_INLINE gdo_hmod_t _gdo_call_dlopen(const char *filename, int flags, bool new_namespace);
#endif

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

    GDO_UNUSED(new_namespace);
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
#if !defined(_GDO_TARGET_WIDECHAR) && defined(GDO_CONVERT_FILENAME)
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
#else
    return LoadLibraryEx(filename, NULL, flags);
#endif //!_GDO_TARGET_WIDECHAR && GDO_CONVERT_FILENAME
}

#else //!GDO_WINAPI

GDO_INLINE gdo_hmod_t _gdo_call_dlopen(const char *filename, int flags, bool new_namespace)
{
#ifdef GDO_HAVE_DLMOPEN
    if (new_namespace) {
        return dlmopen(LM_ID_NEWLM, filename, flags);
    }
#else
    (void)new_namespace;
#endif

    return dlopen(filename, flags);
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




