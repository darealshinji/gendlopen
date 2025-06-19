/*****************************************************************************/
/*                              filename macros                              */
/*****************************************************************************/

/***

GDO_LIBNAME(NAME, API)
GDO_LIBNAMEA(NAME, API)
GDO_LIBNAMEW(NAME, API)
    Convenience macro to create versioned library names including double quote marks.
    GDO_LIBNAME(example,1) for example will become "libexample.1.dylib" on macOS.

    NAME: library name without prefix
    API:  API number to add to filename

GDO_LIBEXT
GDO_LIBEXTA
GDO_LIBEXTW
    Shared library file extension without dot. Useful i.e. on plugins.

***/

/* default library filename extension */
#ifdef GDO_LIBEXT
# undef GDO_LIBEXT
#endif
#ifdef GDO_LIBEXTA
# undef GDO_LIBEXTA
#endif
#ifdef GDO_LIBEXTW
# undef GDO_LIBEXTW
#endif
#ifdef _WIN32
# define GDO_LIBEXTA    ".dll"
# define GDO_LIBEXTW   L".dll"
#elif defined(__APPLE__)
# define GDO_LIBEXTA    ".dylib"
# define GDO_LIBEXTW   L".dylib"
#elif defined(_AIX)
# define GDO_LIBEXTA    ".a"
# define GDO_LIBEXTW   L".a"
#else /* ELF */
# define GDO_LIBEXTA    ".so"
# define GDO_LIBEXTW   L".so"
#endif

/**
 * library name with API number
 *
 * Examples using GDO_LIBNAME(foo, 1.2) macro:
 *
 * Windows: libfoo-1.2.dll when using MinGW, otherwise foo-1.2.dll
 *
 * macOS: libfoo.1.2.dylib
 *
 * AIX: libfoo.a
 * AIX uses archive files without API number.
 *
 * Android: libfoo.so
 * ELF format but without API number.
 *
 * ELF: libfoo.so.1.2
 */
#ifdef GDO_LIBNAME
# undef GDO_LIBNAME
#endif
#ifdef GDO_LIBNAMEA
# undef GDO_LIBNAMEA
#endif
#ifdef GDO_LIBNAMEW
# undef GDO_LIBNAMEW
#endif
#ifdef _WIN32
# ifdef __MINGW32__
#  define GDO_LIBNAMEA(NAME, API)   "lib" #NAME "-" #API ".dll"
#  define GDO_LIBNAMEW(NAME, API)  L"lib" #NAME "-" #API ".dll"
# else
#  define GDO_LIBNAMEA(NAME, API)         #NAME "-" #API ".dll"
#  define GDO_LIBNAMEW(NAME, API)     L"" #NAME "-" #API ".dll"
# endif
#elif defined(__APPLE__)
# define GDO_LIBNAMEA(NAME, API)    "lib" #NAME "." #API ".dylib"
# define GDO_LIBNAMEW(NAME, API)   L"lib" #NAME "." #API ".dylib"
#elif defined(_AIX)
# define GDO_LIBNAMEA(NAME, API)    "lib" #NAME ".a"
# define GDO_LIBNAMEW(NAME, API)   L"lib" #NAME ".a"
#elif defined(__ANDROID__)
# define GDO_LIBNAMEA(NAME, API)    "lib" #NAME ".so"
# define GDO_LIBNAMEW(NAME, API)   L"lib" #NAME ".so"
#else /* default ELF filename */
# define GDO_LIBNAMEA(NAME, API)    "lib" #NAME ".so." #API
# define GDO_LIBNAMEW(NAME, API)   L"lib" #NAME ".so." #API
#endif

/* wide/narrow char default macros */
#if defined(_WIN32) && defined(_UNICODE)
# define GDO_LIBEXT              GDO_LIBEXTW
# define GDO_LIBNAME(NAME, API)  GDO_LIBNAMEW(NAME, API)
#else
# define GDO_LIBEXT              GDO_LIBEXTA
# define GDO_LIBNAME(NAME, API)  GDO_LIBNAMEA(NAME, API)
#endif

