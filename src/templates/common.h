/*****************************************************************************/
/*                         common macros and headers                         */
/*****************************************************************************/


/***

*** flags ***

GDO_USE_DLOPEN
    If defined use `dlopen()' API on win32 targets.
    On other targets `dlopen()' is always used.

GDO_STATIC
    If defined `static inline' linkage is used for all functions.

GDO_WRAP_FUNCTIONS
    Use actual function wrappers instead of a name alias.

GDO_WRAP_IS_VISIBLE
    Declare function wrappers as regular visible functions instead of inlining them.

GDO_ENABLE_AUTOLOAD
    Define this macro if you want to use auto-loading wrapper functions.
    It requires GDO_DEFAULT_LIB to be defined.

GDO_ENABLE_AUTOLOAD_LAZY
    Same as GDO_ENABLE_AUTOLOAD but only the requested symbol is loaded when its
    wrapper function is called instead of all symbols.

GDO_USE_MESSAGE_BOX
    Windows only: if GDO_ENABLE_AUTOLOAD was activated this will enable
    error messages from auto-loading to be displayed in MessageBox windows.

GDO_DISABLE_ALIASING
    Don't use preprocessor macros to alias symbol names.

GDO_DISABLE_WARNINGS
    Mute diagnostic warnings.


*** settings ***

GDO_DEFAULT_FLAGS
    Override the default flags to use when loading a library.

GDO_DEFAULT_LIB
    Set a default library name through this macro.

GDO_VISIBILITY
    Set the symbol visibility of wrapped functions.


*** hooks ***

GDO_HOOK_<function>(...)
    Define a hook macro that will be inserted into a wrap function.
    The hook is placed before the actual function call.
    If you want to call the function inside the macro you must do so using the GDO_ALIAS_* prefix.
    Parameter names are taken from the function prototype declarations (or it's "a1, a2, a3" and so on
    if the header was created with `-param=create').

    For example if a function declaration is `int sum_of_a_and_b(int val_a, int val_b)':

    #define GDO_HOOK_sum_of_a_and_b(...) \
      printf("debug: the sum of %d and %d is %d\n", \
        val_a, val_b, GDO_ALIAS_sum_of_a_and_b(__VA_ARGS__));

***/


#if defined(_WIN32) && !defined(GDO_USE_DLOPEN)
# define GDO_WINAPI
#endif

#ifdef _WIN32
# include <windows.h>
#else
/* __GLIBC__ is defined in <features.h> which isn't present on all
 * systems, so we include it indirectly */
# include <limits.h>
#endif

#ifndef GDO_WINAPI
# include <dlfcn.h>
#endif


#if defined(__GLIBC__) && !defined(_GNU_SOURCE)

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


/* dlinfo(3); n/a on Windows, macOS, OpenBSD and Haiku */
#if !defined(_WIN32) && \
    !defined(__APPLE__) && \
    !defined(__OpenBSD__) && \
    !defined(__HAIKU__) && \
    !defined(GDO_HAVE_DLINFO)
# define GDO_HAVE_DLINFO
#endif

#ifdef GDO_HAVE_DLINFO
# include <link.h>
#endif


/* dlmopen(3); only on Glibc (or compatible C libraries) and Solaris/IllumOS */
#if (defined(__GLIBC__) || defined(__sun)) && \
    !defined(GDO_HAVE_DLMOPEN)
# define GDO_HAVE_DLMOPEN
#endif


/* attributes */
#ifdef __GNUC__
# define GDO_ATTR(x)  __attribute__ ((x))
#else
# define GDO_ATTR(x)  /**/
#endif


/* symbol visibility for wrapped functions */
#ifndef GDO_VISIBILITY
# define GDO_VISIBILITY /**/
#endif


/* check for __builtin_va_arg_pack() */
#if !defined(GDO_HAS_BUILTIN_VA_ARG_PACK) && defined(__has_builtin)
# if __has_builtin(__builtin_va_arg_pack)
#  define GDO_HAS_BUILTIN_VA_ARG_PACK
# endif
#endif


/* diagnostic #pragma warnings */
#define _GDO_XSTR(x)              #x
#define _GDO_STR(x)               _GDO_XSTR(x)
#ifdef __GNUC__
# define GDO_PRAGMA_WARNING(MSG)  _Pragma( _GDO_XSTR(GCC warning MSG) )
#elif defined(_MSC_VER)
# define GDO_PRAGMA_WARNING(MSG)  __pragma( message(__FILE__ ": " MSG) )
#else
# define GDO_PRAGMA_WARNING(...)  /**/
#endif


/* default library name */

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


/* use the default autoload macro for most checks */
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

