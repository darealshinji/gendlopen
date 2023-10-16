#if 0

/**************************************/
/*           quick overview           */
/**************************************/


/* class */
class gdo::dl;


/*** constructors ***/

    /* empty c'tor */
    gdo::dl();

    /* set filename, flags and whether to use a new namespace */
    gdo::dl(const char *filename, int flags=default_flags, bool new_namespace=false);


/*** constants ***/

    /* API-agnostic default flags */
    static constexpr int default_flags;

    /* Shared library file extension without dot ("dll", "dylib" or "so").
     * Useful i.e. on plugins. */
    static constexpr const char *libext;


/*** static member functions ***/

    /* set a custom callback function for error messages */
    void set_callback(void (*callback)(const char *));


/*** member functions ***/

    /* load the library that was set by the c'tor;
     * if nothing was given it will use NULL as filename to load */
    bool load();

    /* load filename; set flags and whether to use a new namespace */
    bool load(const char *filename, int flags=default_flags, bool new_namespace=false);

    /* wide character variants for win32 */
#ifdef GDO_WINAPI
    bool load(const WCHAR *filename, int flags=default_flags);
#endif

    /* check if library was successfully loaded */
    bool lib_loaded() const;

    /* return the flags used to load the library */
    int flags() const;

    /* load all symbols */
    bool load_symbols();

    /* check if symbols were successfully loaded */
    bool symbols_loaded() const;

    /* free library */
    bool free();

    /* get full path of loaded library */
    std::string origin();
#ifdef GDO_WINAPI
    std::wstring origin_w();
#endif

    /* retrieve the last error message */
    std::string error();
#ifdef GDO_WINAPI
    std::wstring error_w();
#endif

/***************************************/

#endif //0


/***

****************************************************
* The following options may be set through macros: *
****************************************************

GDO_USE_DLOPEN
    If defined `dlopen()' API is used on win32 targets.
    On other targets `dlopen()' is always used.

GDO_NO_DLMOPEN
    If defined `dlmopen()` will never be used.

GDO_DEFAULT_LIB
    Set a default library name through this macro (including double quote
    marks). This macro must be defined if you want to set GDO_USE_WRAPPER.

GDO_USE_WRAPPER
    Define this macro if you want to use auto-loading wrapper functions.
    This means you don't need to explicitly call library load functions.
    It requires GDO_DEFAULT_LIB to be defined.
    If an error occures during loading these functions throw an error message
    and call `std::abort()'!



*****************
* Helper macros *
*****************

GDO_LIB(NAME, API)
    Convenience macro to create versioned library names for DLLs, dylibs and DSOs,
    including double quote marks.
    GDO_LIB(z,1) for example will become "libz-1.dll", "libz.1.dylib" or "libz.so.1".

***/


#include <iostream>
#include <string>

GDO_COMMON

GDO_TYPEDEFS


namespace gdo
{

class dl
{
public:

    /* default flags */
#ifdef GDO_WINAPI
    static constexpr const int default_flags = 0;
#else
    static constexpr const int default_flags = RTLD_LAZY;
#endif

    /* Shared library file extension without dot ("dll", "dylib" or "so").
     * Useful i.e. on plugins. */
    static constexpr const char *libext = GDO_LIBEXT;

    typedef void (*callback_t)(const char *);


    /* object pointers */
    static GDO_OBJ_TYPE *GDO_OBJ_SYMBOL_ptr_;


private:

    /* function typedefs */
    typedef GDO_TYPE (*GDO_SYMBOL_t)(GDO_ARGS);

    /* function pointers */
    static GDO_SYMBOL_t GDO_SYMBOL_ptr_;


#ifdef GDO_WINAPI
    HMODULE m_handle = NULL;
    DWORD m_last_error = 0;
#else
    void *m_handle = NULL;
    std::string m_last_error;
#endif

    static callback_t m_callback;

    const char *m_filename = NULL;
    int m_flags = default_flags;
    bool m_new_namespace = false;
    bool m_symbols_loaded = false;


    /* clear error */
    void clear_error()
    {
#ifdef GDO_WINAPI
        m_last_error = 0;
        ::SetLastError(0);
#else
        m_last_error = "";
        ::dlerror();
#endif
    }


    /* save last error */
    void save_error()
    {
#ifdef GDO_WINAPI
        m_last_error = ::GetLastError();
#else
        char *ptr = ::dlerror();
        m_last_error = ptr ? ptr : "";
#endif
    }


    /* if m_handle is NULL */
    void set_error_invalid_handle()
    {
        clear_error();

#ifdef GDO_WINAPI
        m_last_error = ERROR_INVALID_HANDLE;
#else
        m_last_error = "no library was loaded";
#endif
    }


