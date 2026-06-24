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
#undef GDO_LIBEXTA
#ifdef _WIN32
# define GDO_LIBEXTA  ".dll"
#elif defined(__APPLE__)
# define GDO_LIBEXTA  ".dylib"
#elif defined(_AIX)
# define GDO_LIBEXTA  ".a"
#else /* ELF */
# define GDO_LIBEXTA  ".so"
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
 * AIX: libfoo.a(shr.o) or libfoo.a(shr_64.o)
 * AIX uses archive files without API number.
 * These archives typically contain a 32 bit shared object `shr.o'
 * and a 64 bit shared object `shr_64.o'.
 *
 * Android: libfoo.so
 * ELF format but without API number.
 *
 * ELF: libfoo.so.1.2
 */
#undef GDO_LIBNAMEA
#ifdef _WIN32
# ifdef __MINGW32__
#  define GDO_LIBNAMEA(NAME, API)  "lib" #NAME "-" #API ".dll"
# else
#  define GDO_LIBNAMEA(NAME, API)        #NAME "-" #API ".dll"
# endif
#elif defined(__APPLE__)
# define GDO_LIBNAMEA(NAME, API)   "lib" #NAME "." #API ".dylib"
#elif defined(_AIX)
# if defined(__64BIT__) /* IBM XL C */ || defined(__LP64__) /* GCC */
#  define GDO_LIBNAMEA(NAME, API)  "lib" #NAME ".a(shr_64.o)"
# else
#  define GDO_LIBNAMEA(NAME, API)  "lib" #NAME ".a(shr.o)"
# endif
#elif defined(__ANDROID__)
# define GDO_LIBNAMEA(NAME, API)   "lib" #NAME ".so"
#else /* other ELF systems */
# define GDO_LIBNAMEA(NAME, API)   "lib" #NAME ".so." #API
#endif

/* wide character macros */
#undef GDO_LIBEXTW
#undef GDO_LIBNAMEW
#define GDO_LIBEXTW              L"" GDO_LIBEXTA
#define GDO_LIBNAMEW(NAME, API)  L"" GDO_LIBNAMEA(NAME,API)

/* wide/narrow character default macros */
#undef GDO_LIBEXT
#undef GDO_LIBNAME
#if defined(_WIN32) && defined(_UNICODE)
# define GDO_LIBEXT              GDO_LIBEXTW
# define GDO_LIBNAME(NAME, API)  GDO_LIBNAMEW(NAME, API)
#else
# define GDO_LIBEXT              GDO_LIBEXTA
# define GDO_LIBNAME(NAME, API)  GDO_LIBNAMEA(NAME, API)
#endif

