#include <iostream>
#include <string>


/**
 * Enumeration values for `load_symbol()' method
 */
enum {
    GDO_LOAD_%%symbol%%,
    GDO_ENUM_LAST
};


namespace gdo
{

/**
 * Symbol pointers.
 * Symbol names must be prefixed to avoid macro expansion.
 */
extern %%type%% (*GDO_PTR_%%func_symbol%%)(%%args%%);
extern %%obj_type%% *GDO_PTR_%%obj_symbol%%;


/**
 * Create versioned library names.
 * make_libname(example,1) for example will become "libexample.1.dylib" on macOS.
 *
 * name:
 *   library name without prefix
 *
 * api:
 *   API number to add to filename
 */
std::string make_libname(const std::string &name, const size_t api);
#ifdef GDO_WINAPI
std::wstring make_libname(const std::wstring &name, const size_t api);
#endif


/**
 * Default flags to use when loading a library.
 */
constexpr const int default_flags = GDO_DEFAULT_FLAGS;


/**
 * Shared library file extension without dot. Useful i.e. on plugins.
 */
constexpr const char    * const libext   = GDO_LIBEXTA;
#ifdef GDO_WINAPI
constexpr const wchar_t * const libext_w = GDO_LIBEXTW;
#endif


/**
 * Silence `unused reference' compiler warnings.
 */
template<typename T>
void UNUSED_REF(T x)
{
    static_cast<void>(x);
}


/*****************************************************************************/
/*                          library loader class                             */
/*****************************************************************************/
class dl
{
#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

public:
    using msgcb_t = void (*)(const char *);

private:
    static void default_message_callback(const char *msg);
    static msgcb_t m_message_callback;

#endif // GDO_WRAP_FUNCTIONS || GDO_ENABLE_AUTOLOAD

private:

    std::string m_filename;
#ifdef GDO_WINAPI
    std::wstring m_wfilename;
#endif

    int m_flags = default_flags;
    bool m_new_namespace = false;
    bool m_free_lib_in_dtor = true;

#ifdef GDO_WINAPI

    static HMODULE m_handle;
    DWORD m_last_errno = 0;
    std::string m_errmsg;
    std::wstring m_werrmsg;

    bool m_convert_filename_to_wcs = false;

    bool mbs_wcs_conv(size_t *retval, wchar_t *out, size_t size, const char *in);
    bool mbs_wcs_conv(size_t *retval, char *out, size_t size, const wchar_t *in);

    template<typename T_out, typename T_in>
    std::basic_string<T_out> convert_string(const std::basic_string<T_in> &str_in);

    void clear_error();

    void save_error();
    void save_error(const std::string &msg);
    void save_error(const std::wstring &msg);

    void set_error_invalid_handle();

    HMODULE load_library_ex(const std::wstring &filename);
    HMODULE load_library_ex(const std::string &filename);

    template<typename T>
    void transform_path_and_load_library(const std::basic_string<T> &filename, const T &fwd_slash, const T &bwd_slash);

    void load_lib(const std::string &filename, int flags, bool new_namespace);
    void load_lib(const std::wstring &filename, int flags, bool new_namespace);

    template<typename T>
    T sym_load(const char *symbol);

    DWORD get_module_filename(wchar_t *buf, DWORD len);
    DWORD get_module_filename(char *buf, DWORD len);

    template<typename T>
    std::basic_string<T> get_origin_from_module_handle();

    void format_message(DWORD flags, DWORD msgId, DWORD langId, wchar_t *buf);
    void format_message(DWORD flags, DWORD msgId, DWORD langId, char *buf);

    /* std::enable_if and std::is_same combined */
    template<typename T, typename U>
    using enable_if_same_bool = typename std::enable_if<std::is_same<T, U>::value, bool>::type;

    /* template wrapper around std::to_(w)string */
    template<typename T, typename U, enable_if_same_bool<T, std::string::value_type> = true>
    std::string to_string(U val) {
        return std::to_string(val);
    }

    template<typename T, typename U, enable_if_same_bool<T, std::wstring::value_type> = true>
    std::wstring to_string(U val) {
        return std::to_wstring(val);
    }

    /* return char or wchar_t string */
    template<typename T, enable_if_same_bool<T, char> = true>
    const char *get_string(const char *str, const wchar_t *) {
        return str;
    }

    template<typename T, enable_if_same_bool<T, wchar_t> = true>
    const wchar_t *get_string(const char *, const wchar_t *wstr) {
        return wstr;
    }

    /* format_error_message */
    template<typename T_out, typename T_in>
    std::basic_string<T_out> format_error_message(const std::basic_string<T_out> &buf1, const std::basic_string<T_in> &buf2);

#else // !GDO_WINAPI

    static void *m_handle;
    std::string m_errmsg;

