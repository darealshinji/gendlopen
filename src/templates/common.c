/* common inline routines */

GDO_INLINE void *_gdo_call_dlsym(gdo_hmod_t handle, const char *symbol)
{
#ifdef GDO_WINAPI
    return (void *)GetProcAddress(handle, symbol);
#else
    return dlsym(handle, symbol);
#endif
}


GDO_INLINE bool _gdo_call_dlclose(gdo_hmod_t handle)
{
#ifdef GDO_WINAPI
    return (FreeLibrary(handle) == TRUE);
#else
    return (dlclose(handle) == 0);
#endif
}


#if !defined(GDO_WINAPI)

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


#if defined(GDO_HAVE_DLADDR) && !defined(GDO_HAVE_DLINFO)
GDO_INLINE bool _gdo_call_dladdr(const void *addr, Dl_info *info)
{
    info->dli_fname = NULL;

    return (addr && dladdr(addr, info) != 0 && info->dli_fname && info->dli_fname[0] != 0);
}
#endif

#endif //!GDO_WINAPI


#ifdef _AIX

#include <inttypes.h>
#include <string.h>
#include <sys/ldr.h>


/* parse ld_info struct and search for library filename that provides sym */
GDO_INLINE const char *_gdo_aix_parse_ldinfo(struct ld_info *info, uint8_t *sym, const char **member)
{
    struct ld_info *cur = info;

    while (true) {
        uint8_t *text = (uint8_t *)cur->ldinfo_textorg;
        uint8_t *data = (uint8_t *)cur->ldinfo_dataorg;
        uint8_t *text_end = text + cur->ldinfo_textsize;
        uint8_t *data_end = data + cur->ldinfo_datasize;

        /* check if symbol pointer address lies within current text or data section */
        if ((sym >= text && sym < text_end) || (sym >= data && sym < data_end)) {
            /* found */
            break;
        } else if (cur->ldinfo_next == 0) {
            /* end of linked list */
            return NULL;
        } else {
            /* next entry in linked list */
            cur = (struct ld_info *)((uint8_t *)cur + cur->ldinfo_next);
        }
    }

    const char *path = cur->ldinfo_filename;

    if (!path || path[0] == 0) {
        return NULL;
    }

    /* archive member name after library path */
    *member = path + strlen(path) + 1;

    return path;
}

#endif //_AIX


#if defined(GDO_HAVE_DLINFO) && defined(__linux__)

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/fs.h>

#ifdef PROCMAP_QUERY
# include <fcntl.h>
# include <sys/ioctl.h>
# include <unistd.h>
#endif


/* try to get the full library path from process map */
GDO_INLINE bool _gdo_fullpath_procmap(struct link_map *lm, char *buf, size_t bufsize)
{
#ifdef PROCMAP_QUERY

    /* use ioctl() to parse /proc/self/maps */

    struct procmap_query q;

    int fd = open("/proc/self/maps", O_RDONLY | O_NONBLOCK);

    if (fd != -1) {
        memset(&q, 0, sizeof(struct procmap_query));

        q.size          = sizeof(struct procmap_query);
        q.query_flags   = PROCMAP_QUERY_FILE_BACKED_VMA;
        q.query_addr    = (__u64)lm->l_addr;  /* find this address */
        q.vma_name_size = bufsize;     /* in: buffer size; out: saved path length + NUL byte */
        q.vma_name_addr = (__u64)buf;  /* buffer to save pathname */

        if (ioctl(fd, PROCMAP_QUERY, &q) == 0 && q.vma_name_size > 0) {
            close(fd);
            return true;
        }

        close(fd);
    }

    buf[0] = 0;

#endif //PROCMAP_QUERY

    /* use getline() and sscanf() to parse /proc/self/maps */

    FILE *fp = fopen("/proc/self/maps", "r");

    if (!fp) {
        return false;
    }

    uintmax_t start, end;
    char mode[5];
    char *line = NULL, *path = NULL;
    size_t n = 0;

    while (getline(&line, &n, fp) != -1) {
        /* address start-end         mode offset   device inode      pathname */
        /* 7de29d200000-7de29d228000 r--p 00000000 103:01 2758110    /usr/lib/x86_64-linux-gnu/libc.so.6\n */
        int rv = sscanf(line, "%jx-%jx %4[rwxsp-] %*u %*u:%*u %*u %m[^\n]",
                        &start, &end, mode, &path);

        if (rv == 4 && lm->l_addr >= start && lm->l_addr < end) {
            /* match */
            break;
        }

        free(path);
        path = NULL;
    }

    free(line);
    fclose(fp);

    if (path && (n = strlen(path)) < bufsize) {
        memcpy(buf, path, n + 1);
        free(path);
        return true;
    }

    free(path);
    return false;
}

#endif //GDO_HAVE_DLINFO && __linux__

