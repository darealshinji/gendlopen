/*****************************************************************************/
/*                        common macros and headers                          */
/*****************************************************************************/


/***

*** flags ***

GDO_USE_DLOPEN
    If defined use `dlopen()' API on win32 targets.
    On other targets `dlopen()' is always used.

GDO_STATIC
    If defined `static inline' linkage is used for all functions (C only).

GDO_WRAP_FUNCTIONS
    Use actual function wrappers instead of a name alias.

GDO_ENABLE_AUTOLOAD
    Define this macro if you want to use auto-loading wrapper functions.
    It requires GDO_DEFAULT_LIB to be defined.

GDO_ENABLE_AUTOLOAD_LAZY
    Same as GDO_ENABLE_AUTOLOAD but only the requested symbol is loaded when its
    wrapper function is called instead of all symbols.

GDO_USE_MESSAGE_BOX
    Windows only: if GDO_ENABLE_AUTOLOAD was activated this will enable
    error messages from auto-loading to be displayed in MessageBox windows.

GDO_CONVERT_FILENAME
    Windows only: don't use LoadLibraryExA(), instead convert the filename
    to wchar_t and use LoadLibraryExW() (C only).

GDO_DISABLE_ALIASING
    Don't use preprocessor macros to alias symbol names.

GDO_DISABLE_WARNINGS
    Mute diagnostic warnings.


*** settings ***

GDO_DEFAULT_FLAGS
    Override the default flags to use when loading a library.

GDO_DEFAULT_LIB
    Set a default library name through this macro.

GDO_WRAP_VISIBILITY
    Set the symbol visibility of wrapped functions. By default wrapped functions
    are not visible and inlined.


*** hooks ***

GDO_HOOK_<function>(...)
    Define a hook macro that will be inserted into a wrap function.
    The hook is placed before the actual function call.
    If you want to call the function inside the macro you must do so using the GDO_RAWPTR_* prefix.
    Parameter names are taken from the function prototype declarations (or it's "a1, a2, a3" and so on
    if the header was created with `-param=create').

    For example if a function declaration is `int sum_of_a_and_b(int val_a, int val_b)':

    #define GDO_HOOK_sum_of_a_and_b(...) \
      printf("debug: the sum of %d and %d is %d\n", \
        val_a, val_b, GDO_RAWPTR_sum_of_a_and_b(__VA_ARGS__))

    Note: when using GCC variable length argument functions are implemented using
    inline functions, so inlining must be enabled or else linking will later fail.
    On other compilers these functions are implemented using preprocessor macros
    which means there will be some limitations when using hook macros.
***/


#ifndef __cplusplus
# include <stdbool.h>
#endif

#ifdef _WIN32
# ifdef GDO_USE_DLOPEN
#  define GDO_DLFCN_WIN32
# else
#  define GDO_WINAPI
# endif
# include <windows.h>
# include <wchar.h>
#else
# ifdef HAVE_FEATURES_H
#  include <features.h> /* defines macros to identify C library */
# else
#  include <limits.h> /* includes <features.h> indirectly if present */
# endif
#endif

#ifndef GDO_WINAPI
# include <dlfcn.h>
#endif


#ifdef GDO_WINAPI
typedef HMODULE gdo_hmod_t;
#else
typedef void *gdo_hmod_t;
#endif


#define GDO_INLINE  static inline


#ifdef _WIN32
# define GDO_BUFLEN (32*1024) /* https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation */
#else
# define GDO_BUFLEN (8*1024)  /* 2x Linux PATH_MAX */
#endif


#ifdef _AIX
# define GDO_AIX_LOADQUERY_BUFLEN (16*1024)  /* loadquery() buffer length */
#endif


/* flags to use on FormatMessage() */
#ifdef GDO_WINAPI
# define GDO_FORMAT_MESSAGE_FLAGS \
    (FORMAT_MESSAGE_ALLOCATE_BUFFER | \
     FORMAT_MESSAGE_FROM_SYSTEM | \
     FORMAT_MESSAGE_IGNORE_INSERTS | \
     FORMAT_MESSAGE_MAX_WIDTH_MASK)
#endif


/* dladdr(3); n/a on Windows (WINAPI) and AIX */
#if !defined(GDO_WINAPI) && \
    !defined(_AIX)
# define GDO_HAVE_DLADDR
#endif

#ifdef GDO_HAVE_DLADDR
# if !defined(_GNU_SOURCE) && \
     (defined(__GLIBC__) || \
      defined(__UCLIBC__) || \
      defined(__dietlibc__))
#  define GDO_NEED_DLADDR_PROTO
# elif !defined(_NETBSD_SOURCE) && (defined(__NetBSD__) || defined(__minix))
#  define GDO_NEED_DLADDR_PROTO
# endif
#endif

#if !defined(DLADDR_PROTOTYPE) && defined(GDO_NEED_DLADDR_PROTO)
# define DLADDR_PROTOTYPE
typedef struct {
  const char *dli_fname;
  void       *dli_fbase;
  const char *dli_sname;
  void       *dli_saddr;
} Dl_info;
extern int dladdr(const void *addr, Dl_info *info);
#endif


/* dlinfo(3); systems that support dlinfo() with RTLD_DI_LINKMAP request */
#if !defined(GDO_HAVE_DLINFO) && \
    (defined(__GLIBC__) || \
     defined(__FreeBSD__) || \
     defined(__NetBSD__) || \
     defined(__DragonFly__) || \
     defined(__minix) || \
     defined(__sun))