    /* load library */
    void load_lib(const char *filename, int flags, bool new_namespace)
    {
        m_flags = flags;

#if defined(GDO_WINAPI)
        /* win32 */
        (void)new_namespace;
        m_handle = ::LoadLibraryExA(filename, NULL, m_flags);
#elif defined(GDO_NO_DLMOPEN)
        /* dlmopen() disabled */
        (void)new_namespace;
        m_handle = ::dlopen(filename, m_flags);
#else
        /* dlmopen() for new namespace or dlopen() */
        if (new_namespace) {
            m_handle = ::dlmopen(LM_ID_NEWLM, filename, m_flags);
        } else {
            m_handle = ::dlopen(filename, m_flags);
        }
#endif
    }


    /* load symbol address */
    template<typename T>
    bool sym(T &ptr, const char *symbol)
    {
#ifdef GDO_WINAPI
        ptr = reinterpret_cast<T>(::GetProcAddress(m_handle, symbol));
#else
        ptr = reinterpret_cast<T>(::dlsym(m_handle, symbol));
#endif

        if (!ptr) {
            save_error();
            return false;
        }

        return true;
    }


    /* free library handle */
    bool free_lib()
    {
#ifdef GDO_WINAPI
        return (::FreeLibrary(m_handle) == TRUE);
#else
        return (::dlclose(m_handle) == 0);
#endif
    }


public:

    /* c'tor (empty) */
    dl()
    {}


    /* c'tor (set filename) */
    dl(const char *filename, int flags=default_flags, bool new_namespace=false)
    {
        m_filename = filename;
        m_flags = flags;
        m_new_namespace = new_namespace;
    }


    /* d'tor */
    ~dl()
    {
        if (lib_loaded()) {
            free_lib();
        }
    }


    /* set a custom callback function for error messages */
    static void set_callback(callback_t callback)
    {
        m_callback = callback;
    }


    /* load library */
    bool load(const char *filename, int flags=default_flags, bool new_namespace=false)
    {
        clear_error();

        if (lib_loaded()) {
            return true;
        }

        load_lib(filename, flags, new_namespace);
        save_error();

        return lib_loaded();
    }


#ifdef GDO_WINAPI
    /* load library (wide characters version) */
    bool load(const WCHAR *filename, int flags=default_flags)
    {
        clear_error();

        if (lib_loaded()) {
            return true;
        }

        m_flags = flags;
        m_handle = ::LoadLibraryExW(filename, NULL, m_flags);
        save_error();

        return lib_loaded();
    }
#endif


    /* load library */
    bool load()
    {
        return load(m_filename, m_flags, m_new_namespace);
    }


    /* check if library is loaded */
    bool lib_loaded() const
    {
        return (m_handle != NULL);
    }


    /* return the flags used to load the library */
    int flags() const
    {
        return m_flags;
    }


    /* load all symbols */
    bool load_symbols()
    {
        if (m_symbols_loaded) {
            clear_error();
            return true;
        } else if (!lib_loaded()) {
            set_error_invalid_handle();
            return false;
        }

        /* load function pointer addresses */
        if (!sym<GDO_SYMBOL_t>(GDO_SYMBOL_ptr_, "GDO_SYMBOL")) {@
            return false;@
        }

        /* load object addresses */
        if (!sym<GDO_OBJ_TYPE *>(GDO_OBJ_SYMBOL_ptr_, "GDO_OBJ_SYMBOL")) {@
            return false;@
        }

        m_symbols_loaded = true;
        clear_error();

        return true;
    }


    /* check if symbols are loaded */
    bool symbols_loaded() const
    {
        return m_symbols_loaded;
    }


    /* free library */
    bool free()
    {
        if (!lib_loaded()) {
            clear_error();
            return true;
        }

        bool ret = free_lib();
        save_error();

        if (!ret) {
            return false;
        }

        m_handle = NULL;

        GDO_SYMBOL_ptr_ = nullptr;
        GDO_OBJ_SYMBOL_ptr_ = nullptr;

        return true;
    }


    /* get path of loaded library */
    std::string origin()
    {
        if (!lib_loaded()) {
            set_error_invalid_handle();
            return "";
        }

#ifdef GDO_WINAPI
        char buf[32*1024] = {0};

        DWORD ret = ::GetModuleFileNameA(m_handle, reinterpret_cast<LPSTR>(&buf), sizeof(buf)-1);
        save_error();

        return (ret == 0) ? "" : buf;
#else
        struct link_map *lm = NULL;

        int ret = ::dlinfo(m_handle, RTLD_DI_LINKMAP, reinterpret_cast<void *>(&lm));
        save_error();

        if (ret != -1 && lm->l_name) {
            return lm->l_name;
        }

        return "";
#endif //GDO_WINAPI
    }


#ifdef GDO_WINAPI
    /* get path of loaded library (wide characters version) */
    std::wstring origin_w()
    {
        WCHAR buf[32*1024] = {0};

        if (!lib_loaded()) {
            set_error_invalid_handle();
            return L"";
        }

        DWORD ret = ::GetModuleFileNameW(m_handle, reinterpret_cast<LPWSTR>(&buf), sizeof(buf)-1);
        save_error();

        return (ret == 0) ? L"" : buf;
    }
#endif //GDO_WINAPI


