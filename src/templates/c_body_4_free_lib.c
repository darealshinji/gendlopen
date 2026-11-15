/*****************************************************************************/
/*             free the library handle and set pointers to NULL              */
/*****************************************************************************/
GDO_LINKAGE bool _gdo_free_lib(bool return_on_error);

GDO_LINKAGE bool gdo_free_lib(void)
{
    return _gdo_free_lib(true);
}

GDO_INLINE bool _gdo_call_dlclose(gdo_hmod_t handle)
{
#ifdef GDO_WINAPI
    return (FreeLibrary(handle) == TRUE);
#else
    return (dlclose(handle) == 0);
#endif
}

GDO_LINKAGE bool _gdo_free_lib(bool return_on_error)
{
    _gdo_clear_error();

    if (gdo_lib_is_loaded()) {
        if (!_gdo_call_dlclose(gdo_hndl.handle) && return_on_error)
        {
#ifdef GDO_WINAPI
            _gdo_save_error(_T("FreeLibrary()"));
#else
            _gdo_save_error(NULL);
#endif
            return false;
        }
    }

    gdo_hndl.libpath[0] = 0;

    /* set pointers back to NULL */
    gdo_hndl.handle = NULL;
    GDO_RAWPTR_%%symbol%% = NULL;

    return true;
}
/*****************************************************************************/


/*****************************************************************************/
/*    free the library handle and set pointers to NULL (no error checks)     */
/*****************************************************************************/
GDO_LINKAGE void gdo_force_free_lib(void)
{
    _gdo_free_lib(false);
}
/*****************************************************************************/


/*****************************************************************************/
/*                   automatically free library upon exit                    */
/*****************************************************************************/
GDO_LINKAGE bool gdo_enable_autorelease(void)
{
    _gdo_clear_error();

    if (!gdo_hndl.free_lib_reg) {
        if (atexit(gdo_force_free_lib) == 0) {
            gdo_hndl.free_lib_reg = true;
        } else {
            _gdo_save_to_errbuf(_T("atexit(): failed to register `gdo_force_free_lib()'"));
        }
    }

    return gdo_hndl.free_lib_reg;
}
/*****************************************************************************/




