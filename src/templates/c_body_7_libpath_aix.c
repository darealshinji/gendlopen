#ifdef _AIX

/* parse ld_info struct and search for library filename that provides the address of `sym' */
GDO_INLINE const char *_gdo_aix_parse_ldinfo(uint8_t *buf, size_t size, uint8_t *sym, const char **member)
{
    struct ld_info *cur = (struct ld_info *)buf;

    while (cur < (buf + size) && cur->ldinfo_next != 0) {
        uint8_t *text = (uint8_t *)cur->ldinfo_textorg;
        uint8_t *data = (uint8_t *)cur->ldinfo_dataorg;
        uint8_t *text_end = text + cur->ldinfo_textsize;
        uint8_t *data_end = data + cur->ldinfo_datasize;

        /* check if symbol pointer address lies within current text or data section */
        if ((sym >= text && sym < text_end) || (sym >= data && sym < data_end)) {
            /* found */
            break;
        } else {
            /* next entry in linked list */
            cur = (struct ld_info *)((uint8_t *)cur + cur->ldinfo_next);
        }
    }

    const char *path = cur->ldinfo_filename;

    if (!path || path[0] == 0) {
        return NULL;
    }

    /* archive member name after library path (can be empty) */
    *member = path + strlen(path) + 1;

    return path;
}

GDO_INLINE bool _gdo_libpath_from_symptr(uint8_t *buf, size_t size, uint8_t *sym)
{
    const char *path, *member;
    size_t len;

    if (!sym) {
        return false;
    }

    member = NULL;
    path = _gdo_aix_parse_ldinfo(buf, size, sym, (const char **)&member);

    if (!path) {
        return false;
    }

    /* check for an archive member name */
    if (member && member[0] != 0) {
        len = strlen(path) + strlen(member) + 3; /* + parentheses and NUL byte */

        if (len > GDO_BUFLEN) {
            return false;
        }

        snprintf(gdo_hndl.libpath, len, "%s(%s)", path, member);
    } else {
        /* path is not an archive */
        len = strlen(path) + 1; /* + NUL byte */

        if (len > GDO_BUFLEN) {
            return false;
        }

        memcpy(gdo_hndl.libpath, path, len);
    }

    return true;
}

GDO_INLINE bool _gdo_library_path(void)
{
    uint8_t buf[GDO_AIX_LOADQUERY_BUFLEN];

    if (gdo_no_symbols_loaded()) {
        _gdo_save_to_errbuf("no symbols were loaded");
        return false;
    }

    if (loadquery(L_GETINFO, buf, sizeof(buf)) != -1) {
        if (false
            || _gdo_libpath_from_symptr(buf, sizeof(buf), (uint8_t *)GDO_RAWPTR_%%symbol%%)
        ) {
            return true;
        }
    }

    _gdo_save_to_errbuf("loadquery() failed to get the library path");

    return false;
}

#endif //_AIX


