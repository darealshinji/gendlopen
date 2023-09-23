#if defined(_WIN32) && !defined(GDO_USE_DLOPEN)
    #define GDO_WINAPI
#endif


#ifdef GDO_WINAPI
    #include <windows.h>
    #include <tchar.h>
#else
    #ifndef _GNU_SOURCE
        #define _GNU_SOURCE
    #endif
    #include <link.h>
    #include <dlfcn.h>
#endif


/* helper macros for libray file extension and names */

/* Windows;
 * Make sure to exclude winegcc by checking for `__ELF__`! */
#if defined(_WIN32) && !defined(__ELF__)
    #define GDO_LIBEXT          "dll"
    #define GDO_LIB(NAME, API)  "lib" #NAME "-" #API ".dll"

/* Darwin (macOS, iOS) */
#elif defined(__APPLE__)
    #define GDO_LIBEXT          "dylib"
    #define GDO_LIB(NAME, API)  "lib" #NAME "." #API ".dylib"

/* IBM AIX;
 * After looking up some manuals it seems tht shared object files (.o)
 * and even whole shared libraries (.so) are stored in archive files (.a)
 * and can be loaded from there with dlopen().
 * Loading .so files directly is apparently possible too
 * but by default .a files are treated as shared libraries. */
#elif defined(_AIX)
    #define GDO_LIBEXT          "a"
    #define GDO_LIB(NAME, API)  "lib" #NAME ".a"

/* ELF systems */
#else
    #define GDO_LIBEXT          "so"
    #define GDO_LIB(NAME, API)  "lib" #NAME ".so." #API
#endif
