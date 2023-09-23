#if 0

/**************************************/
/*           quick overview           */
/**************************************/

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
    If defined use `dlopen()' API on win32 targets.
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

/* print error message */
void _print_error(const std::string &msg)
{
    /* check for _WIN32 and NOT for GDO_WINAPI, so that
     * we can use the message box on dlopen() too */
#if defined(_WIN32) && defined(GDO_USE_MESSAGE_BOX)
    MessageBoxA(NULL, msg.c_str(), "Error", MB_OK | MB_ICONERROR);
#else
    std::cerr << msg << std::endl;
#endif
}


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


private:

    /* function typedefs */
    typedef GDO_TYPE (*GDO_SYMBOL_t)(GDO_ARGS);

    /* function pointers */
    static GDO_SYMBOL_t GDO_SYMBOL_ptr_;


#ifdef GDO_WINAPI
    static constexpr const DWORD m_fmtmsg_flags =
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_MAX_WIDTH_MASK;

    HMODULE m_handle = NULL;
    DWORD m_last_error = 0;
#else
    void *m_handle = NULL;
    std::string m_last_error;
#endif

    const char *m_filename = NULL;
    int m_flags = default_flags;
    bool m_new_namespace = false;
    bool m_symbols_loaded = false;


    /* clear error;
     * run this at the top of a function that
     * has a libloaderapi or dlfcn API call */
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


    /* save error;
     * run this immediately after a
     * libloaderapi or dlfcn API call */
    void save_error()
    {
#ifdef GDO_WINAPI
        m_last_error = ::GetLastError();
#else
        char *ptr = ::dlerror();
        m_last_error = ptr ? ptr : "";
#endif
    }


    /* load library (convenience wrapper) */
    bool load_(const char *filename, int flags, bool new_namespace)
    {
        return (new_namespace == true)
            ? load_new_namespace(filename, flags)
            : load_default(filename, flags);
    }


    /* load library (default version) */
    bool load_default(const char *filename, int flags)
    {
        clear_error();

        if (m_handle) return true;

#ifdef GDO_WINAPI
        m_handle = ::LoadLibraryExA(filename, NULL, flags);
#else
        m_handle = ::dlopen(filename, flags);
#endif

        save_error();

        return (m_handle != NULL);
    }


    /* load library into new namespace */
    bool load_new_namespace(const char *filename, int flags)
    {
#if defined(GDO_WINAPI) || defined(GDO_NO_DLMOPEN)
        /* no namespace option here */
        return load_default(filename, flags);
#else
        /* use dlmopen() */

        clear_error();

        if (m_handle) return true;

        m_handle = ::dlmopen(LM_ID_NEWLM, filename, flags);
        save_error();

        return (m_handle != NULL);
#endif
    }


    /* load symbol address */
    template<typename T>
    T sym(const char *symbol)
    {
        clear_error();

        if (!m_handle) std::abort();

#ifdef GDO_WINAPI
        T ptr = reinterpret_cast<T>(::GetProcAddress(m_handle, symbol));
#else
        T ptr = reinterpret_cast<T>(::dlsym(m_handle, symbol));
#endif

        save_error();

        return ptr;
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
#ifdef GDO_WINAPI
        if (m_handle) ::FreeLibrary(m_handle);
#else
        if (m_handle) ::dlclose(m_handle);
#endif
    }


    /* load library */
    bool load(const char *filename, int flags=default_flags, bool new_namespace=false)
    {
        return load_(filename, flags, new_namespace);
    }


#ifdef GDO_WINAPI
    /* load library (wide characters version) */
    bool load(const WCHAR *filename, int flags=default_flags)
    {
        clear_error();

        if (m_handle) return true;
        m_handle = ::LoadLibraryExW(filename, NULL, flags);

        save_error();

        return (m_handle != NULL);
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


    /* load all symbols */
    bool load_symbols()
    {
        if (m_symbols_loaded) {
            return true;
        } else if (!m_handle) {
            clear_error();

#ifdef GDO_WINAPI
            m_last_error = ERROR_INVALID_HANDLE;
#else
            m_last_error = "no library was loaded";
#endif
            return false;
        }

#define LOAD_SYM(SYM) \
    if (!SYM##_ptr_) SYM##_ptr_ = sym<SYM##_t>(#SYM); \
    if (!SYM##_ptr_) return false

        /* load function pointer addresses */
        LOAD_SYM(GDO_SYMBOL);

#undef LOAD_SYM

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
        clear_error();

        if (!m_handle) return true;

#ifdef GDO_WINAPI
        bool ret = (::FreeLibrary(m_handle) == TRUE);
#else
        bool ret = (::dlclose(m_handle) == 0);
#endif

        save_error();

        if (ret == true) {
            m_handle = NULL;
            return true;
        }

        return false;
    }


    /* get path of loaded library */
    std::string origin()
    {
        clear_error();

        if (!m_handle) return "";

#ifdef GDO_WINAPI
        char buf[36*1024] = {0};

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
        clear_error();

        if (!m_handle) return L"";

        WCHAR buf[36*1024] = {0};

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

        const DWORD ret = ::FormatMessageA(m_fmtmsg_flags, NULL, m_last_error,
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

        const DWORD ret = ::FormatMessageW(m_fmtmsg_flags, NULL, m_last_error,
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


private:

    static void function_ptr_is_null(const char *symbol)
    {
        std::string msg = "error: function pointer `";
        if (symbol) msg += symbol;
        msg += "' is NULL";

        _print_error(msg);
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
            _print_error(msg);
            std::exit(1);
        } else if (!_al.load_symbols()) {
            std::string msg = "";

            if (symbol) {
                msg += "error in auto-loading wrapper function `";
                msg += symbol;
                msg += "': ";
            }
            msg += _al.error();

            _print_error(msg);
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
#ifndef GDO_SYMBOL@
#define GDO_SYMBOL gdo::GDO_SYMBOL@
#endif

#else

/* use class-internal wrapped functions */
#ifndef GDO_SYMBOL@
#define GDO_SYMBOL gdo::dl::GDO_SYMBOL@
#endif

#endif // GDO_USE_WRAPPER
