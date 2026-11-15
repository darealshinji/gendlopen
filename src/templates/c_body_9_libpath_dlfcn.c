#if !defined(GDO_WINAPI) && !defined(_AIX)

/* receive libary path directly from the handle */
GDO_INLINE void _gdo_libpath_from_handle(void)
{
#ifdef GDO_DLFCN_WIN32

    /* dlfcn-win32:
     * The handle returned by dlopen() is a `HMODULE' casted to `void *',
     * so we can directly use GetModuleFileNameA() to receive the DLL path. */

    DWORD nSize = GetModuleFileNameA((HMODULE)gdo_hndl.handle, gdo_hndl.libpath, GDO_BUFLEN);

    if (nSize > 0 && nSize < GDO_BUFLEN) {
        return;
    }

#elif defined(__HAIKU__)

    size_t len;
    const char *path = (const char *)gdo_hndl.handle;

    /* Haiku:
     * The handle is a pointer to a struct similar to `struct link_map' whose
     * first entry is a char array holding the full library path (1024 chars max) */
    if (path && *path == '/' &&
        (len = strnlen(path, 1024)) < 1024 &&
        _gdo_copy_libpath(path))
    {
        return;
    }

#endif //!GDO_DLFCN_WIN32

    gdo_hndl.libpath[0] = 0;
}


/* use dlinfo() to get a link map */
GDO_INLINE void _gdo_libpath_dlinfo(bool force_abspath)
{
#ifdef GDO_HAVE_DLINFO

    struct link_map *lm = NULL;

    if (dlinfo(gdo_hndl.handle, RTLD_DI_LINKMAP, &lm) == 0 &&
        lm->l_name && lm->l_name[0] != 0)
    {
# ifdef __linux__
        /* try to get the absolute library path from /proc/self/maps */
        if (force_abspath &&
            lm->l_name[0] != '/' &&
            lm->l_addr != 0 &&
            _gdo_abspath_procmap(lm->l_addr))
        {
            return;
        }
# endif

        if (_gdo_copy_libpath(lm->l_name)) {
            return;
        }
    }

#endif //!GDO_HAVE_DLINFO

#if !defined(GDO_HAVE_DLINFO) || !defined(__linux__)
    GDO_UNUSED(force_abspath);
#endif

    gdo_hndl.libpath[0] = 0;
}


#ifdef GDO_HAVE_DLADDR
GDO_INLINE bool _gdo_call_dladdr(const void *addr)
{
    Dl_info info;

    if (addr && dladdr(addr, &info) != 0 &&
        info.dli_fname &&
        info.dli_fname[0] != 0)
    {
        return _gdo_copy_libpath(info.dli_fname);
    }

    return false;
}
#endif //GDO_HAVE_DLADDR


/* use dladdr() to get the library path from a symbol pointer */
GDO_INLINE void _gdo_libpath_dladdr(void)
{
#ifdef GDO_HAVE_DLADDR // dladdr() (POSIX.1-2024)

    if (gdo_any_symbol_loaded()) {
        if (false
            || _gdo_call_dladdr((void *)GDO_RAWPTR_%%symbol%%)
        ) {
            return;
        }
    }

#endif //!GDO_HAVE_DLADDR

    gdo_hndl.libpath[0] = 0;
}


/* parse /proc/self/maps for absolute path */
GDO_INLINE void _gdo_libpath_procmap(void)
{
#ifdef GDO_HAVE_PROC_SELF_MAPS
    void *addr = NULL;

    if (false
        || (addr = (void *)GDO_RAWPTR_%%symbol%%) != NULL
    ) {
        if (_gdo_abspath_procmap((uint64_t)addr)) {
            return;
        }
    }

#endif //GDO_HAVE_PROC_SELF_MAPS

    gdo_hndl.libpath[0] = 0;
}


GDO_INLINE bool _gdo_libpath_found(bool force_abspath)
{
    char *lp = gdo_hndl.libpath;

#ifdef _WIN32

    GDO_UNUSED(force_abspath);

    return (*lp != 0);

#else

    return ((force_abspath && *lp == '/') || (!force_abspath && *lp != 0));

#endif //!_WIN32
}


GDO_INLINE void _gdo_libpath_any_method(bool force_abspath)
{
    /* void *handle */
    _gdo_libpath_from_handle();

    if (_gdo_libpath_found(force_abspath)) {
        return;
    }

    /* dlinfo() */
    _gdo_libpath_dlinfo(force_abspath);

    if (_gdo_libpath_found(force_abspath)) {
        return;
    }

    /* dladdr() */
    _gdo_libpath_dladdr();

    if (_gdo_libpath_found(force_abspath)) {
        return;
    }

    /* /proc/self/maps */
    if (force_abspath) {
        _gdo_libpath_procmap();
    }
}


GDO_INLINE bool _gdo_library_path(void)
{
    /* try to get absolute library path,
     * ignoring relative path results */
    _gdo_libpath_any_method(true);

#if !defined(_WIN32)
    /* try to get relative library path */
    if (gdo_hndl.libpath[0] == 0) {
        _gdo_libpath_any_method(false);
    }
#endif

    if (gdo_hndl.libpath[0] == 0) {
        _gdo_save_to_errbuf("failed to get library path");
        return false;
    }

    _gdo_clear_error();

    return true;
}

#endif // !GDO_WINAPI && !_AIX
/*****************************************************************************/




