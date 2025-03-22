#if 0

/* summary */

namespace gdo
{

/* default flags */
constexpr const int default_flags;


/* Shared library file extension without dot ("dll", "dylib" or "so").
 * Useful i.e. on plugins. */
constexpr const char * const libext;
#ifdef GDO_WINAPI
constexpr const wchar_t * const libext_w;
#endif


/* Create versioned library names for DLLs, dylibs and DSOs.
 * make_libname("z",1) for example will return "libz-1.dll", "libz.1.dylib" or "libz.so.1" */
std::string make_libname(const std::string &name, const size_t api);
#ifdef GDO_WINAPI
std::wstring make_libname(const std::wstring &name, const size_t api);
#endif


class dl
{
public:

    using message_callback_t = void (*)(const char *);


    /* c'tor */
    dl();
    dl(const std::string &filename, int flags=default_flags, bool new_namespace=false);
#ifdef GDO_WINAPI
    dl(const std::wstring &filename, int flags=default_flags, bool new_namespace=false);
#endif


    /* d'tor */
    ~dl();


    /* Load the library; `filename' and `flags' are passed to the underlying library
     * loading functions.
     *
     * If `new_namespace' is true the library will be loaded into a new namespace.
     * This is done using dlmopen() (if available on the platform) with the
     * LM_ID_NEWLM argument. */
    bool load(const std::string &filename, int flags=default_flags, bool new_namespace=false);
#ifdef GDO_WINAPI
    bool load(const std::wstring &filename, int flags=default_flags, bool new_namespace=false);
#endif


    /* Load the library.
     * Filename and flags must have been set with the the constructor. */
    bool load();


    /* Load library from a list.
     * T should be of a container type like std::vector, std::list, std::array, ... */
    bool load_from_list(const T &list, int flags=default_flags, bool new_namespace=false)


    /* Load library and symbols.
     * Filename and flags must have been set with the the constructor. */
    bool load_lib_and_symbols();


    /* check if the library is loaded */
    bool lib_loaded();


    /* return the flags used to load the library */
    int flags();


    /* Load the symbols. This function can safely be called multiple times.
     * If one or more symbols weren't loaded the function returns false. */
    bool load_all_symbols();


    /* Load a specific symbol.
     * `symbol_num' is an enumeration value: `GDO_LOAD_<symbol_name>' */
    bool load_symbol(int symbol_num);


    /* check if ALL symbols were loaded */
    bool all_symbols_loaded();


    /* check if NO symbols were loaded */
    bool no_symbols_loaded();


    /* check if ANY symbol was loaded */
    bool any_symbol_loaded();


    /* free library */
    bool free();


    /* whether to free the library in the class destructor */
    void free_lib_in_dtor(bool b);


#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

    /* Set a message callback function to be used if an error occurred
     * in a wrapped function. */
    static void message_callback(message_callback_t cb);


    /* Get a pointer to the message callback function. */
    static message_callback_t message_callback();

#endif // GDO_WRAP_FUNCTIONS || GDO_ENABLE_AUTOLOAD


    /* get path of loaded library */
    std::string origin();
#ifdef GDO_WINAPI
    std::wstring origin_w();
#endif

    /* retrieve the last error */
#ifdef GDO_WINAPI
    std::string error();
    std::wstring error_w();
#else
    std::string error() const;
#endif

    /* get filename passed to load */
#ifdef GDO_WINAPI
    std::string filename();
    std::wstring filename_w();
#else
    std::string filename() const;
#endif

}; /* class */

} /* namespace */

#endif // 0


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
    Always disable usage of `dlinfo(3)'.

GDO_DISABLE_DLMOPEN
    Always disable usage of `dlmopen(3)'.



*****************
* Helper macros *
*****************

GDO_DEFAULT_FLAGS
    Default flags for `dlopen()' or `LoadLibraryEx()'

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
      printf("debug: the sum of %d and %d is %d\n", val_a, val_b, GDO_ALIAS_sum_of_a_and_b(__VA_ARGS__));

***/

#ifdef GDO_WINAPI
# include <cstdlib>
# include <cstring>
#endif
#ifdef _AIX
# include <cerrno>
# include <cstring>
#endif
#include <string>

#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)
# define GDO_HAS_MSG_CB
#endif


/* enumeration values for `load_symbol()' method */
enum {
    GDO_LOAD_%%symbol%%,
    GDO_ENUM_LAST
};


