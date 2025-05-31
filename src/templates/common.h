/*****************************************************************************/
/*                         common macros and headers                         */
/*****************************************************************************/

/* whether to use WinAPI or dlfcn */
#if defined(_WIN32) && !defined(GDO_USE_DLOPEN)
# define GDO_WINAPI
#endif

#ifdef _WIN32
# include <windows.h>
#else
/* <features.h> is a Glibc header that defines __GLIBC__
 * and will be automatically included with a system header if present */
# include <stdio.h>
#endif

#ifndef GDO_WINAPI
# include <dlfcn.h>
#endif


/* attributes */
#ifdef __GNUC__
# define GDO_ATTR(x)  __attribute__ ((x))
#else
# define GDO_ATTR(x)  /**/
#endif


#if defined(__GLIBC__) && !defined(_GNU_SOURCE)

/* disable GNU extensions */
# ifndef GDO_DISABLE_DLINFO
# define GDO_DISABLE_DLINFO
# endif
# ifndef GDO_DISABLE_DLMOPEN
# define GDO_DISABLE_DLMOPEN
# endif

/* Provide a declaration for `dladdr(3)'.
 * This is simply for convenience so that it's not required
 * to always set _GNU_SOURCE on Linux. */

typedef struct {
  const char *dli_fname;
  void       *dli_fbase;
  const char *dli_sname;
  void       *dli_saddr;
} _GDO_Dl_info;

# ifdef __cplusplus
extern "C" int dladdr(const void *, _GDO_Dl_info *);
# else
extern     int dladdr(const void *, _GDO_Dl_info *);
# endif

#elif !defined(GDO_WINAPI)

/* typename in POSIX-2024 is `Dl_info_t' but systems that
 * conform to it usually provide `Dl_info' too */
typedef Dl_info _GDO_Dl_info;

#endif //__GLIBC__ && !_GNU_SOURCE


/* symbol visibility, i.e. __declspec(dllexport)
 * or __attribute__((visibility("default"))) */
#ifndef GDO_VISIBILITY
# define GDO_VISIBILITY
#endif


/* set default library name values */

#if defined(GDO_WINAPI) && defined(_UNICODE)
# define _GDO_TARGET_WIDECHAR
#else
# define _GDO_TARGET_UTF8
#endif

/* GDO_DEFAULT_LIBA ?= GDO_HARDCODED_DEFAULT_LIBA */
#if !defined(GDO_DEFAULT_LIBA) && defined(GDO_HARDCODED_DEFAULT_LIBA)
# define GDO_DEFAULT_LIBA  GDO_HARDCODED_DEFAULT_LIBA
#endif

/* GDO_DEFAULT_LIBW ?= GDO_HARDCODED_DEFAULT_LIBW */
#if !defined(GDO_DEFAULT_LIBW) && defined(GDO_HARDCODED_DEFAULT_LIBW)
# define GDO_DEFAULT_LIBW  GDO_HARDCODED_DEFAULT_LIBW
#endif

/* GDO_DEFAULT_LIB ?= GDO_DEFAULT_LIBA or GDO_DEFAULT_LIBW */
#ifndef GDO_DEFAULT_LIB
# if defined(GDO_DEFAULT_LIBA) && defined(_GDO_TARGET_UTF8)
#  define GDO_DEFAULT_LIB  GDO_DEFAULT_LIBA
# elif defined(GDO_DEFAULT_LIBW) && defined(_GDO_TARGET_WIDECHAR)
#  define GDO_DEFAULT_LIB  GDO_DEFAULT_LIBW
# endif
#endif

/* GDO_DEFAULT_LIBA ?= GDO_DEFAULT_LIB */
#if !defined(GDO_DEFAULT_LIBA) && defined(GDO_DEFAULT_LIB) && defined(_GDO_TARGET_UTF8)
# define GDO_DEFAULT_LIBA  GDO_DEFAULT_LIB
#endif

/* GDO_DEFAULT_LIBW ?= GDO_DEFAULT_LIB */
#if !defined(GDO_DEFAULT_LIBW) && defined(GDO_DEFAULT_LIB) && defined(_GDO_TARGET_WIDECHAR)
# define GDO_DEFAULT_LIBW  GDO_DEFAULT_LIB
#endif