    /* retrieve the last error */
    std::string error()
    {
#ifdef GDO_WINAPI
        std::string buf;
        char *msg = NULL;
        const DWORD dwFlags =
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS |
            FORMAT_MESSAGE_MAX_WIDTH_MASK;

        const DWORD ret = ::FormatMessageA(dwFlags, NULL, m_last_error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPSTR>(&msg), 0, NULL);

        if (ret == 0 || !msg) {
            /* FormatMessageA failed */
            buf = "Last saved error code: ";
            buf += std::to_string(m_last_error);
            if (msg) ::LocalFree(msg);
        } else if (msg) {
            buf = msg;
            ::LocalFree(msg);
        }

        return buf;
#else
        return m_last_error;
#endif //GDO_WINAPI
    }


#ifdef GDO_WINAPI
    /* retrieve the last error (wide characters version) */
    std::wstring error_w()
    {
        std::wstring buf;
        WCHAR *msg = NULL;
        const DWORD dwFlags =
            FORMAT_MESSAGE_ALLOCATE_BUFFER |
            FORMAT_MESSAGE_FROM_SYSTEM |
            FORMAT_MESSAGE_IGNORE_INSERTS |
            FORMAT_MESSAGE_MAX_WIDTH_MASK;

        const DWORD ret = ::FormatMessageW(dwFlags, NULL, m_last_error,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
            reinterpret_cast<LPWSTR>(&msg), 0, NULL);

        if (ret == 0 || !msg) {
            /* FormatMessageW failed */
            buf = L"Last saved error code: ";
            buf += std::to_wstring(m_last_error);
            if (msg) ::LocalFree(msg);
        } else if (msg) {
            buf = msg;
            ::LocalFree(msg);
        }

        return buf;
    }
#endif //GDO_WINAPI


    /* used internally but must be set to public */
    static void _print_error(const char *msg)
    {
        if (m_callback) {
            m_callback(msg);
        } else {
            std::cerr << msg << std::endl;
        }
    }


private:

    static void function_ptr_is_null(const char *symbol)
    {
        std::string msg = "error: function pointer `";
        if (symbol) msg += symbol;
        msg += "' is NULL";

        _print_error(msg.c_str());
        std::abort();
    }

public:

    /* wrapped functions */

    static GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
        if (!GDO_SYMBOL_ptr_) function_ptr_is_null("GDO_SYMBOL");@
        GDO_RET GDO_SYMBOL_ptr_(GDO_NOTYPE_ARGS);@
    }@

#undef _GDO_CHECK_FUNCPTR

}; /* class dl */


#ifdef GDO_USE_WRAPPER

#if !defined(GDO_DEFAULT_LIB)
#error  GDO_DEFAULT_LIB was not defined!
#endif

    auto _al = dl();

    void _autoload(const char *symbol)
    {
        if (!_al.load(GDO_DEFAULT_LIB)) {
            std::string msg = "error loading library `" GDO_DEFAULT_LIB "':\n" + _al.error();
            dl::_print_error(msg.c_str());
            std::exit(1);
        } else if (!_al.load_symbols()) {
            std::string msg = "";

            if (symbol) {
                msg += "error in auto-loading wrapper function `";
                msg += symbol;
                msg += "': ";
            }
            msg += _al.error();

            dl::_print_error(msg.c_str());
            std::exit(1);
        }
    }


    /* auto-loading wrapper functions */

    GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
        _autoload("GDO_SYMBOL");@
        GDO_RET dl::GDO_SYMBOL(GDO_NOTYPE_ARGS);@
    }@

#endif // GDO_USE_WRAPPER

} /* namespace gdo */


/* aliases */

#ifdef GDO_USE_WRAPPER

/* use auto-loading wrapper functions */
#define GDO_SYMBOL gdo::GDO_SYMBOL

#else

/* use class-internal wrapped functions */
#define GDO_SYMBOL gdo::dl::GDO_SYMBOL

#endif // GDO_USE_WRAPPER


/* object pointers */
#define GDO_OBJ_SYMBOL *gdo::dl::GDO_OBJ_SYMBOL_ptr_