namespace gdo
{

/* silence `unused' compiler warnings, basically
 * the same as `#define UNUSED_VAL_(x) (void)x' */
template<typename T>
void UNUSED_VAL_(T val) {
    static_cast<void>(val);
}


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
constexpr const char * const libext = LIBEXTA;
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
public:

    using message_callback_t = void (*)(const char *);

private:

#ifdef GDO_HAS_MSG_CB
    /* function pointer to error message callback */
    static message_callback_t m_message_callback;
#endif

    std::string m_filename;
#ifdef GDO_WINAPI
    std::wstring m_wfilename;
#endif
    int m_flags = default_flags;
    bool m_new_namespace = false;
    bool m_free_lib_in_dtor = true;

#ifdef GDO_WINAPI

    template<typename T>
    T function_cast(FARPROC proc) {
        /* cast to void* to supress compiler warnings */
        return reinterpret_cast<T>(reinterpret_cast<void *>(proc));
    }

    /* library handle */
    using handle_t = HMODULE;
    static handle_t m_handle;

    /* error message */
    DWORD m_last_error = 0;
    std::string m_errmsg;
    std::wstring m_werrmsg;


    /* wstring to string */
    static std::string wstr_to_str(const std::wstring &wstr)
    {
        size_t len, n;
        char *buf;
        std::string str;

        if (wstr.empty()) {
            return {};
        }

        if (::wcstombs_s(&len, nullptr, 0, wstr.c_str(), 0) != 0 || len == 0) {
            return {};
        }

        buf = new char[len + 1];
        if (!buf) return {};

        if (::wcstombs_s(&n, buf, len+1, wstr.c_str(), len) != 0 || n == 0) {
            return {};
        }

        buf[len] = '\0';
        str = buf;
        delete[] buf;

        return str;
    }


    /* string to wstring */
    static std::wstring str_to_wstr(const std::string &str)
    {
        size_t len, n;
        wchar_t *buf;
        std::wstring wstr;

        if (str.empty()) {
            return {};
        }

        if (::mbstowcs_s(&len, nullptr, 0, str.c_str(), 0) != 0 || len == 0) {
            return {};
        }

        buf = new wchar_t[(len + 1) * sizeof(wchar_t)];
        if (!buf) return {};

        if (::mbstowcs_s(&n, buf, len+1, str.c_str(), len) != 0 || n == 0) {
            delete[] buf;
            return {};
        }

        buf[len] = L'\0';
        wstr = buf;
        delete[] buf;

        return wstr;
    }


    /* clear error */
    void clear_error()
    {
        m_errmsg.clear();
        m_werrmsg.clear();
        m_last_error = 0;
    }


    /* save last error (no extra message) */
    void save_error()
    {
        m_last_error = ::GetLastError();
        m_errmsg.clear();
        m_werrmsg.clear();
    }


    /* save last error (narrow char message) */
    void save_error(const std::string &msg)
    {
        m_last_error = ::GetLastError();
        m_errmsg = msg;
        m_werrmsg.clear();
    }


    /* save last error (wide char message) */
    void save_error(const std::wstring &msg)
    {
        m_last_error = ::GetLastError();
        m_errmsg.clear();
        m_werrmsg = msg;
    }


    /* if m_handle is NULL */
    void set_error_invalid_handle()
    {
        clear_error();
        m_last_error = ERROR_INVALID_HANDLE;
    }


    /* if filename is empty */
    void set_error_empty_filename()
    {
        clear_error();
        m_last_error = ERROR_INVALID_NAME;
        m_errmsg = "empty filename";
    }


    /* load library */
    void load_lib(const std::string &filename, int flags, bool new_namespace)
    {
        UNUSED_VAL_(new_namespace);

        m_wfilename.clear();
        m_filename = filename;
        m_flags = flags;

        m_handle = ::LoadLibraryExA(filename.c_str(), NULL, m_flags);
    }


    /* load library (wide character) */
    void load_lib(const std::wstring &filename, int flags, bool new_namespace)
    {
        UNUSED_VAL_(new_namespace);

        m_filename.clear();
        m_wfilename = filename;
        m_flags = flags;

        m_handle = ::LoadLibraryExW(filename.c_str(), NULL, m_flags);
    }


    /* free library handle */
    bool free_lib()
    {
        return (::FreeLibrary(m_handle) == TRUE);
    }


    /* load symbol address */
    template<typename T>
    T sym_load(const char *symbol)
    {
        clear_error();

        T proc = function_cast<T>(::GetProcAddress(m_handle, symbol));

        if (!proc) {
            save_error(symbol);
        }

        return proc;
    }