    void clear_error();
    void save_error(const std::string &msg = {}); /* `msg' is always ignored */
    void set_error_invalid_handle();

    void load_lib(const std::string &filename, int flags, bool new_namespace);

    template<typename T>
    T sym_load(const char *symbol);

#endif // !GDO_WINAPI

    template<typename T>
    bool load_filename(const T &filename, int flags, bool new_namespace);


public:

    /**
     * Empty constructor
     */
    dl();


    /**
     * Constructor. The specified filename is not loaded yet.
     *
     * filename:
     *   Library filename or path to load. Must not be empty.
     *
     * flags:
     *   These are passed to the underlying library loading functions.
     *
     * new_namespace:
     *   If true the library will be loaded into a new namespace.
     *   This is done using dlmopen() with the LM_ID_NEWLM argument.
     *   This argument is only used on Glibc and if _GNU_SOURCE was defined,
     *   otherwise it has no effect.
     */
    dl(const std::string &filename, int flags=default_flags, bool new_namespace=false);
#ifdef GDO_WINAPI
    dl(const std::wstring &filename, int flags=default_flags, bool new_namespace=false);
#endif


    /**
     * Destructor. Will free library handle by default (see also `free_lib_in_dtor()').
     */
    ~dl();


    /**
     * Load a library.
     *
     * filename:
     *   Library filename or path to load. Must not be empty.
     *
     * flags:
     *   These are passed to the underlying library loading functions.
     *
     * new_namespace:
     *   If true the library will be loaded into a new namespace.
     *   This is done using dlmopen() with the LM_ID_NEWLM argument.
     *   This argument is only used on Glibc and if _GNU_SOURCE was defined,
     *   otherwise it has no effect.
     *
     * On success `true' is returned.
     * On an error or if the library is already loaded the return value is `false'.
     */
    bool load(const std::string &filename, int flags=default_flags, bool new_namespace=false);
#ifdef GDO_WINAPI
    bool load(const std::wstring &filename, int flags=default_flags, bool new_namespace=false);
#endif


    /**
     * Load the library filename or path specified in the class constructor.
     *
     * On success `true' is returned.
     * On an error or if the library is already loaded the return value is `false'.
     */
    bool load();


    /**
     * Load the library filename or path specified in the class constructor
     * and all the symbols.
     *
     * On success `true' is returned.
     * On an error or if the library is already loaded the return value is `false'.
     */
    bool load_lib_and_symbols();


    /**
     * Returns `true' if the library was successfully loaded.
     */
    static bool lib_loaded();


    /**
     * Returns the flags used on the last attempt to load the library or zero.
     */
    int flags() const;


    /**
     * Load symbols.
     *
     * symbol_num:
     *   Auto-generated enumeration value `GDO_LOAD_<symbol_name>'.
     *   For example use `GDO_LOAD_foo' to load the symbol `foo'.
     *
     * symbol:
     *   Name of the symbol to load.
     *
     * Returns `true' on success or if the symbol was already loaded.
     */
    bool load_all_symbols();
    bool load_symbol(int symbol_num);
    bool load_symbol(const char *symbol);


    /**
     * Check if symbols were loaded.
     */
    bool all_symbols_loaded() const;
    bool no_symbols_loaded() const;
    bool any_symbol_loaded() const;


    /**
     * Whether or not narrow char (std::string / char *) library names should be
     * converted to wide char (std::wstring / wchar_t *) and passed to LoadLibraryExW().
     * By default no conversion is done and narrow char names will be passed to
     * LoadLibraryExA().
     * Wide char names are always passed to LoadLibraryExW().
     *
     * b:
     *   If set `true' and the given library name was in std::string format the
     *   name will be converted to std::wstring and passed to LoadLibraryExW().
     *   If set `false' and the given library name was in std::string format
     *   it will be passed to LoadLibraryExA().
     */
#ifdef GDO_WINAPI
    void convert_filename_to_wcs(bool b);
    bool convert_filename_to_wcs() const;
#endif


    /**
     * Free/release the library. Internal handle and pointers are set back to NULL
     * if the underlying calls were successful, in which case `true' is returned.
     * Can safely be called even if no library was loaded.
     *
     * force:
     *   Don't check if the underlying calls were successful.
     *   Internal handle and pointers are always set back to NULL.
     *
     * Return value is `true' if no library was loaded or if `force' was set `true'.
     */
    bool free(bool force=false);


    /**
     * Explicitly enable or disable automatic library freeing through class destructor.
     * This is enabled by default.
     *
     * b:
     *   true == enable
     *   false == disable
     */
    void free_lib_in_dtor(bool b);
    bool free_lib_in_dtor() const;


#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

