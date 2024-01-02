#if defined(_WIN32) && !defined(_$USE_DLOPEN)
    #define _$WINAPI
#endif


#ifdef _$WINAPI
    #include <windows.h>
    #include <tchar.h>
#else
    #ifdef _GNU_SOURCE
    #undef _GNU_SOURCE
    #endif
    #define _GNU_SOURCE
    #include <link.h>
    #include <dlfcn.h>
#endif
#ifndef __cplusplus
    #include <stdbool.h>
#endif


/* helper macros for libray file extension and names */

/* Windows */
#if defined(_WIN32)
    /* ANSI */
    #define _$LIBEXTA             "dll"
    #define _$LIBA(NAME, API)     "lib" #NAME "-" #API ".dll"

    /* WCHAR */
    #define _$LIBEXTW             L"dll"
    #define _$LIBW(NAME, API)     L"lib" #NAME "-" #API ".dll"

    #ifdef _UNICODE
        #define _$LIBEXT          _$LIBEXTW
        #define _$LIB(NAME, API)  _$LIBW(NAME, API)
    #else
        #define _$LIBEXT          _$LIBEXTA
        #define _$LIB(NAME, API)  _$LIBA(NAME, API)
    #endif

/* Darwin (macOS, iOS) */
#elif defined(__APPLE__)
    #define _$LIBEXT              "dylib"
    #define _$LIB(NAME, API)      "lib" #NAME "." #API ".dylib"

/* IBM AIX;
 * After looking up some manuals it seems tht shared object files (.o)
 * and even whole shared libraries (.so) are stored in archive files (.a)
 * and can be loaded from there with dlopen().
 * Loading .so files directly is apparently possible too
 * but by default .a files are treated as shared libraries. */
#elif defined(_AIX)
    #define _$LIBEXT              "a"
    #define _$LIB(NAME, API)      "lib" #NAME ".a"

/* ELF systems */
#else
    #define _$LIBEXT              "so"
    #define _$LIB(NAME, API)      "lib" #NAME ".so." #API
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
