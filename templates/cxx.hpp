
#if 0

/**************************************/
/*           quick overview           */
/**************************************/

namespace gdo
{
    /* API-agnostic default flags */
    const int default_flags;

    /* Shared library file extension without dot ("dll", "dylib" or "so").
     * Useful i.e. on plugins. */
    const char * const libext;

    /* function pointer to error message callback */
    void (*message_callback)(const char *) = nullptr;

    /* Create versioned library names for DLLs, dylibs and DSOs.
     * libname("z",1) for example will return "libz-1.dll", "libz.1.dylib" or "libz.so.1" */
    const char *libname(const char *name, unsigned int api);


    class dl
    {
    /*** constructors ***/

        /* empty c'tor */
        dl();

        /* set filename, flags and whether to use a new namespace */
        dl(const char *filename, int flags=default_flags, bool new_namespace=false);


    /*** member functions ***/

        /* load the library that was set by the c'tor;
        * if nothing was given it will use NULL as filename to load */
        bool load();

        /* load filename; set flags and whether to use a new namespace */
        bool load(const char *filename, int flags=default_flags, bool new_namespace=false);

        /* wide character variants for win32 */
#ifdef GDO_WINAPI
        bool load(const wchar_t *filename, int flags=default_flags);
#endif

        /* load library and symbols */
        bool load_lib_and_symbols();

        /* check if library was successfully loaded */
        bool lib_loaded() const;

        /* return the flags used to load the library */
        int flags() const;

        /* load all symbols;
        * If ignore_errors is set true the function won't stop on the first
        * symbol that can't be loaded but instead tries to load them all.
        * If one or more symbols weren't loaded the function returns false. */
        bool load_symbols(bool ignore_errors=false);

        /* load a specific symbol */
        bool load_symbol(const char *symbol);

        /* check if symbols were successfully loaded */
        bool symbols_loaded() const;

        /* free library */
        bool free();

        /* whether to free the library in the class destructor */
        void free_lib_in_dtor(bool b);

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

    }; /* class dl */
} /* namespace gdo */

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

GDO_DEFAULT_FLAGS
    Override the default flags to use when loading a library.

GDO_DEFAULT_LIB
    Set a default library name through this macro (including double quote
    marks). This macro must be defined if you want to set GDO_ENABLE_AUTOLOAD.

