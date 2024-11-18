#line 2 "<built-in>/c.h"

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
bool               gdo_load_lib_name_and_symbols (const gdo_char_t *filename);
bool               gdo_load_lib_args (const gdo_char_t *filename, int flags, bool new_namespace);

bool               gdo_lib_is_loaded ();
bool               gdo_free_lib ();

bool               gdo_all_symbols_loaded ();
bool               gdo_no_symbols_loaded ();
bool               gdo_any_symbol_loaded ();
bool               gdo_load_all_symbols (bool ignore_errors);
bool               gdo_load_symbol (int symbol_num);

bool               gdo_all_symbols_loaded ();
bool               gdo_no_symbols_loaded ();
bool               gdo_any_symbol_loaded ();

const gdo_char_t * gdo_last_error ();
gdo_char_t *       gdo_lib_origin ();



bool gdo_load_lib ();

    Load the library specified by the macro GDO_DEFAULT_LIB using default flags.
    This function is not available if GDO_DEFAULT_LIB was not defined.


bool gdo_load_lib_and_symbols ();

    Calls gdo_load_lib() and gdo_load_symbols().


bool gdo_load_lib_name (const gdo_char_t *filename);

    Load the library specified by `filename' using default flags.


bool gdo_load_lib_name_and_symbols (const gdo_char_t *filename);

    Calls gdo_load_lib_name() and gdo_load_symbols().


bool gdo_load_lib_args (const gdo_char_t *filename, int flags, bool new_namespace);

    Load the library; `filename' and `flags' are passed to the underlying library
    loading functions.

    If `new_namespace' is true the library will be loaded into a new namespace.
    This is done using dlmopen() with the LM_ID_NEWLM argument.
    This argument is only used on Glibc and if _GNU_SOURCE was defined.


bool gdo_lib_is_loaded ();

    Returns true if the library was successfully loaded.


bool gdo_free_lib ();

    Free/release library handle.


bool gdo_load_symbols (bool ignore_errors);

    Load the symbols. This function can safely be called multiple times.
    If ignore_errors is set true the function won't stop on the first
    symbol that can't be loaded but instead tries to load them all.
    If one or more symbols weren't loaded the function returns false.


bool gdo_load_symbol (int symbol_num);

    Load a specific symbol.
    `symbol_num' is an enumeration value: `GDO_LOAD_<symbol_name>'


bool gdo_all_symbols_loaded ();

    Returns true if ALL symbols were successfully loaded.


bool gdo_no_symbols_loaded ();

    Returns true if NO symbols were loaded at all.


bool gdo_any_symbol_loaded ();

    Returns true if 1 or more symbols were successfully loaded.


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

