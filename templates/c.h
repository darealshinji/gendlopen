
/***

******************
*   gdo_char_t   *
******************

If compiling for win32 and `_UNICODE` is defined and `GDO_USE_DLOPEN` is NOT defined
`gdo_char_t` will become `wchar_t`.

Otherwise `gdo_char_t` will become `char`.


**************************
*   Functions provided   *
**************************

bool               gdo_load_lib ();
bool               gdo_load_lib_and_symbols ();
bool               gdo_load_lib_name (const gdo_char_t *filename);
bool               gdo_load_lib_args (const gdo_char_t *filename, int flags, bool new_namespace);
bool               gdo_load_symbols (bool ignore_errors);
bool               gdo_load_symbol (const char *symbol);
bool               gdo_free_lib ();
bool               gdo_lib_is_loaded ();
const gdo_char_t * gdo_last_error ();
gdo_char_t *       gdo_lib_origin ();


bool gdo_load_lib ();

    Load the library specified by the macro GDO_DEFAULT_LIB using default flags.
    This function is not available if GDO_DEFAULT_LIB was not defined.


bool gdo_load_lib_and_symbols ();

    Calls gdo_load_lib() and gdo_load_symbols().


bool gdo_load_lib_name (const gdo_char_t *filename);

    Load the library specified by `filename' using default flags.


bool gdo_load_lib_args (const gdo_char_t *filename, int flags, bool new_namespace);

    Load the library; `filename' and `flags' are passed to the underlying library
    loading functions.

    If `new_namespace' is true the library will be loaded into a new namespace.
    This is done using dlmopen() with the LM_ID_NEWLM argument.
    If the win32 API is used or the macro GDO_NO_DLMOPEN is defined this argument
    is ignored.


bool gdo_load_symbols (bool ignore_errors);

    Load the symbols. This function can safely be called multiple times.
    If ignore_errors is set true the function won't stop on the first
    symbol that can't be loaded but instead tries to load them all.
    If one or more symbols weren't loaded the function returns false.


bool gdo_load_symbol (const char *symbol);

    Load a specific symbol.


bool gdo_free_lib ();

    Free/release library handle.


bool gdo_lib_is_loaded ();

    Returns true if the library was successfully loaded.


const gdo_char_t *gdo_last_error ();

    Returns a pointer to the error message buffer with the last saved error string.
    This buffer can be empty and is in fact cleared when no error occured in a function.
    This function doesn't return a null pointer.


gdo_char_t *gdo_lib_origin ();

    Return the full library path. The returned string must be deallocated with free().
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

GDO_DEFAULT_FLAGS
    Override the default flags to use when loading a library.

GDO_DEFAULT_LIB
    Set a default library name through this macro (including double quote
    marks). This macro must be defined if you want to set GDO_ENABLE_AUTOLOAD
    or if you want to use the `gdo_load_lib()' function.

GDO_ENABLE_AUTOLOAD
    Define this macro if you want to use auto-loading wrapper functions.
    This means you don't need to explicitly call library load functions.
    It requires GDO_DEFAULT_LIB to be defined.
    If an error occures during loading these functions throw an error message
    and call `exit(1)'!

GDO_AUTO_RELEASE
    If defined the library handle will automatically be released on program exit.

GDO_WRAP_FUNCTIONS
    Use actual wrapped functions instead of a name alias. This is useful if you
    want to create a library to later link an application against.

GDO_VISIBILITY
    You can set the symbol visibility of wrapped functions (enabled with GDO_WRAP_FUNCTIONS)
    using this macro.



*****************
* Helper macros *
*****************

GDO_DEFAULT_FLAGS
    Default flags for `dlopen()' or `LoadLibraryEx()'

LIBNAME(NAME, API)
    Convenience macro to create versioned library names for DLLs, dylibs and DSOs,
    including double quote marks.
    LIBNAME(z,1) for example will become "libz-1.dll", "libz.1.dylib" or "libz.so.1".

LIBEXT
    Shared library file extension without dot ("dll", "dylib" or "so").
    Useful i.e. on plugins.

***/

/* static linkage */
#ifdef GDO_STATIC
    #define GDO_LINKAGE  static
#else
    #define GDO_LINKAGE  /**/
#endif


/* enable wrapped functions if auto-loading was enabled */
#if defined(GDO_ENABLE_AUTOLOAD) && !defined(GDO_WRAP_FUNCTIONS)
    #define GDO_WRAP_FUNCTIONS 1
#endif


/* char / wchar_t */
#if defined(GDO_WINAPI) && defined(_UNICODE)
typedef wchar_t gdo_char_t;
#else
typedef char gdo_char_t;
#endif


#ifdef GDO_DEFAULT_LIB
GDO_LINKAGE bool gdo_load_lib(void);
GDO_LINKAGE bool gdo_load_lib_and_symbols(void);
#endif
GDO_LINKAGE bool gdo_load_lib_args(const gdo_char_t *filename, int flags, bool new_namespace);
GDO_LINKAGE bool gdo_load_symbols(bool ignore_errors);
GDO_LINKAGE bool gdo_load_symbol(const char *symbol);
GDO_LINKAGE bool gdo_free_lib(void);
GDO_LINKAGE bool gdo_lib_is_loaded(void);
GDO_LINKAGE const gdo_char_t *gdo_last_error(void);
GDO_LINKAGE gdo_char_t *gdo_lib_origin(void);


#if !defined(GDO_WRAP_FUNCTIONS)

/* aliases to raw function pointers */
#define GDO_SYMBOL gdo_hndl.GDO_SYMBOL_ptr_

#endif //!GDO_WRAP_FUNCTIONS


/* aliases to raw object pointers */
#define GDO_OBJ_SYMBOL *gdo_hndl.GDO_OBJ_SYMBOL_ptr_