/* dlinfo(3); n/a on Windows, macOS, OpenBSD and Haiku */
#if defined(_WIN32) || \
    defined(__APPLE__) || \
    defined(__OpenBSD__) || \
    defined(__HAIKU__)
# define _GDO_TARGET_NO_DLINFO
#endif

#ifdef GDO_DISABLE_DLINFO
# ifdef GDO_HAVE_DLINFO
# undef GDO_HAVE_DLINFO
# endif
#endif

#if !defined(GDO_DISABLE_DLINFO) && \
    !defined(_GDO_TARGET_NO_DLINFO) && \
    !defined(GDO_HAVE_DLINFO)
# define GDO_HAVE_DLINFO
#endif

#ifdef GDO_HAVE_DLINFO
# include <link.h>
#endif


/* dlmopen(3); only on Glibc and Solaris/IllumOS */
#ifdef GDO_DISABLE_DLMOPEN
# ifdef GDO_HAVE_DLMOPEN
# undef GDO_HAVE_DLMOPEN
# endif
#endif

#if !defined(GDO_DISABLE_DLMOPEN) && \
    (defined(__GLIBC__) || defined(__sun)) && \
    !defined(GDO_HAVE_DLMOPEN)
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
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif
#ifndef RTLD_LOCAL
#define RTLD_LOCAL 0
#endif

#ifndef RTLD_BINDING_MASK
#define RTLD_BINDING_MASK 0
#endif
#ifndef RTLD_CONFGEN
#define RTLD_CONFGEN 0
#endif
#ifndef RTLD_DEEPBIND
#define RTLD_DEEPBIND 0
#endif
#ifndef RTLD_FIRST
#define RTLD_FIRST 0
#endif
#ifndef RTLD_GROUP
#define RTLD_GROUP 0
#endif
#ifndef RTLD_MEMBER
#define RTLD_MEMBER 0
#endif
#ifndef RTLD_MODEMASK
#define RTLD_MODEMASK 0
#endif
#ifndef RTLD_NOAUTODEFER
#define RTLD_NOAUTODEFER 0
#endif
#ifndef RTLD_NODELETE
#define RTLD_NODELETE 0
#endif
#ifndef RTLD_NOLOAD
#define RTLD_NOLOAD 0
#endif
#ifndef RTLD_PARENT
#define RTLD_PARENT 0
#endif
#ifndef RTLD_TRACE
#define RTLD_TRACE 0
#endif
#ifndef RTLD_WORLD
#define RTLD_WORLD 0
#endif

#ifndef DL_LAZY
#define DL_LAZY RTLD_LAZY  /* compat */
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


/* default flags */
#ifndef GDO_DEFAULT_FLAGS
# ifdef GDO_WINAPI
#  define GDO_DEFAULT_FLAGS 0
# else
#  define GDO_DEFAULT_FLAGS (RTLD_LAZY | RTLD_MEMBER)
# endif
#endif


%PARAM_SKIP_REMOVE_BEGIN%
/* wrapped functions are enabled */
#if defined(GDO_ENABLE_AUTOLOAD) && !defined(GDO_DEFAULT_LIB)
# error You need to define GDO_DEFAULT_LIB if you want to make use of GDO_ENABLE_AUTOLOAD
#endif
#if defined(GDO_DELAYLOAD) && !defined(GDO_ENABLE_AUTOLOAD)
# error You need to define GDO_ENABLE_AUTOLOAD if you want to make use of GDO_DELAYLOAD
#endif
%PARAM_SKIP_USE_BEGIN%
/* wrapped functions are disabled */
#if defined(GDO_WRAP_FUNCTIONS)
# error "GDO_WRAP_FUNCTIONS" defined but wrapped functions were disabled with "-param=skip"
#endif
#if defined(GDO_ENABLE_AUTOLOAD)
# error "GDO_ENABLE_AUTOLOAD" defined but wrapped functions were disabled with "-param=skip"
#endif
%PARAM_SKIP_END%