GDO_STATIC
    If defined static linkage (the `static' keyword) is used for all
    functions.

GDO_DEFAULT_FLAGS
    Override the default flags to use when loading a library.

GDO_DEFAULT_LIB
    Set a default library name through this macro (including double quote
    marks). This macro must be defined if you want to set GDO_ENABLE_AUTOLOAD
    or if you want to use the `gdo_load_lib()' function.

GDO_WRAP_FUNCTIONS
    Use actual wrapped functions instead of a name alias. This is useful if you
    want to create a library to later link an application against.

GDO_ENABLE_AUTOLOAD
    Define this macro if you want to use auto-loading wrapper functions.
    This means you don't need to explicitly call library load functions.
    The first wrapper function called will load all symbols at once.
    It requires GDO_DEFAULT_LIB to be defined.
    If an error occures during loading these functions print an error message
    and call `exit(1)'!

GDO_DELAYLOAD
    Same as GDO_ENABLE_AUTOLOAD but only the requested symbol is loaded when its
    wrapper function is called instead of all symbols.
    It requires GDO_ENABLE_AUTOLOAD to be defined.

GDO_AUTO_RELEASE
    If defined the library handle will automatically be released on program exit.

GDO_VISIBILITY
    You can set the symbol visibility of wrapped functions (enabled with GDO_WRAP_FUNCTIONS)
    using this macro.

GDO_USE_MESSAGE_BOX
    If win32 API is used and GDO_ENABLE_AUTOLOAD was activated this will enable
    error messages from auto-loading to be displayed in MessageBox windows.

GDO_DISABLE_ALIASING
    Don't use preprocessor macros to alias symbol names. Use with care.

GDO_DISABLE_DLINFO
    Always disable usage of `dlinfo(3)'.

GDO_DISABLE_DLMOPEN
    Always disable usage of `dlmopen(3)'.



*****************
* Helper macros *
*****************

GDO_DEFAULT_FLAGS
    Default flags for `dlopen()' or `LoadLibraryEx()'

LIBNAME(NAME, API)
LIBNAMEA(NAME, API)
LIBNAMEW(NAME, API)
    Convenience macro to create versioned library names for DLLs, dylibs and DSOs,
    including double quote marks.
    LIBNAME(z,1) for example will become "libz-1.dll", "libz.1.dylib" or "libz.so.1".

LIBEXT
LIBEXTA
LIBEXTW
    Shared library file extension without dot ("dll", "dylib" or "so").
    Useful i.e. on plugins.

***/


/*****************************************************************************/
/*                                   C API                                   */
/*****************************************************************************/


/* static linkage */
#ifdef GDO_STATIC
# define GDO_LINKAGE  static
#else
# define GDO_LINKAGE  /**/
#endif


/* char / wchar_t */
#if defined(GDO_WINAPI) && defined(_UNICODE)
typedef wchar_t gdo_char_t;
#else
typedef char gdo_char_t;
#endif


/* Our library and symbols handle */
typedef struct gdo_handle
{
#ifdef GDO_WINAPI
    HMODULE handle;
    DWORD last_errno;
    /* FormatMessage: according to MSDN the maximum is either 64k or 128k */
    gdo_char_t buf_formatted[64*1024];
#else
    void *handle;
#endif
    bool call_free_lib_is_registered;
    gdo_char_t buf[8*1024];

    /* symbols */
    %%type%% (*%%func_symbol%%_ptr_)(%%args%%);
    %%obj_type%% *%%obj_symbol%%_ptr_;

} gdo_handle_t;

#ifndef GDO_STATIC
extern gdo_handle_t gdo_hndl;
#endif


#ifdef GDO_DEFAULT_LIB
GDO_LINKAGE bool gdo_load_lib(void);
GDO_LINKAGE bool gdo_load_lib_and_symbols(void);
#endif
GDO_LINKAGE bool gdo_load_lib_name(const gdo_char_t *filename);
GDO_LINKAGE bool gdo_load_lib_name_and_symbols(const gdo_char_t *filename);
GDO_LINKAGE bool gdo_load_lib_args(const gdo_char_t *filename, int flags, bool new_namespace);

GDO_LINKAGE bool gdo_lib_is_loaded(void);
GDO_LINKAGE bool gdo_free_lib(void);

GDO_LINKAGE bool gdo_all_symbols_loaded(void);
GDO_LINKAGE bool gdo_no_symbols_loaded(void);
GDO_LINKAGE bool gdo_any_symbol_loaded(void);
GDO_LINKAGE bool gdo_load_all_symbols(bool ignore_errors);
GDO_LINKAGE bool gdo_load_symbol(int symbol_num);

GDO_LINKAGE const gdo_char_t *gdo_last_error(void);
GDO_LINKAGE gdo_char_t *gdo_lib_origin(void);


/* enumeration values for gdo_load_symbol() */
enum {
    GDO_LOAD_%%symbol%%,
    GDO_LOAD_COUNT
};

/* prefixed aliases, useful if GDO_DISABLE_ALIASING was defined */
#define GDO_ALIAS_%%func_symbol%% gdo_hndl.%%func_symbol%%_ptr_
#define GDO_ALIAS_%%obj_symbol%% *gdo_hndl.%%obj_symbol%%_ptr_

/* aliases to raw function pointers */
#if !defined(GDO_DISABLE_ALIASING) && !defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)
#define %%func_symbol%% gdo_hndl.%%func_symbol%%_ptr_
#endif

/* aliases to raw object pointers */
#if !defined(GDO_DISABLE_ALIASING)
#define %%obj_symbol%% *gdo_hndl.%%obj_symbol%%_ptr_
#endif