# define GDO_HAVE_DLINFO
#endif

#ifdef GDO_HAVE_DLINFO
# include <link.h> /* link map */
#endif

#ifdef GDO_HAVE_DLINFO
# if !defined(_GNU_SOURCE) && defined(__GLIBC__)
#  define GDO_NEED_DLINFO_PROTO
# elif !defined(_NETBSD_SOURCE) && (defined(__NetBSD__) || defined(__minix))
#  define GDO_NEED_DLINFO_PROTO
# endif
#endif

#if !defined(DLINFO_PROTOTYPE) && defined(GDO_NEED_DLINFO_PROTO)
# define DLINFO_PROTOTYPE
# ifdef __GLIBC__
#  define RTLD_DI_LINKMAP 2  /* Glibc */
# else
#  define RTLD_DI_LINKMAP 3  /* NetBSD libc */
# endif
extern int dlinfo(void *handle, int request, void *info);
#endif


/* dlmopen(3); only on Glibc and Solaris/IllumOS */
#if !defined(GDO_HAVE_DLMOPEN) && \
    (defined(__GLIBC__) || \
     defined(__sun))
# define GDO_HAVE_DLMOPEN
#endif

#ifdef GDO_HAVE_DLMOPEN
# if !defined(_GNU_SOURCE) && defined(__GLIBC__)
#  define GDO_NEED_DLMOPEN_PROTO
# endif
#endif

#if !defined(DLMOPEN_PROTOTYPE) && defined(GDO_NEED_DLMOPEN_PROTO)
# define DLMOPEN_PROTOTYPE
# define LM_ID_NEWLM -1  /* new namespace */
typedef long int Lmid_t;
extern void *dlmopen(Lmid_t lmid, const char *path, int flags);
#endif


/* GCC specific attributes */
#ifdef __GNUC__
# define GDO_GCC_ATTRIBUTE(x)  __attribute__ ((x))
#else
# define GDO_GCC_ATTRIBUTE(x)  /**/
#endif


/* set visibility of wrapped functions */
#ifdef GDO_WRAP_VISIBILITY
/* visible as regular functions */
# define GDO_WRAP_DECL  GDO_WRAP_VISIBILITY
# define GDO_WRAP(x)    x
# else
/* declare as prefixed inline functions by default */
# define GDO_WRAP_DECL  static inline
# define GDO_WRAP(x)    GDO_WRAP_##x
#endif


/* diagnostic #pragma warnings */
#ifdef __GNUC__
# define _GDO_PRAGMA(x)    _Pragma(#x)
# define GDO_WARNING(MSG)  _GDO_PRAGMA(GCC warning MSG)
#elif defined(_MSC_VER)
# define GDO_WARNING(MSG)  __pragma( message(__FILE__ ": " MSG) )
#else
# define GDO_WARNING(...)  /**/
#endif


/* default library name */

#if defined(GDO_WINAPI) && defined(_UNICODE)
# define _GDO_TARGET_WIDECHAR
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
# if defined(GDO_DEFAULT_LIBA) && !defined(_GDO_TARGET_WIDECHAR)
#  define GDO_DEFAULT_LIB  GDO_DEFAULT_LIBA
# elif defined(GDO_DEFAULT_LIBW) && defined(_GDO_TARGET_WIDECHAR)
#  define GDO_DEFAULT_LIB  GDO_DEFAULT_LIBW
# endif
#endif

/* GDO_DEFAULT_LIBA or GDO_DEFAULT_LIBW ?= GDO_DEFAULT_LIB */
#ifdef GDO_DEFAULT_LIB
# if !defined(GDO_DEFAULT_LIBA) && !defined(_GDO_TARGET_WIDECHAR)
#  define GDO_DEFAULT_LIBA  GDO_DEFAULT_LIB
# endif
# if !defined(GDO_DEFAULT_LIBW) && defined(_GDO_TARGET_WIDECHAR)
#  define GDO_DEFAULT_LIBW  GDO_DEFAULT_LIB
# endif
#endif


/* dlopen(3) flags for compatibility with LoadLibrary()
 * taken from different implementations of dlfcn.h */
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


/* LoadLibrary() flags for compatibility with dlopen()
 * https://learn.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-loadlibraryexw */
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
#if !defined(GDO_DEFAULT_FLAGS)
# ifdef GDO_WINAPI
#  define GDO_DEFAULT_FLAGS 0
# else
#  define GDO_DEFAULT_FLAGS (RTLD_LAZY | RTLD_MEMBER)
# endif
#endif


/* always enable autoload if "lazy autoload" was enabled */
#if defined(GDO_ENABLE_AUTOLOAD_LAZY) && !defined(GDO_ENABLE_AUTOLOAD)
# define GDO_ENABLE_AUTOLOAD
#endif


/* whether wrapped functions can be used */
%PARAM_SKIP_REMOVE_BEGIN%
#if defined(GDO_ENABLE_AUTOLOAD) && !defined(GDO_DEFAULT_LIB)
# error Auto-loading functions require "GDO_DEFAULT_LIB" to be set
#endif
%PARAM_SKIP_USE_BEGIN%
#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)
# error Auto-loading and wrapped functions cannot be used because "-param=skip" was set
#endif
%PARAM_SKIP_END%

