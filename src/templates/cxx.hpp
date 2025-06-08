/***

****************************************************
* The following options may be set through macros: *
****************************************************

GDO_USE_DLOPEN
    If defined `dlopen()' API is used on win32 targets.
    On other targets `dlopen()' is always used.

GDO_DEFAULT_FLAGS
    Override the default flags to use when loading a library.

GDO_DEFAULT_LIB
    Set a default library name through this macro (including double quote
    marks). This macro must be defined if you want to set GDO_ENABLE_AUTOLOAD.

GDO_WRAP_FUNCTIONS
    Use actual wrapped functions instead of a name alias. This is useful if you
    want to create a library to later link an application against.

GDO_ENABLE_AUTOLOAD
    Define this macro if you want to use auto-loading wrapper functions.
    This means you don't need to explicitly call library load functions.
    The first wrapper function called will load all symbols at once.
    It requires GDO_DEFAULT_LIB to be defined.
    If an error occures during loading these functions print an error message
    and call `std::exit(1)'!

GDO_DELAYLOAD
    Same as GDO_ENABLE_AUTOLOAD but only the requested symbol is loaded when its
    wrapper function is called instead of all symbols.
    It requires GDO_ENABLE_AUTOLOAD to be defined.

GDO_VISIBILITY
    You can set the symbol visibility of wrapped functions (enabled with
    GDO_WRAP_FUNCTIONS) using this macro.

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

#include <string>


/* enumeration values for `load_symbol()' method */
enum {
    GDO_LOAD_%%symbol%%,
    GDO_ENUM_LAST
};


namespace gdo
{
/* silence `unused reference' compiler warnings */
template<typename T>
void UNUSED_REF(T x);


/* Create versioned library names for DLLs, dylibs and DSOs.
 * make_libname("z",1) for example will return "libz-1.dll", "libz.1.dylib" or "libz.so.1" */
std::string make_libname(const std::string &name, const size_t api);
#ifdef GDO_WINAPI
std::wstring make_libname(const std::wstring &name, const size_t api);
#endif


/* default flags */
constexpr const int default_flags = GDO_DEFAULT_FLAGS;


/* Shared library file extension without dot ("dll", "dylib" or "so").
 * Useful i.e. on plugins. */
constexpr const char    * const libext   = LIBEXTA;
#ifdef GDO_WINAPI
constexpr const wchar_t * const libext_w = LIBEXTW;
#endif


/* symbol pointers */
namespace ptr {
    extern %%type%% (*%%func_symbol%%)(%%args%%);
    extern %%obj_type%% *%%obj_symbol%%;
};


/*****************************************************************************/
/*                          library loader class                             */
/*****************************************************************************/
class dl
{
#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

public:
    using message_callback_t = void (*)(const char *);

private:
    /* function pointer to error message callback */
    static message_callback_t m_message_callback;

#endif

private:

    std::string m_filename;
#ifdef GDO_WINAPI
    std::wstring m_wfilename;
#endif

    int m_flags = default_flags;
    bool m_new_namespace = false;
    bool m_free_lib_in_dtor = true;

#ifdef GDO_WINAPI

    /* library handle */
    using handle_t = HMODULE;
    static handle_t m_handle;

    /* error message */
    DWORD m_last_error = 0;
    std::string m_errmsg;
    std::wstring m_werrmsg;

    errno_t mbs_wcs_conv(size_t *rv, wchar_t *out, size_t sz, const char *in, size_t count);
    errno_t mbs_wcs_conv(size_t *rv, char *out, size_t sz, const wchar_t *in, size_t count);

    /* convert string */
    template<typename T_out, typename T_in>
    std::basic_string<T_out> convert_string(const std::basic_string<T_in> &str_in);

    /* clear error */
    void clear_error();

    /* save last error */
    void save_error();
    void save_error(const std::string &msg);
    void save_error(const std::wstring &msg);

    /* if m_handle is NULL */
    void set_error_invalid_handle();

    HMODULE load_library_ex(const wchar_t *path);
    HMODULE load_library_ex(const char *path);

    /* documentation says only backward slash path separators shall be used on
     * LoadLibraryEx(), so transform the path if needed */
    template<typename T>
    void transform_path_and_load_library(const std::basic_string<T> &filename, const T &fwd_slash, const T &bwd_slash);

