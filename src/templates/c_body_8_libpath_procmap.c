#ifdef GDO_HAVE_PROC_SELF_MAPS

GDO_INLINE bool _gdo_procmap_ioctl(uint64_t addr)
{
# ifdef PROCMAP_QUERY

    /* use ioctl() to parse /proc/self/maps */

    struct procmap_query q;

    int fd = open("/proc/self/maps", O_RDONLY | O_NONBLOCK);

    if (fd != -1) {
        memset(&q, 0, sizeof(struct procmap_query));

        q.size          = sizeof(struct procmap_query);
        q.query_flags   = PROCMAP_QUERY_FILE_BACKED_VMA;
        q.query_addr    = addr;                     /* find this address */
        q.vma_name_size = GDO_BUFLEN;               /* in: buffer size; out: saved path length + NUL byte */
        q.vma_name_addr = (__u64)gdo_hndl.libpath;  /* buffer to save pathname */

        if (ioctl(fd, PROCMAP_QUERY, &q) == 0 &&
            q.vma_name_size > 0 &&
            gdo_hndl.libpath[0] == '/')
        {
            close(fd);
            return true;
        }

        close(fd);
    }

# else

    GDO_UNUSED(addr);

# endif //!PROCMAP_QUERY

    return false;
}


GDO_INLINE char *_gdo_read_hex(char *line, uint64_t *val, char ch_end)
{
    char *endptr = NULL;

    if (!isxdigit(*line)) {
        return NULL;
    }

    errno = 0;
    *val = strtoul(line, &endptr, 16);

    if (errno != 0 || !endptr || *endptr != ch_end) {
        return NULL;
    }

    return endptr;
}


GDO_INLINE char *_gdo_procmap_file(FILE *fp, uint64_t addr)
{
    char *line = NULL;
    char *path = NULL;
    size_t n = 0;
    ssize_t nread;

    while ((nread = getline(&line, &n, fp)) != -1) {
        /* address start-end         mode offset   device inode      pathname */
        /* 7de29d200000-7de29d228000 r--p 00000000 103:01 2758110    /usr/lib/x86_64-linux-gnu/libc.so.6\n */

        uint64_t start = 0, end = 0;
        char *ptr, *ptr_path;

        /* library path */
        if (nread < 1 || (ptr_path = strstr(line, " /")) == NULL) {
            continue;
        }

        ptr_path++;

        /* address start value */
        if ((ptr = _gdo_read_hex(line, &start, '-')) == NULL || addr < start) {
            continue;
        }

        ptr++;

        /* address end value */
        if (_gdo_read_hex(ptr, &end, ' ') == NULL || addr >= end) {
            continue;
        }

        /* remove trailing newline */
        if (line[nread - 1] == '\n') {
            line[nread - 1] = 0;
        }

        path = strdup(ptr_path);
        break;
    }

    free(line);

    return path;
}


/* try to get the absolute library path from process map */
GDO_INLINE bool _gdo_abspath_procmap(uint64_t addr)
{
    FILE *fp;
    char *path;

    if (_gdo_procmap_ioctl(addr)) {
        return true;
    }

    gdo_hndl.libpath[0] = 0;

    /* parse /proc/self/maps line by line */

    fp = fopen("/proc/self/maps", "r");

    if (!fp) {
        return false;
    }

    path = _gdo_procmap_file(fp, addr);
    fclose(fp);

    if (path && *path && _gdo_copy_libpath(path)) {
        free(path);
        return true;
    }

    free(path);
    return false;
}

#endif //GDO_HAVE_PROC_SELF_MAPS


