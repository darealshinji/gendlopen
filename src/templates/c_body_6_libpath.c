/*****************************************************************************/
/*                        get the loaded library path                        */
/*****************************************************************************/
GDO_INLINE bool _gdo_library_path(void);

GDO_LINKAGE const gdo_char_t *gdo_library_path(void)
{
#if defined(GDO_HAVE_DLINFO) || \
    defined(GDO_HAVE_DLADDR) || \
    defined(GDO_WINAPI) || \
    defined(__linux__) || \
    defined(__HAIKU__) || \
    defined(_AIX)

    _gdo_clear_error();

    if (!gdo_lib_is_loaded()) {
        _gdo_set_error_no_library_loaded();
        return NULL;
    }

    if (gdo_hndl.libpath[0] != 0 || _gdo_library_path()) {
        return gdo_hndl.libpath;
    }

#else

    _gdo_save_to_errbuf("function not implemented");

#endif

    /* error */
    return NULL;
}


#ifdef GDO_WINAPI
GDO_INLINE bool _gdo_library_path(void)
{
    /* use GetModuleFileName() to get the path */

    const gdo_char_t *msg = (sizeof(gdo_char_t) == 1)
        ? _T("GetModuleFileNameA()")
        : _T("GetModuleFileNameW()");

    DWORD nSize = GetModuleFileName(gdo_hndl.handle, gdo_hndl.libpath, GDO_BUFLEN);

    if (nSize > 0 && nSize < GDO_BUFLEN) {
        return true;
    }

    _gdo_save_error(msg);

    return false;
}
#endif //GDO_WINAPI


#if defined(GDO_HAVE_DLINFO) || defined(GDO_HAVE_DLADDR)
GDO_INLINE bool _gdo_copy_libpath(const char *path)
{
    size_t len = strlen(path);

    if (len < GDO_BUFLEN) {
        memcpy(gdo_hndl.libpath, path, len + 1);
        return true;
    }

    return false;
}
#endif //GDO_HAVE_DLINFO || GDO_HAVE_DLADDR


