/*****************************************************************************/
/*                                save error                                 */
/*****************************************************************************/
#ifdef GDO_WINAPI
# define GDO_SET_LAST_ERRNO(x)  do { gdo_hndl.last_errno = x; } while(0)
#else
# define GDO_SET_LAST_ERRNO(x)  /**/
#endif

/* save message to error buffer */
GDO_INLINE void _gdo_save_to_errbuf(const gdo_char_t *msg)
{
    if (msg) {
        GDO_SNPRINTF(gdo_hndl.errbuf, _T("%s"), msg);
    } else {
        gdo_hndl.errbuf[0] = 0;
    }

#ifdef GDO_WINAPI
    gdo_hndl.formatted[0] = 0;
#endif
}

#ifdef GDO_WINAPI

/* clear error buffers */
GDO_INLINE void _gdo_clear_error(void)
{
    gdo_hndl.errbuf[0] = 0;
    gdo_hndl.formatted[0] = 0;
    gdo_hndl.last_errno = 0;
}

/* save the last system error code; a message for additional information
 * can be provided too. */
GDO_INLINE void _gdo_save_error(const gdo_char_t *msg)
{
    gdo_hndl.last_errno = GetLastError();
    _gdo_save_to_errbuf(msg);
}

/* sets the "no library was loaded" error message */
GDO_INLINE void _gdo_set_error_no_library_loaded(void)
{
    gdo_hndl.last_errno = ERROR_INVALID_HANDLE;
    _gdo_save_to_errbuf(_T("no library was loaded"));
}

#else
/*********************************** dlfcn ***********************************/

/* clear error buffers */
GDO_INLINE void _gdo_clear_error(void)
{
    dlerror();
    gdo_hndl.errbuf[0] = 0;
}

/* save the last message provided by dlerror() */
GDO_INLINE void _gdo_save_error(const gdo_char_t *msg)
{
    GDO_UNUSED(msg);
    _gdo_save_to_errbuf(dlerror());
}

/* sets the "no library was loaded" error message */
GDO_INLINE void _gdo_set_error_no_library_loaded(void)
{
    _gdo_save_to_errbuf("no library was loaded");
}

#endif //!GDO_WINAPI
/*****************************************************************************/


/*****************************************************************************/
/*                   retrieve the last saved error message                   */
/*****************************************************************************/
GDO_LINKAGE const gdo_char_t *gdo_last_error(void)
{
#ifdef GDO_WINAPI

    /* formatted message was already saved */
    if (gdo_hndl.formatted[0] != 0) {
        return gdo_hndl.formatted;
    }

    gdo_char_t *buf = NULL;

    FormatMessage(GDO_FORMAT_MESSAGE_FLAGS,
                  NULL,
                  gdo_hndl.last_errno,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (gdo_char_t *)&buf,
                  0,
                  NULL);

    if (buf) {
        if (gdo_hndl.errbuf[0] != 0) {
            /* put custom message in front */
            GDO_SNPRINTF(gdo_hndl.formatted, _T("%s: %s"), gdo_hndl.errbuf, buf);
        } else {
            GDO_SNPRINTF(gdo_hndl.formatted, _T("%s"), buf);
        }

        LocalFree(buf);
    } else {
        /* FormatMessage() failed, save the error code */
        if (gdo_hndl.errbuf[0] != 0) {
            /* put custom message in front */
            GDO_SNPRINTF(gdo_hndl.formatted, _T("%s: error code: %zu"), gdo_hndl.errbuf,
                (size_t)gdo_hndl.last_errno);
        } else {
            GDO_SNPRINTF(gdo_hndl.formatted, _T("error code: %zu"), (size_t)gdo_hndl.last_errno);
        }
    }

    return gdo_hndl.formatted;

#else

    if (gdo_hndl.errbuf[0] == 0) {
        _gdo_save_to_errbuf("no error");
    }

    return gdo_hndl.errbuf;

#endif //GDO_WINAPI
}
/*****************************************************************************/




