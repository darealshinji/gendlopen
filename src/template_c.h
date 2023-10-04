/***

******************
*   gdo_char_t   *
******************

If compiling for win32 and `_UNICODE` is defined and `GDO_USE_DLOPEN` is NOT defined
`gdo_char_t` will become `WCHAR`.

Otherwise `gdo_char_t` will become `char`.


**************************
*   Functions provided   *
**************************

bool               gdo_load_lib ();
bool               gdo_load_lib_args (const gdo_char_t *filename, int flags, bool new_namespace);
bool               gdo_load_symbols ();
bool               gdo_free_lib ();
bool               gdo_lib_is_loaded ();
const gdo_char_t * gdo_last_error ();
const gdo_char_t * gdo_lib_origin ();


bool gdo_load_lib ();

    Load the library specified by the macro GDO_DEFAULT_LIB using default flags.
    This function is not available if GDO_DEFAULT_LIB was not defined.


bool gdo_load_lib_args (const gdo_char_t *filename, int flags, bool new_namespace);

    Load the library; `filename' and `flags' are passed to the underlying library
    loading functions.

    If `new_namespace' is true the library will be loaded into a new namespace.
    This is done using dlmopen() with the LM_ID_NEWLM argument.
    If the win32 API is used or the macro GDO_NO_DLMOPEN is defined this argument
    is ignored.


bool gdo_load_symbols();

    Load the symbols. This function can safely be called multiple times.


bool gdo_free_lib();

    Free/release library handle.


bool gdo_lib_is_loaded();

    Returns true if the library was successfully loaded.


const gdo_char_t *gdo_last_error();

    Returns a pointer to the error message buffer with the last saved error string.
    This buffer can be empty and is in fact cleared when no error occured in a function.
    This function doesn't return a null pointer.


const gdo_char_t *gdo_lib_origin();

    Return the full library path.
    On error or if no library was loaded NULL is returned.



****************************************************
* The following options may be set through macros: *
****************************************************

GDO_USE_DLOPEN
    If defined use `dlopen()' API on win32 targets.
    On other targets `dlopen()' is always used.

GDO_NO_DLMOPEN
    If defined `dlmopen()` will never be used.
    See the function `gdo_load_lib_args()` for details.

GDO_STATIC
    If defined static linkage (the `static' keyword) is used for all
    functions.

GDO_DEFAULT_LIB
    Set a default library name through this macro (including double quote
    marks). This macro must be defined if you want to set GDO_USE_WRAPPER
    or if you want to use the `gdo_load_lib()' function.

GDO_USE_WRAPPER
    Define this macro if you want to use auto-loading wrapper functions.
    This means you don't need to explicitly call library load functions.
    It requires GDO_DEFAULT_LIB to be defined.
    If an error occures during loading these functions throw an error message
    and call `exit(1)'!

GDO_ATEXIT
    If defined the library handle will automatically be released on program exit.



*****************
* Helper macros *
*****************

GDO_DEFAULT_FLAGS
    Default flags for `dlopen()' or `LoadLibraryEx()'

GDO_LIB(NAME, API)
    Convenience macro to create versioned library names for DLLs, dylibs and DSOs,
    including double quote marks.
    GDO_LIB(z,1) for example will become "libz-1.dll", "libz.1.dylib" or "libz.so.1".

GDO_LIBEXT
    Shared library file extension without dot ("dll", "dylib" or "so").
    Useful i.e. on plugins.

***/

#include <stdbool.h>


GDO_COMMON


/* default flags */
#ifdef GDO_WINAPI
    #define GDO_DEFAULT_FLAGS  0
#else
    #define GDO_DEFAULT_FLAGS  RTLD_LAZY
#endif


/* static linkage */
#ifdef GDO_STATIC
    #define GDO_LINKAGE  static
#else
    #define GDO_LINKAGE  /**/
#endif


/* char / WCHAR */
#if defined(GDO_WINAPI) && defined(_UNICODE)
typedef WCHAR gdo_char_t;
#else
typedef char gdo_char_t;
#endif


#ifdef GDO_DEFAULT_LIB
GDO_LINKAGE bool gdo_load_lib();
#endif
GDO_LINKAGE bool gdo_load_lib_args(const gdo_char_t *filename, int flags, bool new_namespace);
GDO_LINKAGE bool gdo_load_symbols();
GDO_LINKAGE bool gdo_free_lib();
GDO_LINKAGE bool gdo_lib_is_loaded();
GDO_LINKAGE const gdo_char_t *gdo_last_error();
GDO_LINKAGE const gdo_char_t *gdo_lib_origin();


GDO_TYPEDEFS


/****************************************************************************/
/* aliases */
/****************************************************************************/
#if defined(GDO_DEFAULT_LIB) && defined(GDO_USE_WRAPPER)

#ifndef GDO_ATEXIT
#define GDO_ATEXIT
#endif

#define GDO_SYMBOL _gdo_wrap_GDO_SYMBOL

#else

#define GDO_SYMBOL _gdo_GDO_SYMBOL_ptr_

#endif

#define GDO_OBJ_SYMBOL *_gdo_GDO_OBJ_SYMBOL_ptr_
/****************************************************************************/