    DWORD get_module_filename(HMODULE handle, wchar_t *buf, DWORD len) {
        return ::GetModuleFileNameW(handle, buf, len);
    }

    DWORD get_module_filename(HMODULE handle, char *buf, DWORD len) {
        return ::GetModuleFileNameA(handle, buf, len);
    }


    /* get the module's full path using GetModuleFileName() */
    template<typename T>
    std::basic_string<T> get_origin_from_module_handle()
    {
        if (!lib_loaded()) {
            set_error_invalid_handle();
            return {};
        }

        DWORD len = 260; /* MAX_PATH */
        T *origin = new T[len * sizeof(T)]();
        ::memset(origin, 0, len * sizeof(T));

        if (get_module_filename(m_handle, origin, len-1) == 0) {
            save_error();
            delete[] origin;
            return {};
        }

        /* https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
         * technically the path could exceed 260 characters, but in reality
         * it's practically still stuck at the old MAX_PATH value */
        if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            delete[] origin;

            len = 32*1024;
            origin = new T[len * sizeof(T)]();
            ::memset(origin, 0, len * sizeof(T));

            if (get_module_filename(m_handle, origin, len-1) == 0) {
                save_error();
                delete[] origin;
                return {};
            }
        }

        std::basic_string<T> str = origin;
        delete[] origin;

        return str;
    }


    DWORD format_message(DWORD flags, DWORD msgId, LPWSTR buf) {
        return ::FormatMessageW(flags, NULL, msgId, 0, buf, 0, NULL);
    }

    DWORD format_message(DWORD flags, DWORD msgId, LPSTR buf) {
        return ::FormatMessageA(flags, NULL, msgId, 0, buf, 0, NULL);
    }


    /* return a formatted error message */
    template<typename T>
    std::basic_string<T> format_last_error_message()
    {
        std::basic_string<T> str;
        T *buf = nullptr;

        const DWORD flags = FORMAT_MESSAGE_ALLOCATE_BUFFER |
                            FORMAT_MESSAGE_FROM_SYSTEM |
                            FORMAT_MESSAGE_MAX_WIDTH_MASK;

        format_message(flags, m_last_error, reinterpret_cast<T*>(&buf));

        if (buf) {
            str = buf;
            ::LocalFree(buf);
        }

        return str;
    }


#else
/*********************************** dlfcn ***********************************/


    /* library handle */
    using handle_t = void*;
    static handle_t m_handle;

    /* error message */
    std::string m_errmsg;


    /* clear error */
    void clear_error()
    {
        m_errmsg.clear();
        ::dlerror();
    }


    /* save last error */
    void save_error()
    {
        auto ptr = ::dlerror();
        m_errmsg = ptr ? ptr : "";
    }

    void save_error(const std::string&)
    {
        save_error();
    }


    /* if m_handle is NULL */
    void set_error_invalid_handle()
    {
        clear_error();
        m_errmsg = "no library was loaded";
    }


    /* if filename is empty */
    void set_error_empty_filename()
    {
        clear_error();
        m_errmsg = "empty filename";
    }


    /* load library */
    void load_lib(const std::string &filename, int flags, bool new_namespace)
    {
        m_flags = flags;
        m_filename = filename;

#ifdef GDO_HAVE_DLMOPEN
        /* dlmopen() for new namespace or dlopen() */
        if (new_namespace) {
            m_handle = ::dlmopen(LM_ID_NEWLM, filename.c_str(), m_flags);
        } else {
            m_handle = ::dlopen(filename.c_str(), m_flags);
        }
#else
        /* no dlmopen() */
        UNUSED_VAL_(new_namespace);
        m_handle = ::dlopen(filename.c_str(), m_flags);
#endif
    }


    /* free library handle */
    bool free_lib()
    {
        return (::dlclose(m_handle) == 0);
    }


    /* load symbol address */
    template<typename T>
    T sym_load(const char *symbol)
    {
        clear_error();

        T ptr = reinterpret_cast<T>(::dlsym(m_handle, symbol));

        if (!ptr) {
            save_error();
        }

        return ptr;
    }

#endif // !GDO_WINAPI


    /* load library by filename */
    template<typename T>
    bool load_filename(const T &filename, int flags, bool new_namespace)
    {
        /* release old libhandle */
        if (lib_loaded() && !free()) {
            return false;
        }

        if (filename.empty()) {
            set_error_empty_filename();
            return false;
        }

        clear_error();

#ifdef _AIX
        errno = 0;
        load_lib(filename, flags, new_namespace);
        int errsav = errno;

        if (!lib_loaded()) {
            const char *ptr = (errsav == ENOEXEC) ? ::dlerror() : ::strerror(errsav);
            m_errmsg = ptr ? ptr : "";
            return false;
        }

        return true;
#else
        load_lib(filename, flags, new_namespace);
        save_error(filename);

        return lib_loaded();
#endif //!_AIX
    }