GDO_ENABLE_AUTOLOAD
    Define this macro if you want to use auto-loading wrapper functions.
    This means you don't need to explicitly call library load functions.
    It requires GDO_DEFAULT_LIB to be defined.
    If an error occures during loading these functions throw an error message
    and call `std::exit(1)'!

GDO_WRAP_FUNCTIONS
    Use actual wrapped functions instead of a name alias. This is useful if you
    want to create a library to later link an application against.

GDO_VISIBILITY
    You can set the symbol visibility of wrapped functions (enabled with
    GDO_WRAP_FUNCTIONS) using this macro.

***/

#include <iostream>
#include <string>
#include <stdlib.h>
#include <string.h>


namespace gdo
{
    /* function pointer typedefs */
    namespace type
    {
        using GDO_SYMBOL = GDO_TYPE (*)(GDO_ARGS);
    }

    /* symbol pointers */
    namespace ptr
    {
        type::GDO_SYMBOL GDO_SYMBOL = nullptr;
        GDO_OBJ_TYPE *GDO_OBJ_SYMBOL = nullptr;
    }

    /* whether or not a symbol was loaded */
    namespace loaded
    {
        bool GDO_SYMBOL = false;
        bool GDO_OBJ_SYMBOL = false;
    }

    /* function pointer to error message callback */
    void (*message_callback)(const char *) = nullptr;

    /* default flags */
    const int default_flags = GDO_DEFAULT_FLAGS;

    /* Shared library file extension without dot ("dll", "dylib" or "so").
     * Useful i.e. on plugins. */
    const char * const libext = LIBEXTA;


    /* Create versioned library names for DLLs, dylibs and DSOs.
     * libname("z",1) for example will return "libz-1.dll", "libz.1.dylib" or "libz.so.1" */
    const char *libname(const char *name, unsigned int api)
    {
        static std::string s = "lib";
        s += name;

#if defined(_WIN32)
        s += '-';
        s += std::to_string(api);
        s += ".dll";
#elif defined(__APPLE__)
        s += '.';
        s += std::to_string(api);
        s += ".dylib";
#elif defined(_AIX)
        (void)api;
        s += ".a";
#else /* ELF */
        s += ".so.";
        s += std::to_string(api);
#endif

        return s.c_str();
    }


/* library loader class */
class dl
{
private:

#ifdef GDO_WINAPI
    HMODULE m_handle = NULL;
    DWORD m_last_error = 0;
    const char *m_errmsg = NULL;
    const wchar_t *m_werrmsg = NULL;
#else
    void *m_handle = NULL;
    std::string m_last_error;
#endif

    const char *m_filename = NULL;
    int m_flags = default_flags;
    bool m_new_namespace = false;
    bool m_free_lib_in_dtor = true;


#ifdef GDO_WINAPI
    std::string wchar_to_string(const wchar_t *lpWstr)
    {
        size_t mbslen, n;
        char *buf;
        std::string str;

        if (!lpWstr || ::wcstombs_s(&mbslen, NULL, 0, lpWstr, 0) != 0 || mbslen == 0) {
            return {};
        }

        buf = new char[mbslen + 1];
        if (!buf) return {};

        if (::wcstombs_s(&n, buf, mbslen+1, lpWstr, mbslen) != 0 || n == 0) {
            delete buf;
            return {};
        }

        buf[mbslen] = '\0';
        str = buf;
        delete buf;

        return buf;
    }


    std::wstring char_to_wstring(const char *str)
    {
        size_t len, n;
        wchar_t *buf;
        std::wstring wstr;

        if (!str || ::mbstowcs_s(&len, NULL, 0, str, 0) != 0 || len == 0) {
            return {};
        }

        buf = new wchar_t[(len + 1) * sizeof(wchar_t)];
        if (!buf) return {};

        if (::mbstowcs_s(&n, buf, len+1, str, len) != 0 || n == 0) {
            delete buf;
            return {};
        }

        buf[len] = L'\0';
        wstr = buf;
        delete buf;

        return wstr;
    }
#endif // GDO_WINAPI


    /* clear error */
    void clear_error()
    {
#ifdef GDO_WINAPI
        m_last_error = 0;
        m_errmsg = NULL;
        m_werrmsg = NULL;
#else
        m_last_error = "";
        ::dlerror();
#endif
    }


    /* save last error */
    void save_error(const char *msg=NULL)
    {
#ifdef GDO_WINAPI
        m_last_error = ::GetLastError();
        m_errmsg = msg;
        m_werrmsg = NULL;
#else
        (void)msg;
        const char *ptr = ::dlerror();
        m_last_error = ptr ? ptr : "";
#endif
    }


    /* save last error (wide characters) */
#ifdef GDO_WINAPI
    void save_error(const wchar_t *msg)
    {
        m_last_error = ::GetLastError();
        m_werrmsg = msg;
        m_errmsg = NULL;
    }
#endif


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

        if (!ptr) {
            save_error(symbol);
            return false;
        }
#else
        ptr = reinterpret_cast<T>(::dlsym(m_handle, symbol));

        /* NULL can be a valid value (unusual but possible),
         * so call dlerror() to check for errors */
        const char *errptr = ::dlerror();

        if (errptr) {
            /* save error message */
            m_last_error = errptr;

            /* clear error */
            ::dlerror();

            return false;
        }
#endif

        return true;
    }


#ifdef GDO_WINAPI
    inline DWORD get_module_filename(HMODULE handle, wchar_t *buf, DWORD len) {
        return ::GetModuleFileNameW(handle, buf, len);
    }

    inline DWORD get_module_filename(HMODULE handle, char *buf, DWORD len) {
        return ::GetModuleFileNameA(handle, buf, len);
    }


    /* get the module's full path using GetModuleFileName() */
    template<typename T>
    std::basic_string<T> get_origin_from_module_handle()
    {
        DWORD len = 260; /* MAX_PATH */
        std::basic_string<T> str;
        T *buf = reinterpret_cast<T*>(::malloc(len * sizeof(T)));

        if (!buf) {
            save_error();
            return {};
        }

        if (get_module_filename(m_handle, buf, len-1) == 0) {
            save_error();
            ::free(buf);
            return {};
        }

        /* https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
         * technically the path could exceed 260 characters, but in reality
         * it's practically still stuck at the old MAX_PATH value */
        if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            len = 32*1024;
            buf = reinterpret_cast<T*>(realloc(buf, len * sizeof(T)));

            if (get_module_filename(m_handle, buf, len-1) == 0) {
                save_error();
                ::free(buf);
                return {};
            }
        }

        str = buf;
        ::free(buf);

        return str;
    }


    inline DWORD format_message(DWORD flags, LPWSTR buf) {
        return ::FormatMessageW(flags, NULL, m_last_error, 0, buf, 0, NULL);
    }

    inline DWORD format_message(DWORD flags, LPSTR buf) {
        return ::FormatMessageA(flags, NULL, m_last_error, 0, buf, 0, NULL);
    }


    /* return a formatted error message */
    template<typename T>
    std::basic_string<T> format_last_error_message()
    {
        std::basic_string<T> str;
        T *buf = NULL;

        format_message(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK,
                    reinterpret_cast<T*>(&buf));

        if (buf) {
            str = buf;
            ::LocalFree(buf);
        }

        return str;
    }
#endif //GDO_WINAPI


public:

    /* c'tor (empty) */
    dl()
    {}


    /* c'tor (set filename) */
    dl(const char *filename, int flags=default_flags, bool new_namespace=false)
      : m_filename(filename),
        m_flags(flags),
        m_new_namespace(new_namespace)
    {}


    /* d'tor */
    ~dl()
    {
        if (m_free_lib_in_dtor && lib_loaded()) {
#ifdef GDO_WINAPI
            ::FreeLibrary(m_handle);
#else
            ::dlclose(m_handle);
#endif
        }
    }


    /* load library */
    bool load(const char *filename, int flags=default_flags, bool new_namespace=false)
    {
        clear_error();

        if (lib_loaded()) {
            return true;
        }

        load_lib(filename, flags, new_namespace);
        save_error(filename);

        return lib_loaded();
    }


#ifdef GDO_WINAPI
    /* load library (wide characters version) */
    bool load(const wchar_t *filename, int flags=default_flags)
    {
        clear_error();

        if (lib_loaded()) {
            return true;
        }

        m_flags = flags;
        m_handle = ::LoadLibraryExW(filename, NULL, m_flags);
        save_error(filename);

        return lib_loaded();
    }
#endif


    /* load library */
    bool load()
    {
        return load(m_filename, m_flags, m_new_namespace);
    }


    /* load library and symbols */
    bool load_lib_and_symbols()
    {
        return (load() && load_symbols());
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
    bool load_symbols(bool ignore_errors=false)
    {
        clear_error();

        if (symbols_loaded()) {
            return true;
        } else if (!lib_loaded()) {
            set_error_invalid_handle();
            return false;
        }

        /* get symbol addresses */
@
        loaded::GDO_SYMBOL = sym<type::GDO_SYMBOL>(@
            ptr::GDO_SYMBOL, "GDO_SYMBOL");@
        if (!loaded::GDO_SYMBOL && !ignore_errors){@
            return false;@
        }
@
        loaded::GDO_OBJ_SYMBOL = sym<GDO_OBJ_TYPE *>(@
            ptr::GDO_OBJ_SYMBOL, "GDO_OBJ_SYMBOL");@
        if (!loaded::GDO_OBJ_SYMBOL && !ignore_errors) {@
            return false;@
        }

        clear_error();

        return symbols_loaded();
    }


    /* load a specific symbol */
    bool load_symbol(const char *symbol)
    {
        clear_error();

        if (!lib_loaded()) {
            set_error_invalid_handle();
            return false;
        } else if (!symbol || !*symbol) {
            return false;
        }

        /* get symbol address */
@
        if (strcmp("GDO_SYMBOL", symbol) == 0) {@
            return sym<type::GDO_SYMBOL>(@
                ptr::GDO_SYMBOL, "GDO_SYMBOL");@
        }
@
        if (strcmp("GDO_OBJ_SYMBOL", symbol) == 0) {@
            return sym<GDO_OBJ_TYPE *>(@
                ptr::GDO_OBJ_SYMBOL, "GDO_OBJ_SYMBOL");@
        }

        return false;
    }


    /* check if all symbols are loaded */
    bool symbols_loaded() const
    {
        if (true
            && loaded::GDO_SYMBOL
            && loaded::GDO_OBJ_SYMBOL
        ) {
            return true;
        }

        return false;
    }


    /* free library */
    bool free()
    {
        if (!lib_loaded()) {
            clear_error();
            return true;
        }

#ifdef GDO_WINAPI
        bool ret = (::FreeLibrary(m_handle) == TRUE);
#else
        bool ret = (::dlclose(m_handle) == 0);
#endif

        save_error();

        if (!ret) {
            return false;
        }

        m_handle = NULL;

        ptr::GDO_SYMBOL = nullptr;
        ptr::GDO_OBJ_SYMBOL = nullptr;

        loaded::GDO_SYMBOL = false;
        loaded::GDO_OBJ_SYMBOL = false;

        return true;
    }


    /* whether to free the library in the class destructor */
    void free_lib_in_dtor(bool b)
    {
        m_free_lib_in_dtor = b;
    }


    /* get path of loaded library */
    std::string origin()
    {
        if (!lib_loaded()) {
            set_error_invalid_handle();
            return {};
        }

#ifdef GDO_WINAPI
        return get_origin_from_module_handle<char>();
#else
        struct link_map *lm = NULL;

        int ret = ::dlinfo(m_handle, RTLD_DI_LINKMAP, reinterpret_cast<void *>(&lm));
        save_error();

        if (ret != -1 && lm->l_name) {
            return lm->l_name;
        }

        return {};
#endif //GDO_WINAPI
    }


#ifdef GDO_WINAPI
    /* get path of loaded library (wide characters version) */
    std::wstring origin_w()
    {
        if (!lib_loaded()) {
            set_error_invalid_handle();
            return {};
        }

        return get_origin_from_module_handle<wchar_t>();
    }
#endif //GDO_WINAPI


    /* retrieve the last error */
    std::string error()
    {
#ifdef GDO_WINAPI
        auto buf = format_last_error_message<char>();

        if (buf.empty()) {
            buf = "Last saved error code: ";
            buf += std::to_string(m_last_error);
        }

        if (m_errmsg && m_errmsg[0] != 0) {
            buf.insert(0, ": ");
            buf.insert(0, m_errmsg);
        } else if (m_werrmsg && m_werrmsg[0] != 0) {
            buf.insert(0, ": ");
            buf.insert(0, wchar_to_string(m_werrmsg));
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
        auto buf = format_last_error_message<wchar_t>();

        if (buf.empty()) {
            buf = L"Last saved error code: ";
            buf += std::to_wstring(m_last_error);
        }

        if (m_werrmsg && m_werrmsg[0] != 0) {
            buf.insert(0, L": ");
            buf.insert(0, m_werrmsg);
        } else if (m_errmsg && m_errmsg[0] != 0) {
            buf.insert(0, L": ");
            buf.insert(0, char_to_wstring(m_errmsg));
        }

        return buf;
    }
#endif //GDO_WINAPI

}; /* end class dl */

} /* namespace gdo */


#if !defined(_GDO_WRAP_CODE_WAS_GENERATED)

/* aliases to raw function pointers */
#define GDO_SYMBOL gdo::ptr::GDO_SYMBOL

/* aliases to raw object pointers */
#define GDO_OBJ_SYMBOL *gdo::ptr::GDO_OBJ_SYMBOL

#endif //!_GDO_WRAP_CODE_WAS_GENERATED
