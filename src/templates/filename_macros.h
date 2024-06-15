/* default library filename extension */
#ifdef LIBEXT
#undef LIBEXT
#endif
#ifdef LIBEXTA
#undef LIBEXTA
#endif
#ifdef LIBEXTW
#undef LIBEXTW
#endif
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MSYS__)
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


/* library name with API number */
#ifdef LIBNAME
#undef LIBNAME
#endif
#ifdef LIBNAMEA
#undef LIBNAMEA
#endif
#ifdef LIBNAMEW
#undef LIBNAMEW
#endif
#if defined(_WIN32) || defined(__CYGWIN__) || defined(__MSYS__)
# define LIBNAMEA(NAME, API)    "lib" #NAME "-" #API ".dll"
# define LIBNAMEW(NAME, API)   L"lib" #NAME "-" #API ".dll"
#elif defined(__APPLE__) && defined(__MACH__)
# define LIBNAMEA(NAME, API)    "lib" #NAME "." #API ".dylib"
# define LIBNAMEW(NAME, API)   L"lib" #NAME "." #API ".dylib"
#elif defined(_AIX)
# define LIBNAMEA(NAME, API)    "lib" #NAME ".a"
# define LIBNAMEW(NAME, API)   L"lib" #NAME ".a"
#else /* ELF */
# define LIBNAMEA(NAME, API)    "lib" #NAME ".so." #API
# define LIBNAMEW(NAME, API)   L"lib" #NAME ".so." #API
#endif


/* Unicode/ANSI default macros */
#if defined(_WIN32) && defined(_UNICODE) && !defined(__CYGWIN__) && !defined(__MSYS__)
# define LIBEXT              LIBEXTW
# define LIBNAME(NAME, API)  LIBNAMEW(NAME, API)
#else
# define LIBEXT              LIBEXTA
# define LIBNAME(NAME, API)  LIBNAMEA(NAME, API)
#endif
