/* helper macro to quote filenames */
#ifndef QUOTE_STRING
    #define QUOTE_STRING(str)  #str
#endif


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
#if defined(_WIN32)
    #define LIBEXTA    "dll"
    #define LIBEXTW    L"dll"
#elif defined(__APPLE__)
    #define LIBEXTA    "dylib"
    #define LIBEXT     "dylib"
#elif defined(_AIX)
    #define LIBEXTA    "a"
    #define LIBEXT     "a"
#else /* ELF */
    #define LIBEXTA    "so"
    #define LIBEXT     "so"
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
#if defined(_WIN32)
    #define LIBNAMEA(NAME, API)    "lib" #NAME "-" #API ".dll"
    #define LIBNAMEW(NAME, API)    L"lib" #NAME "-" #API ".dll"
#elif defined(__APPLE__)
    #define LIBNAMEA(NAME, API)    "lib" #NAME "." #API ".dylib"
    #define LIBNAME(NAME, API)     "lib" #NAME "." #API ".dylib"
#elif defined(_AIX)
    #define LIBNAMEA(NAME, API)    "lib" #NAME ".a"
    #define LIBNAME(NAME, API)     "lib" #NAME ".a"
#else /* ELF */
    #define LIBNAMEA(NAME, API)    "lib" #NAME ".so." #API
    #define LIBNAME(NAME, API)     "lib" #NAME ".so." #API
#endif


/* Windows Unicode/ANSI default macros */
#ifdef _WIN32
    #ifdef _UNICODE
        #define LIBEXT                L"dll"
        #define LIBNAME(NAME, API)    L"lib" #NAME "-" #API ".dll"
    #else
        #define LIBEXT                "dll"
        #define LIBNAME(NAME, API)    "lib" #NAME "-" #API ".dll"
    #endif
#endif