    /* load library */
    void load_lib(const std::string &filename, int flags, bool new_namespace);
    void load_lib(const std::wstring &filename, int flags, bool new_namespace);

    /* load symbol address */
    template<typename T>
    T sym_load(const char *symbol);

    DWORD get_module_filename(wchar_t *buf, DWORD len);
    DWORD get_module_filename(char *buf, DWORD len);

    /* get the module's full path using GetModuleFileName() */
    template<typename T>
    std::basic_string<T> get_origin_from_module_handle();

    void format_message(DWORD flags, DWORD msgId, DWORD langId, wchar_t *buf);
    void format_message(DWORD flags, DWORD msgId, DWORD langId, char *buf);

    /* return a formatted error message */
    template<typename T>
    std::basic_string<T> format_last_error_message();

#else // !GDO_WINAPI

    /* library handle */
    using handle_t = void*;
    static handle_t m_handle;

    /* error message */
    std::string m_errmsg;

    /* clear error */
    void clear_error();

    /* save last error */
    void save_error();
    void save_error(const std::string&);

    /* if m_handle is NULL */
    void set_error_invalid_handle();

    /* load library */
    void load_lib(const std::string &filename, int flags, bool new_namespace);

    /* load symbol address */
    template<typename T>
    T sym_load(const char *symbol);

#endif // !GDO_WINAPI

    /* load library by filename */
    template<typename T>
    bool load_filename(const T &filename, int flags, bool new_namespace);


public:

    /* c'tor */
    dl();
    dl(const std::string &filename, int flags=default_flags, bool new_namespace=false);
#ifdef GDO_WINAPI
    dl(const std::wstring &filename, int flags=default_flags, bool new_namespace=false);
#endif

    /* d'tor */
    ~dl();

    /* load library */
    bool load(const std::string &filename, int flags=default_flags, bool new_namespace=false);
#ifdef GDO_WINAPI
    bool load(const std::wstring &filename, int flags=default_flags, bool new_namespace=false);
#endif
    bool load();

    /* load library and symbols */
    bool load_lib_and_symbols();

    /* check if library is loaded */
    static bool lib_loaded();

    /* return the flags used to load the library */
    int flags() const;

    /* load all symbols */
    bool load_all_symbols();

    /* load a specific symbol */
    bool load_symbol(int symbol_num);
    bool load_symbol(const char *symbol);

    /* check if symbols were loaded */
    bool all_symbols_loaded() const;
    bool no_symbols_loaded() const;
    bool any_symbol_loaded() const;

    /* free library; always succeeds if `force == true` */
    bool free(bool force=false);

    /* whether to free the library in the class destructor */
    void free_lib_in_dtor(bool b);

#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

    /* Get/set message callback function. */
    static void message_callback(message_callback_t cb);
    static message_callback_t message_callback();

#endif // GDO_WRAP_FUNCTIONS || GDO_ENABLE_AUTOLOAD

#ifdef GDO_WINAPI

    /* get path of loaded library */
    std::string origin();
    std::wstring origin_w();

    /* retrieve the last error */
    std::string error();
    std::wstring error_w();

    /* get filename passed to load */
    std::string filename();
    std::wstring filename_w();

#else

    /* get path of loaded library */
    std::string origin();

    /* retrieve the last error */
    std::string error() const;

    /* get filename passed to load */
    std::string filename() const;

#endif // !GDO_WINAPI

};
/******************************* end of class ********************************/

} /* end namespace */


/* prefixed aliases, useful if GDO_DISABLE_ALIASING was defined */
#define GDO_ALIAS_%%func_symbol_pad%% gdo::ptr::%%func_symbol%%
#define GDO_ALIAS_%%obj_symbol_pad%% *gdo::ptr::%%obj_symbol%%


/* disable aliasing if we saved into separate files and the
 * header file was included from the body file */
#if defined(GDO_SEPARATE) && !defined(GDO_INCLUDED_IN_BODY)

/* aliases to raw function pointers */
#if !defined(GDO_DISABLE_ALIASING) && !defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)
#define %%func_symbol_pad%% GDO_ALIAS_%%func_symbol_pad%%
#endif

/* aliases to raw object pointers */
#if !defined(GDO_DISABLE_ALIASING)
#define %%obj_symbol_pad%% GDO_ALIAS_%%obj_symbol_pad%%
#endif

#endif //GDO_SEPARATE && !GDO_INCLUDED_IN_BODY

