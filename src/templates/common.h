/* whether to use WinAPI */
#ifdef _WIN32
# define GDO_OS_WIN32
#endif
#if defined(GDO_OS_WIN32) && !defined(GDO_USE_DLOPEN)
# define GDO_WINAPI
#endif

/* default headers to include */
#ifdef GDO_WINAPI
# include <windows.h>
#else
# include <link.h>
# include <dlfcn.h>
#endif
#ifndef __cplusplus
# include <stdbool.h>
#endif

/* default library name */
#ifndef GDO_DEFAULT_LIB
# if defined(GDO_DEFAULT_LIBW) && defined(GDO_WINAPI) && defined(_UNICODE)
#  define GDO_DEFAULT_LIB GDO_DEFAULT_LIBW
# elif defined(GDO_DEFAULT_LIBA)
#  define GDO_DEFAULT_LIB GDO_DEFAULT_LIBA
# endif
#endif

/* whether to use dlinfo(3) */
#if defined(HAVE_DLINFO) && !defined(GDO_WINAPI)
# define GDO_HAVE_DLINFO
#endif

/* whether to use dlmopen(3) */
#if defined(HAVE_DLMOPEN) && !defined(GDO_WINAPI)
# define GDO_HAVE_DLMOPEN
#endif

/* dlopen(3) flags for compatibility with LoadLibrary() */
/* taken from different implementations of dlfcn.h */
#ifndef RTLD_LAZY
#define RTLD_LAZY 0
#endif
#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif
#ifndef RTLD_MODEMASK
#define RTLD_MODEMASK 0  /* FreeBSD, DragonFlyBSD */
#endif
#ifndef RTLD_BINDING_MASK
#define RTLD_BINDING_MASK 0  /* glibc */
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
#define RTLD_DEEPBIND 0  /* glibc, FreeBSD */
#endif
#ifndef RTLD_TRACE
#define RTLD_TRACE 0  /* FreeBSD, OpenBSD, DragonFlyBSD */
#endif
#ifndef RTLD_GROUP
#define RTLD_GROUP 0  /* Solaris */
#endif
#ifndef RTLD_PARENT
#define RTLD_PARENT 0  /* Solaris */
#endif
#ifndef RTLD_WORLD
#define RTLD_WORLD 0  /* Solaris */
#endif
#ifndef RTLD_FIRST
#define RTLD_FIRST 0  /* macOS, Solaris */
#endif
#ifndef RTLD_MEMBER
#define RTLD_MEMBER 0  /* AIX */
#endif
#ifndef RTLD_NOAUTODEFER
#define RTLD_NOAUTODEFER 0  /* AIX */
#endif
#ifndef DL_LAZY
#define DL_LAZY RTLD_LAZY  /* NetBSD, OpenBSD */
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
# define GDO_VISIBILITY
#endif

/* default flags */
#ifndef GDO_DEFAULT_FLAGS
# ifdef GDO_WINAPI
#  define GDO_DEFAULT_FLAGS  0
# else
#  define GDO_DEFAULT_FLAGS  (RTLD_LAZY | RTLD_MEMBER)
# endif
#endif