public:

    /* c'tor (empty) */
    dl()
    {}


    /* c'tor (set filename) */
    dl(const std::string &filename, int flags=default_flags, bool new_namespace=false)
      : m_filename(filename),
        m_flags(flags),
        m_new_namespace(new_namespace)
    {}


#ifdef GDO_WINAPI
    /* c'tor (set wide character filename) */
    dl(const std::wstring &filename, int flags=default_flags, bool new_namespace=false)
      : m_wfilename(filename),
        m_flags(flags)
    {
        UNUSED_VAL_(new_namespace);
    }
#endif


    /* d'tor */
    ~dl()
    {
        if (m_free_lib_in_dtor && lib_loaded()) {
            free_lib();
        }
    }


    /* load library */
    bool load(const std::string &filename, int flags=default_flags, bool new_namespace=false)
    {
        return load_filename(filename, flags, new_namespace);
    }


#ifdef GDO_WINAPI
    /* load library (wide characters version) */
    bool load(const std::wstring &filename, int flags=default_flags, bool new_namespace=false)
    {
        return load_filename(filename, flags, new_namespace);
    }
#endif


    /* load library */
    bool load()
    {
#ifdef GDO_WINAPI
        if (m_filename.empty() && !m_wfilename.empty()) {
            return load(m_wfilename, m_flags, m_new_namespace);
        }
#endif
        return load(m_filename, m_flags, m_new_namespace);
    }


    /* load from a container list of filenames (std::vector, std::list, etc.) */
    template<class T>
    bool load_from_list(const T &list, int flags=default_flags, bool new_namespace=false)
    {
        for (const auto &e : list) {
            if (load(e, flags, new_namespace)) {
                return true;
            }
        }

        return false;
    }


    /* load library and symbols */
    bool load_lib_and_symbols()
    {
        return (load() && load_all_symbols());
    }


    /* check if library is loaded */
    bool lib_loaded() const
    {
        return (m_handle != nullptr);
    }


    /* return the flags used to load the library */
    int flags() const
    {
        return m_flags;
    }


    /* load all symbols */
    bool load_all_symbols()
    {
        clear_error();

        if (all_symbols_loaded()) {
            return true;
        } else if (!lib_loaded()) {
            set_error_invalid_handle();
            return false;
        }

        /* get symbol addresses */

        /* %%symbol%% */@
        if ((ptr::%%symbol%% =@
            sym_load<%%sym_type%%>@
                ("%%symbol%%")) == nullptr) {@
            return false;@
        }@

        clear_error();

        return all_symbols_loaded();
    }


    /* load a specific symbol */
    bool load_symbol(int symbol_num)
    {
        clear_error();

        if (!lib_loaded()) {
            set_error_invalid_handle();
            return false;
        }

        switch (symbol_num)
        {
        /* %%symbol%% */@
        case GDO_LOAD_%%symbol%%:@
            ptr::%%symbol%% =@
                sym_load<%%sym_type%%>@
                    ("%%symbol%%");@
            return (ptr::%%symbol%% != nullptr);@

        default:
            break;
        }

# ifdef GDO_WINAPI
        m_last_error = ERROR_NOT_FOUND;
# endif
        m_errmsg = "unknown symbol number: " + std::to_string(symbol_num);

        return false;
    }


    /* load a specific symbol by name */
    bool load_symbol(const char *symbol)
    {
        clear_error();

        if (!lib_loaded()) {
            set_error_invalid_handle();
            return false;
        }

        if (!symbol || *symbol == 0) {
#ifdef GDO_WINAPI
            m_last_error = ERROR_INVALID_PARAMETER;
#endif
            m_errmsg = "empty symbol name";
        } else {
            /* jumps to `GDO_JUMP_<..>' label if symbol was found */
            GDO_CHECK_SYMBOL_NAME(symbol);

#ifdef GDO_WINAPI
            m_last_error = ERROR_NOT_FOUND;
#endif
            m_errmsg = "unknown symbol: ";
            m_errmsg += symbol;
        }

        return false;

        /* jump labels */
@
        /* %%symbol%% */@
    GDO_JUMP_%%symbol%%:@
        ptr::%%symbol%% =@
            sym_load<%%sym_type%%>@
                ("%%symbol%%");@
        return (ptr::%%symbol%% != nullptr);
    }


    /* check if ALL symbols were loaded */
    bool all_symbols_loaded() const
    {
        if (true
            && ptr::%%symbol%% != nullptr
        ) {
            return true;
        }

        return false;
    }


    /* check if NO symbols were loaded */
    bool no_symbols_loaded() const
    {
        if (true
            && ptr::%%symbol%% == nullptr
        ) {
            return true;
        }

        return false;
    }


    /* check if ANY symbol was loaded */
    bool any_symbol_loaded() const
    {
        if (false
            || ptr::%%symbol%% != nullptr
        ) {
            return true;
        }

        return false;
    }


    /* free library */
    bool free()
    {
        clear_error();

        if (!lib_loaded()) {
            return true;
        }

        bool ret = free_lib();
        save_error();

        if (!ret) {
            return false;
        }

        m_handle = nullptr;

        ptr::%%symbol%% = nullptr;

        return true;
    }


    /* whether to free the library in the class destructor */
    void free_lib_in_dtor(bool b)
    {
        m_free_lib_in_dtor = b;
    }


