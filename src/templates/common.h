#if defined(_WIN32) && !defined(GDO_USE_DLOPEN)
    #define GDO_WINAPI
#endif


#ifdef GDO_WINAPI
    #include <windows.h>
#else
    #if defined(__GLIBC__) && !defined(_GNU_SOURCE)
        #define _GNU_SOURCE 1
    #endif
    #include <link.h>
    #include <dlfcn.h>
#endif
#ifndef __cplusplus
    #include <stdbool.h>
#endif


/* set the symbol visibility manually */
#ifndef GDO_VISIBILITY
    #define GDO_VISIBILITY  /**/
#endif


/* helper macros for libray file extension and names */

/* Windows */
#if defined(_WIN32)
    /* ANSI */
    #define GDO_LIBEXTA             "dll"
    #define GDO_LIBA(NAME, API)     "lib" #NAME "-" #API ".dll"

    /* WCHAR */
    #define GDO_LIBEXTW             L"dll"
    #define GDO_LIBW(NAME, API)     L"lib" #NAME "-" #API ".dll"

    #ifdef _UNICODE
        #define GDO_LIBEXT          GDO_LIBEXTW
        #define GDO_LIB(NAME, API)  GDO_LIBW(NAME, API)
    #else
        #define GDO_LIBEXT          GDO_LIBEXTA
        #define GDO_LIB(NAME, API)  GDO_LIBA(NAME, API)
    #endif

/* Darwin (macOS, iOS) */
#elif defined(__APPLE__)
    #define GDO_LIBEXT              "dylib"
    #define GDO_LIB(NAME, API)      "lib" #NAME "." #API ".dylib"

/* IBM AIX;
 * After looking up some manuals it seems that shared object files (.o)
 * and even whole shared libraries (.so) are stored in archive files (.a)
 * and can be loaded from there with dlopen().
 * Loading .so files directly is apparently possible too
 * but by default .a files are treated as shared libraries. */
#elif defined(_AIX)
    #define GDO_LIBEXT              "a"
    #define GDO_LIB(NAME, API)      "lib" #NAME ".a"

/* ELF systems */
#else
    #define GDO_LIBEXT              "so"
    #define GDO_LIB(NAME, API)      "lib" #NAME ".so." #API
#endif

#ifndef GDO_LIBEXTA
#define GDO_LIBEXTA GDO_LIBEXT
#endif


/* default flags */
#ifndef GDO_DEFAULT_FLAGS
    #ifdef GDO_WINAPI
        #define GDO_DEFAULT_FLAGS  0
    #else
        #define GDO_DEFAULT_FLAGS  RTLD_LAZY
    #endif
#endif


/* dlopen(3) */
#ifndef RTLD_LAZY
#define RTLD_LAZY 0
#endif
#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
#ifndef RTLD_LOCAL
#define RTLD_LOCAL 0
#endif
#ifndef RTLD_NODELETE
#define RTLD_NODELETE 0
#endif
#ifndef RTLD_NOLOAD
#define RTLD_NOLOAD 0
#endif
#ifndef RTLD_DEEPBIND
#define RTLD_DEEPBIND 0
#endif

/* https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw */
#ifndef DONT_RESOLVE_DLL_REFERENCES
#define DONT_RESOLVE_DLL_REFERENCES 0
#endif
#ifndef LOAD_IGNORE_CODE_AUTHZ_LEVEL
#define LOAD_IGNORE_CODE_AUTHZ_LEVEL 0
#endif
#ifndef LOAD_LIBRARY_AS_DATAFILE
#define LOAD_LIBRARY_AS_DATAFILE 0
#endif
#ifndef LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE
#define LOAD_LIBRARY_AS_DATAFILE_EXCLUSIVE 0
#endif
#ifndef LOAD_LIBRARY_AS_IMAGE_RESOURCE
#define LOAD_LIBRARY_AS_IMAGE_RESOURCE 0
#endif
#ifndef LOAD_LIBRARY_SEARCH_APPLICATION_DIR
#define LOAD_LIBRARY_SEARCH_APPLICATION_DIR 0
#endif
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS 0
#endif
#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
#define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR 0
#endif
#ifndef LOAD_LIBRARY_SEARCH_SYSTEM32
#define LOAD_LIBRARY_SEARCH_SYSTEM32 0
#endif
#ifndef LOAD_LIBRARY_SEARCH_USER_DIRS
#define LOAD_LIBRARY_SEARCH_USER_DIRS 0
#endif
#ifndef LOAD_WITH_ALTERED_SEARCH_PATH
#define LOAD_WITH_ALTERED_SEARCH_PATH 0
#endif
#ifndef LOAD_LIBRARY_REQUIRE_SIGNED_TARGET
#define LOAD_LIBRARY_REQUIRE_SIGNED_TARGET 0
#endif
#ifndef LOAD_LIBRARY_SAFE_CURRENT_DIRS
#define LOAD_LIBRARY_SAFE_CURRENT_DIRS 0
#endif
