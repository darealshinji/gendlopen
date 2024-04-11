/* whether to use WinAPI */
#ifdef _WIN32
    #define GDO_OS_WIN32
#endif
#if defined(GDO_OS_WIN32) && !defined(GDO_USE_DLOPEN)
    #define GDO_WINAPI
#endif

/* default headers to include */
#ifdef GDO_WINAPI
    #include <windows.h>
#else
    #include <link.h>
    #include <dlfcn.h>
#endif
#ifndef __cplusplus
    #include <stdbool.h>
#endif

/* whether to use dlmopen(3) GNU extension */
#if defined(HAVE_DLMOPEN) || (defined(_GNU_SOURCE) && defined(__GLIBC__))
    #define GDO_HAVE_DLMOPEN
#endif
#if defined(GDO_HAVE_DLMOPEN) && defined(GDO_NO_DLMOPEN)
    #undef GDO_HAVE_DLMOPEN
#endif

/* dlopen(3) flags for compatibility with LoadLibrary() */
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

/* LoadLibrary() flags for compatibility with dlopen() */
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

/* symbol visibility */
#ifndef GDO_VISIBILITY
    #define GDO_VISIBILITY  /**/
#endif

/* default flags */
#ifndef GDO_DEFAULT_FLAGS
    #ifdef GDO_WINAPI
        #define GDO_DEFAULT_FLAGS  0
    #else
        #define GDO_DEFAULT_FLAGS  RTLD_LAZY
    #endif
#endif
