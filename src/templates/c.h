/***

******************
*   $char_t   *
******************

If compiling for win32 and `_UNICODE` is defined and `_$USE_DLOPEN` is NOT defined
`$char_t` will become `wchar_t`.

Otherwise `$char_t` will become `char`.


**************************
*   Functions provided   *
**************************

bool            $load_lib ();
bool            $load_lib_and_symbols ();
bool            $load_lib_name (const $char_t *filename);
bool            $load_lib_args (const $char_t *filename, int flags, bool new_namespace);
bool            $load_symbols (bool ignore_errors);
bool            $load_symbol (const char *symbol);
bool            $free_lib ();
bool            $lib_is_loaded ();
const $char_t * $last_error ();
$char_t *       $lib_origin ();


bool $load_lib ();

    Load the library specified by the macro _$DEFAULT_LIB using default flags.
    This function is not available if _$DEFAULT_LIB was not defined.


bool $load_lib_and_symbols ();

    Calls $load_lib() and $load_symbols().


bool $load_lib_name (const $char_t *filename);

    Load the library specified by `filename' using default flags.


bool $load_lib_args (const $char_t *filename, int flags, bool new_namespace);

    Load the library; `filename' and `flags' are passed to the underlying library
    loading functions.

    If `new_namespace' is true the library will be loaded into a new namespace.
    This is done using dlmopen() with the LM_ID_NEWLM argument.
    If the win32 API is used or the macro _$NO_DLMOPEN is defined this argument
    is ignored.


bool $load_symbols (bool ignore_errors);

    Load the symbols. This function can safely be called multiple times.
    If ignore_errors is set true the function won't stop on the first
    symbol that can't be loaded but instead tries to load them all.
    If one or more symbols weren't loaded the function returns false.


bool $load_symbol (const char *symbol);

    Load a specific symbol.


bool $free_lib ();

    Free/release library handle.


bool $lib_is_loaded ();

    Returns true if the library was successfully loaded.


const $char_t *$last_error ();

    Returns a pointer to the error message buffer with the last saved error string.
    This buffer can be empty and is in fact cleared when no error occured in a function.
    This function doesn't return a null pointer.


$char_t *$lib_origin ();

    Return the full library path. The returned string must be deallocated with free().
    On error or if no library was loaded NULL is returned.



****************************************************
* The following options may be set through macros: *
****************************************************

_$USE_DLOPEN
    If defined use `dlopen()' API on win32 targets.
    On other targets `dlopen()' is always used.

_$NO_DLMOPEN
    If defined `dlmopen()` will never be used.
    See the function `$load_lib_args()` for details.

_$STATIC
    If defined static linkage (the `static' keyword) is used for all
    functions.

_$DEFAULT_LIB
    Set a default library name through this macro (including double quote
    marks). This macro must be defined if you want to set _$ENABLE_AUTOLOAD
    or if you want to use the `$load_lib()' function.

_$ENABLE_AUTOLOAD
    Define this macro if you want to use auto-loading wrapper functions.
    This means you don't need to explicitly call library load functions.
    It requires _$DEFAULT_LIB to be defined.
    If an error occures during loading these functions throw an error message
    and call `exit(1)'!

_$AUTO_RELEASE
    If defined the library handle will automatically be released on program exit.

_$WRAP_FUNCTIONS
    Use actual wrapped functions instead of a name alias. This is useful if you
    want to create a library to later link an application against.

_$VISIBILITY
    You can set the symbol visibility of wrapped functions (enabled with _$WRAP_FUNCTIONS)
    using this macro.



*****************
* Helper macros *
*****************

_$DEFAULT_FLAGS
    Default flags for `dlopen()' or `LoadLibraryEx()'

_$LIB(NAME, API)
    Convenience macro to create versioned library names for DLLs, dylibs and DSOs,
    including double quote marks.
    _$LIB(z,1) for example will become "libz-1.dll", "libz.1.dylib" or "libz.so.1".

_$LIBEXT
    Shared library file extension without dot ("dll", "dylib" or "so").
    Useful i.e. on plugins.

***/

#ifdef __cplusplus
extern "C" {
#endif


GDO_COMMON


/* static linkage */
#ifdef _$STATIC
    #define _$LINKAGE  static
#else
    #define _$LINKAGE  /**/
#endif


/* char / wchar_t */
#if defined(_$WINAPI) && defined(_UNICODE)
typedef wchar_t $char_t;
#else
typedef char $char_t;
#endif


#ifdef _$DEFAULT_LIB
_$LINKAGE bool $load_lib(void);
_$LINKAGE bool $load_lib_and_symbols(void);
#endif
_$LINKAGE bool $load_lib_args(const $char_t *filename, int flags, bool new_namespace);
_$LINKAGE bool $load_symbols(bool ignore_errors);
_$LINKAGE bool $load_symbol(const char *symbol);
_$LINKAGE bool $free_lib(void);
_$LINKAGE bool $lib_is_loaded(void);
_$LINKAGE const $char_t *$last_error(void);
_$LINKAGE $char_t *$lib_origin(void);


#if !defined(_$WRAP_FUNCTIONS)

/* aliases to raw function pointers */
#define GDO_SYMBOL $hndl.GDO_SYMBOL_ptr_

#endif //!_$WRAP_FUNCTIONS


/* aliases to raw object pointers */
#define GDO_OBJ_SYMBOL *$hndl.GDO_OBJ_SYMBOL_ptr_


#ifdef __cplusplus
} /* extern "C" */
#endif
