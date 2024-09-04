/* default library filename extension */
#ifdef LIBEXT
# undef LIBEXT
#endif
#ifdef LIBEXTA
# undef LIBEXTA
#endif
#ifdef LIBEXTW
# undef LIBEXTW
#endif
#ifdef _WIN32
# define LIBEXTA    ".dll"
# define LIBEXTW   L".dll"
#elif defined(__APPLE__) && defined(__MACH__)
# define LIBEXTA    ".dylib"
# define LIBEXTW   L".dylib"
#elif defined(_AIX)
# define LIBEXTA    ".a"
# define LIBEXTW   L".a"
#else /* ELF */
# define LIBEXTA    ".so"
# define LIBEXTW   L".so"
#endif


/**
 * library name with API number
 *
 * Examples using LIBNAME(foo, 1.2) macro:
 *
 * Windows: libfoo-1.2.dll
 * De-facto standard when libraries are ported to Windows.
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
#ifdef LIBNAME
# undef LIBNAME
#endif
#ifdef LIBNAMEA
# undef LIBNAMEA
#endif
#ifdef LIBNAMEW
# undef LIBNAMEW
#endif
#ifdef _WIN32
# define LIBNAMEA(NAME, API)    "lib" #NAME "-" #API ".dll"
# define LIBNAMEW(NAME, API)   L"lib" #NAME "-" #API ".dll"
#elif defined(__APPLE__) && defined(__MACH__)
# define LIBNAMEA(NAME, API)    "lib" #NAME "." #API ".dylib"
# define LIBNAMEW(NAME, API)   L"lib" #NAME "." #API ".dylib"
#elif defined(_AIX)
# define LIBNAMEA(NAME, API)    "lib" #NAME ".a"
# define LIBNAMEW(NAME, API)   L"lib" #NAME ".a"
#elif defined(__ANDROID__)
# define LIBNAMEA(NAME, API)    "lib" #NAME ".so"
# define LIBNAMEW(NAME, API)   L"lib" #NAME ".so"
#else /* default ELF filename */
# define LIBNAMEA(NAME, API)    "lib" #NAME ".so." #API
# define LIBNAMEW(NAME, API)   L"lib" #NAME ".so." #API
#endif


/* Unicode/ANSI default macros */
#if defined(_WIN32) && defined(_UNICODE)
# define LIBEXT              LIBEXTW
# define LIBNAME(NAME, API)  LIBNAMEW(NAME, API)
#else
# define LIBEXT              LIBEXTA
# define LIBNAME(NAME, API)  LIBNAMEA(NAME, API)
#endif