#ifdef GDO_HAS_MSG_CB

    /* Set a message callback function. */
    static void message_callback(message_callback_t cb)
    {
        m_message_callback = cb;
    }


    /* Get a pointer to the message callback function. */
    static message_callback_t message_callback()
    {
        return m_message_callback;
    }

#endif //GDO_HAS_MSG_CB


#ifdef GDO_WINAPI

    /* get path of loaded library */
    std::string origin()
    {
        return get_origin_from_module_handle<char>();
    }

    std::wstring origin_w()
    {
        return get_origin_from_module_handle<wchar_t>();
    }


    /* retrieve the last error */
    std::string error()
    {
        std::string buf = format_last_error_message<char>();

        if (buf.empty()) {
            buf = "Last saved error code: " + std::to_string(m_last_error);
        }

        if (!m_errmsg.empty()) {
            buf.insert(0, ": ");
            buf.insert(0, m_errmsg);
        } else if (!m_werrmsg.empty()) {
            buf.insert(0, ": ");
            buf.insert(0, wstr_to_str(m_werrmsg));
        }

        return buf;
    }

    std::wstring error_w()
    {
        std::wstring buf = format_last_error_message<wchar_t>();

        if (buf.empty()) {
            buf = L"Last saved error code: " + std::to_wstring(m_last_error);
        }

        if (!m_werrmsg.empty()) {
            buf.insert(0, L": ");
            buf.insert(0, m_werrmsg);
        } else if (!m_errmsg.empty()) {
            buf.insert(0, L": ");
            buf.insert(0, str_to_wstr(m_errmsg));
        }

        return buf;
    }


    /* get filename passed to load */
    std::string filename()
    {
        return m_filename.empty() ? wstr_to_str(m_wfilename) : m_filename;
    }

    std::wstring filename_w()
    {
        return m_wfilename.empty() ? str_to_wstr(m_filename) : m_wfilename;
    }


#else
/*********************************** dlfcn ***********************************/


    /* get path of loaded library */
    std::string origin()
    {
        clear_error();

        if (!lib_loaded()) {
            set_error_invalid_handle();
            return {};
        }

#ifdef GDO_HAVE_DLINFO
        struct link_map *lm = nullptr;

        int ret = ::dlinfo(m_handle, RTLD_DI_LINKMAP, reinterpret_cast<void *>(&lm));
        save_error();

        return (ret != -1 && lm->l_name) ? lm->l_name : "";
#else
        /* use dladdr() to get the library path from a symbol pointer */
        std::string fname;

        if (no_symbols_loaded()) {
            m_errmsg = "no symbols were loaded";
            return {};
        }

        auto get_fname = [&fname] (const void *ptr)
        {
            Dl_info info;

            if (ptr && ::dladdr(ptr, &info) != 0 && info.dli_fname) {
                fname = info.dli_fname;
            }
        };

        get_fname(reinterpret_cast<void *>(ptr::%%symbol%%));@
        if (!fname.empty()) return fname;

        m_errmsg = "dladdr() failed to get library path";

        return {};
#endif // !GDO_HAVE_DLINFO
    }


    /* retrieve the last error */
    std::string error() const
    {
        return m_errmsg.empty() ? "no error" : m_errmsg;
    }


    /* get filename passed to load */
    std::string filename() const
    {
        return m_filename;
    }

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