    /**
     * Set a custom message callback function to be used if an error
     * occurred in the wrapped functions.
     *
     * cb:
     *   Callback function pointer of type `void (*)(const char *)'.
     *   The function takes an error message as argument.
     *   Setting cb to NULL will enable the default callback function.
     */
    static void message_callback(void (*cb)(const char *msg));


    /**
     * Return pointer to currently used message callback function.
     */
    static msgcb_t message_callback();

#endif // GDO_WRAP_FUNCTIONS || GDO_ENABLE_AUTOLOAD


    /**
     * Return the full library path.
     * On error or if no library was loaded an empty string is returned.
     *
     * On some systems and configurations the path is taken from the loaded symbols
     * in which case at least one symbol must have been successfully loaded before
     * using this function.
     */
    std::string origin();
#ifdef GDO_WINAPI
    std::wstring origin_w();
#endif


    /**
     * Return a string with the last saved error message.
     * The message will indicate if no error had occured.
     * This function doesn't return an empty string.
     */
#ifdef GDO_WINAPI
    std::string error();
    std::wstring error_w();
#else
    std::string error() const;
#endif

    /**
     * Return the original filename passed to load.
     * This is not the same as `origin()'.
     */
#ifdef GDO_WINAPI
    std::string filename();
    std::wstring filename_w();
#else
    std::string filename() const;
#endif

};
/******************************* end of class ********************************/

} /* end namespace */


/**
 * Prefixed aliases, useful if GDO_DISABLE_ALIASING was defined.
 */
#define GDO_RAWPTR_%%func_symbol_pad%% gdo::GDO_PTR_%%func_symbol%%
#define GDO_RAWPTR_%%obj_symbol_pad%% gdo::GDO_PTR_%%obj_symbol%%
%PARAM_SKIP_REMOVE_BEGIN%


/*****************************************************************************/
/*                                wrap code                                  */
/*****************************************************************************/
#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

/* #define empty hooks by default */
#ifndef GDO_HOOK_%%func_symbol%%@
#define GDO_HOOK_%%func_symbol%%(...) /**/@
#endif


/* diagnostic warnings on variable arguments functions */
#if !defined(GDO_DISABLE_WARNINGS) && defined(GDO_WRAP_VISIBILITY)

# ifdef GDO_HAS_VA_ARGS_%%func_symbol%%@
GDO_WARNING("GDO_WRAP_VISIBILITY defined but wrapper function %%func_symbol%%() can only be used inlined; define GDO_DISABLE_WARNINGS to silence this message")@
# endif@

#endif //!GDO_DISABLE_WARNINGS && GDO_WRAP_VISIBILITY


namespace gdo {
    namespace wrap {
        void not_loaded(int load, const char *sym);
    }
}

@
/* %%func_symbol%%() */@
#ifdef GDO_HAS_VA_ARGS_%%func_symbol%%@
    template<typename... Types>@
    %%type%% GDO_WRAP(%%func_symbol%%) (Types... args) {@
        if (!GDO_RAWPTR_%%func_symbol%%) {@
            gdo::wrap::not_loaded(GDO_LOAD_%%func_symbol%%, "%%func_symbol%%");@
        }@
        GDO_HOOK_%%func_symbol%%(args...);@
        %%return%% GDO_RAWPTR_%%func_symbol%%(args...);@
    }@
#else@
    GDO_WRAP_DECL@
    %%type%% GDO_WRAP(%%func_symbol%%) (%%args%%) {@
        if (!GDO_RAWPTR_%%func_symbol%%) {@
            gdo::wrap::not_loaded(GDO_LOAD_%%func_symbol%%, "%%func_symbol%%");@
        }@
        GDO_HOOK_%%func_symbol%%(%%param_names%%);@
        %%return%% GDO_RAWPTR_%%func_symbol%%(%%param_names%%);@
    }@
#endif //!GDO_HAS_VA_ARGS_%%func_symbol%%


#endif //GDO_WRAP_FUNCTIONS ...
/***************************** end of wrap code ******************************/
%PARAM_SKIP_END%


/**
 * Set function name alias prefix.
 */
#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)
# define GDO_FUNC_ALIAS(x) GDO_WRAP_##x
#else
# define GDO_FUNC_ALIAS(x) GDO_RAWPTR_##x
#endif


/**
 * Disable aliasing if we saved into separate files and the
 * header file was included from the body file.
 */
#if  defined(GDO_SEPARATE) && \
    !defined(GDO_INCLUDED_IN_BODY) && \
    !defined(GDO_DISABLE_ALIASING)

/* function name aliases */
#if !defined(GDO_WRAP_VISIBILITY)
# define %%func_symbol_pad%% GDO_FUNC_ALIAS(%%func_symbol%%)
#endif

/* object name aliases */
#define %%obj_symbol_pad%% *GDO_RAWPTR_%%obj_symbol%%

#endif //GDO_SEPARATE ...

