/*****************************************************************************/
/*                        C API implementation begin                         */
/*****************************************************************************/
#if defined(_MSC_VER) && defined(GDO_USE_MESSAGE_BOX)
# pragma comment(lib, "user32.lib")  /* dependency for MessageBox() */
#endif

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef _AIX
# include <sys/ldr.h>
#endif

#ifdef GDO_HAVE_PROC_SELF_MAPS
# include <ctype.h>
# include <errno.h>
# include <fcntl.h>
# include <inttypes.h>
# include <linux/fs.h>
# include <sys/ioctl.h>
# include <unistd.h>
#endif

#ifdef _GDO_TARGET_WIDECHAR
# define GDO_XHS  L"%hs"  /* narrow character string */
#else
# define GDO_XHS  "%s"
#endif

/* linkage */
#ifdef GDO_STATIC
# define GDO_LINKAGE      GDO_DECL
# define GDO_OBJ_LINKAGE  GDO_OBJ_DECL
#else
# define GDO_LINKAGE      /**/
# define GDO_OBJ_LINKAGE  /**/
#endif

/* Silence `unused' compiler warnings. */
#define GDO_UNUSED(x)  (void)(x)

/* countof() macro */
#ifdef _countof
# define GDO_COUNTOF(x)  _countof(x)
#elif defined(_Countof)
# define GDO_COUNTOF(x)  _Countof(x)
#elif defined(__cplusplus)
template <typename T, size_t N>
  constexpr size_t _gdo_countof(T (&a)[N]) { return N; }
# define GDO_COUNTOF(x)  _gdo_countof(x)
#else
# define GDO_COUNTOF(x)  (sizeof(x) / sizeof(x[0]))
#endif

/* snprintf() / snwprintf() */
#ifdef _WIN32
# define GDO_SNPRINTF(DEST, FORMAT, ...) \
    _sntprintf_s(DEST, GDO_COUNTOF(DEST), _TRUNCATE, FORMAT, __VA_ARGS__)
#else
# define GDO_SNPRINTF(DEST, FORMAT, ...) \
    snprintf(DEST, GDO_COUNTOF(DEST), FORMAT, __VA_ARGS__)
#endif


/* library handle */
GDO_OBJ_LINKAGE gdo_handle_t gdo_hndl;
/*****************************************************************************/




