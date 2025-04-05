#ifdef GDO_WINAPI
# include <wchar.h>
#endif
#ifndef __cplusplus
# include <stdbool.h>
#endif

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
bool               gdo_load_all_symbols ();
bool               gdo_load_symbol (int symbol_num);
bool               gdo_load_symbol_name (const char *symbol);

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


bool gdo_load_all_symbols ();

    Load the symbols. This function can safely be called multiple times.


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
    The message will indicate if no error occured in a function.
    This function doesn't return a null pointer or empty string.


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
    If defined `static inline' linkage is used for all functions.

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
    Windows only: if GDO_ENABLE_AUTOLOAD was activated this will enable
    error messages from auto-loading to be displayed in MessageBox windows.

GDO_DISABLE_ALIASING
    Don't use preprocessor macros to alias symbol names. Use with care.

GDO_DISABLE_DLINFO
    Always disable usage of `dlinfo(3)' to retrieve the library path.
    `dladdr(3)' will be used instead.

GDO_DISABLE_DLMOPEN
    Always disable usage of `dlmopen(3)'.



*****************
* Helper macros *
*****************

GDO_ALIAS_<symbol>
    Convenience macro to access the symbol pointer. I.e. `GDO_ALIAS_helloworld' will
    access the pointer to `helloworld'.

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



*********
* Hooks *
*********

GDO_HOOK_<function>(...)
    Define a hook macro that will be inserted into a wrap function.
    The hook is placed before the actual function call.
    If you want to call the function inside the macro you must do so using the GDO_ALIAS_* prefix.
    Parameter names are taken from the function prototype declarations (or it's "a, b, c" and so on
    if the header was created with `-param=create'). A hook may be left undefined.

    For example if a function declaration is `int sum_of_a_and_b(int val_a, int val_b)':

    #define GDO_HOOK_sum_of_a_and_b(...) \
      printf("debug: the sum of %d and %d is %d\n", \
        val_a, val_b, GDO_ALIAS_sum_of_a_and_b(__VA_ARGS__));

***/


/*****************************************************************************/
/*                                   C API                                   */
/*****************************************************************************/


/* declaration */
#ifdef GDO_STATIC
# define GDO_DECL      static inline
# define GDO_OBJ_DECL  static
#else
# define GDO_DECL      extern
# define GDO_OBJ_DECL  extern
#endif

/* attributes */
#ifdef __GNUC__
# define GDO_ATTR(x)  __attribute__ ((x))
#else
# define GDO_ATTR(x)  /**/
#endif


/* char / wchar_t */
#ifdef _GDO_TARGET_WIDECHAR
typedef wchar_t gdo_char_t;
#else
typedef char    gdo_char_t;
#endif


/* our library and symbols handle */
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
    struct _gdo_ptr {
        %%type%% (*%%func_symbol%%)(%%args%%);
        %%obj_type%% *%%obj_symbol%%;
    } ptr;

} gdo_handle_t;


GDO_OBJ_DECL gdo_handle_t gdo_hndl;

#ifdef GDO_DEFAULT_LIB
GDO_DECL bool gdo_load_lib(void);
GDO_DECL bool gdo_load_lib_and_symbols(void);
#endif
GDO_DECL bool gdo_load_lib_name(const gdo_char_t *filename);
GDO_DECL bool gdo_load_lib_name_and_symbols(const gdo_char_t *filename);
GDO_DECL bool gdo_load_lib_args(const gdo_char_t *filename, int flags, bool new_namespace);

GDO_DECL bool gdo_lib_is_loaded(void);
GDO_DECL bool gdo_free_lib(void);

GDO_DECL bool gdo_all_symbols_loaded(void);
GDO_DECL bool gdo_no_symbols_loaded(void);
GDO_DECL bool gdo_any_symbol_loaded(void);
GDO_DECL bool gdo_load_all_symbols(void);
GDO_DECL bool gdo_load_symbol(int symbol_num);
GDO_DECL bool gdo_load_symbol_name(const char *symbol);

GDO_DECL const gdo_char_t *gdo_last_error(void)  GDO_ATTR (returns_nonnull);
GDO_DECL gdo_char_t *gdo_lib_origin(void)  GDO_ATTR (warn_unused_result);


/* enumeration values for gdo_load_symbol() */
enum {
    GDO_LOAD_%%symbol%%,
    GDO_ENUM_LAST
};


/* prefixed aliases, useful if GDO_DISABLE_ALIASING was defined */
#define GDO_ALIAS_%%func_symbol_pad%% gdo_hndl.ptr.%%func_symbol%%
#define GDO_ALIAS_%%obj_symbol_pad%% *gdo_hndl.ptr.%%obj_symbol%%


/* disable aliasing if we saved into separate files and the
 * header file was included from the body file */
#if defined(GDO_SEPARATE) && !defined(GDO_INCLUDED_IN_BODY)

/* aliases to raw function pointers */
#if !defined(GDO_DISABLE_ALIASING) && !defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)
#define %%func_symbol_pad%% GDO_ALIAS_%%func_symbol%%
#endif

/* aliases to raw object pointers */
#if !defined(GDO_DISABLE_ALIASING)
#define %%obj_symbol_pad%% GDO_ALIAS_%%obj_symbol%%
#endif

#endif //GDO_SEPARATE && !GDO_INCLUDED_IN_BODY

