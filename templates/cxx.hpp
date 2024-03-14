
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
#ifdef GDO_WINAPI
    const wchar_t *libname(const wchar_t *name, unsigned int api);
#endif


    class dl
    {
    /*** constructors ***/

        /* empty c'tor */
        dl();

        /* set filename, flags and whether to use a new namespace */
        dl(const std::string &filename, int flags=default_flags, bool new_namespace=false);


    /*** member functions ***/

        /* load the library that was set by the c'tor */
        bool load();

        /* load filename; set flags and whether to use a new namespace */
        bool load(const std::string &filename, int flags=default_flags, bool new_namespace=false);

        /* wide character variants for win32 */
#ifdef GDO_WINAPI
        bool load(const std::wstring &filename, int flags=default_flags);
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
        bool load_symbol(const std::string &symbol);

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

***/

#include <iostream>
#include <string>
#include <stdlib.h>
#include <string.h>


#if defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_HAS_WRAP_CODE)
#error "GDO_WRAP_FUNCTIONS" defined but wrapped functions were disabled with "--skip-parameter-names"
#endif

#if defined(GDO_ENABLE_AUTOLOAD) && !defined(GDO_HAS_WRAP_CODE)
#error "GDO_ENABLE_AUTOLOAD" defined but wrapped functions were disabled with "--skip-parameter-names"
#endif

#if defined(GDO_ENABLE_AUTOLOAD) && !defined(GDO_DEFAULT_LIB)
#error You need to define GDO_DEFAULT_LIB if you want to make use of GDO_ENABLE_AUTOLOAD
#endif

#if defined(GDO_DELAYLOAD) && !defined(GDO_ENABLE_AUTOLOAD)
#error You need to define GDO_ENABLE_AUTOLOAD if you want to make use of GDO_DELAYLOAD
#endif


/*****************************************************************************/
/*                          begin of namespace                               */
/*****************************************************************************/
namespace gdo
{
    using UNUSED_REF = void;
    using UNUSED_RESULT = void;

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
    std::string libname(const std::string &name, unsigned int api)
    {
#if defined(_WIN32)  // not GDO_WINAPI!
        std::string s = "lib" + (name + ('-' + (std::to_string(api) + ".dll")));
#elif defined(__APPLE__)
        std::string s = "lib" + (name + ('.' + (std::to_string(api) + ".dylib")));
#elif defined(_AIX)
        (UNUSED_REF) api;
        std::string s = "lib" + (name + ".a");
#else
        std::string s = "lib" + (name + (".so." + std::to_string(api)));
#endif
        return s;
    }

#ifdef GDO_WINAPI
    const wchar_t *libname(const std::wstring &name, unsigned int api)
    {
        static std::wstring s = L"lib" + (name + (L'-' + (std::to_wstring(api) + L".dll")));
        return s.c_str();
    }
#endif //GDO_WINAPI


/*****************************************************************************/
/*                          library loader class                             */
/*****************************************************************************/
class dl
{
private:

    std::string m_filename;
    int m_flags = default_flags;
    bool m_new_namespace = false;
    bool m_free_lib_in_dtor = true;

#ifdef GDO_WINAPI

    /* library handle */
    HMODULE m_handle = nullptr;

    /* error message */
    DWORD m_last_error = 0;
    std::string m_errmsg;
    std::wstring m_werrmsg;


    /* wstring to string */
    std::string wstr_to_str(const std::wstring &wstr)
    {
        size_t len, n;
        std::string buf;

        if (wstr.empty() || ::wcstombs_s(&len, nullptr, 0, wstr.c_str(), 0) != 0 ||
            len == 0)
        {
            return {};
        }

        buf.reserve(len+1);

        if (::wcstombs_s(&n, &buf[0], len+1, wstr.c_str(), len) != 0 || n == 0) {
            return {};
        }

        return buf;
    }


    /* string to wstring */
    std::wstring str_to_wstr(const std::string &str)
    {
        size_t len, n;
        std::wstring buf;

        if (str.empty() || ::mbstowcs_s(&len, nullptr, 0, str.c_str(), 0) != 0
            || len == 0)
        {
            return {};
        }

        buf.reserve(len+1);

        if (::mbstowcs_s(&n, &buf[0], len+1, str.c_str(), len) != 0 || n == 0) {
            return {};
        }

        return buf;
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
    void load_lib(const char *filename, int flags, bool /*unused*/)
    {
        m_flags = flags;
        m_handle = ::LoadLibraryExA(filename, nullptr, m_flags);
    }


    /* free library handle */
    bool free_lib()
    {
        return (::FreeLibrary(m_handle) == TRUE);
    }


    /* load symbol address */
    void *sym(const char *symbol, bool &rv)
    {
        clear_error();

        void *ptr = reinterpret_cast<void *>(::GetProcAddress(m_handle, symbol));

        if (!ptr) {
            save_error(symbol);
            rv = false;
            return nullptr;
        }

        rv = true;
        return ptr;
    }


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
        if (!lib_loaded()) {
            set_error_invalid_handle();
            return {};
        }

        DWORD len = 260; /* MAX_PATH */
        std::basic_string<T> buf(len, 0);

        if (get_module_filename(m_handle, &buf[0], len-1) == 0) {
            save_error();
            return {};
        }

        /* https://learn.microsoft.com/en-us/windows/win32/fileio/maximum-file-path-limitation
         * technically the path could exceed 260 characters, but in reality
         * it's practically still stuck at the old MAX_PATH value */
        if (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            len = 32*1024;
            buf.clear();
            buf.resize(len);

            if (get_module_filename(m_handle, &buf[0], len-1) == 0) {
                save_error();
                return {};
            }

            buf.shrink_to_fit();
        }

        return buf;
    }


    inline DWORD format_message(DWORD flags, DWORD msgId, LPWSTR buf) {
        return ::FormatMessageW(flags, NULL, msgId, 0, buf, 0, NULL);
    }

    inline DWORD format_message(DWORD flags, DWORD msgId, LPSTR buf) {
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
    void *m_handle = nullptr;

    /* error message */
    std::string m_errmsg;


    /* clear error */
    void clear_error()
    {
        m_errmsg.clear();
        (UNUSED_RESULT) ::dlerror();
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
    void load_lib(const char *filename, int flags, bool new_namespace)
    {
        m_flags = flags;

#ifdef GDO_NO_DLMOPEN
        /* dlmopen() disabled */
        (UNUSED_REF) new_namespace;
        m_handle = ::dlopen(filename, m_flags);
#else
        /* dlmopen() for new namespace or dlopen() */
        if (new_namespace) {
            m_handle = ::dlmopen(LM_ID_NEWLM, filename, m_flags);
        } else {
            m_handle = ::dlopen(filename, m_flags);
        }
#endif // GDO_NO_DLMOPEN
    }


    /* free library handle */
    bool free_lib()
    {
        return (::dlclose(m_handle) == 0);
    }


    /* load symbol address */
    void *sym(const char *symbol, bool &rv)
    {
        clear_error();

        void *ptr = ::dlsym(m_handle, symbol);

        /* NULL can be a valid value (unusual but possible),
         * so call dlerror() to check for errors */
        if (!ptr) {
            auto err = ::dlerror();

            if (err) {
                /* must save our error message manually instead of
                 * invoking save_error() */
                m_errmsg = err;

                /* clear error */
                (UNUSED_RESULT) ::dlerror();

                rv = false;
                return nullptr;
            }
        }

        rv = true;
        return ptr;
    }

#endif // !GDO_WINAPI


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
        /* release old libhandle */
        if (lib_loaded() && !free()) {
            return false;
        }

        if (filename.empty()) {
            set_error_empty_filename();
            return false;
        }

        clear_error();
        load_lib(filename.c_str(), flags, new_namespace);
        save_error(filename);

        return lib_loaded();
    }


#ifdef GDO_WINAPI
    /* load library (wide characters version) */
    bool load(const std::wstring &filename, int flags=default_flags)
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

        m_flags = flags;
        m_handle = ::LoadLibraryExW(filename.c_str(), NULL, m_flags);
        save_error(filename);

        return lib_loaded();
    }
#endif //GDO_WINAPI


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
        return (m_handle != nullptr);
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
        ptr::GDO_SYMBOL = reinterpret_cast<type::GDO_SYMBOL>(@
            sym("GDO_SYMBOL", loaded::GDO_SYMBOL));@
        if (!loaded::GDO_SYMBOL && !ignore_errors) {@
            return false;@
        }
@
        ptr::GDO_OBJ_SYMBOL = reinterpret_cast<GDO_OBJ_TYPE *>(@
            sym("GDO_OBJ_SYMBOL", loaded::GDO_OBJ_SYMBOL));@
        if (!loaded::GDO_OBJ_SYMBOL && !ignore_errors) {@
            return false;@
        }

        clear_error();

        return symbols_loaded();
    }


    /* load a specific symbol */
    bool load_symbol(const std::string &symbol)
    {
        clear_error();

        if (!lib_loaded()) {
            set_error_invalid_handle();
            return false;
        }

        if (symbol.empty()) {
#ifdef GDO_WINAPI
            m_last_error = ERROR_INVALID_PARAMETER;
#endif
            m_errmsg = "empty symbol name";
            return false;
        }

        /* get symbol address */
@
        if (symbol == "GDO_SYMBOL") {@
            ptr::GDO_SYMBOL = reinterpret_cast<type::GDO_SYMBOL>(@
                sym("GDO_SYMBOL", loaded::GDO_SYMBOL));@
            return loaded::GDO_SYMBOL;@
        }
@
        if (symbol == "GDO_OBJ_SYMBOL") {@
            ptr::GDO_OBJ_SYMBOL = reinterpret_cast<GDO_OBJ_TYPE *>(@
                sym("GDO_OBJ_SYMBOL", loaded::GDO_OBJ_SYMBOL));@
            return loaded::GDO_OBJ_SYMBOL;@
        }

        clear_error();

#ifdef GDO_WINAPI
        m_last_error = ERROR_NOT_FOUND;
#endif
        m_errmsg = "symbol not among lookup list: ";
        m_errmsg += symbol;

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


#else
/*********************************** dlfcn ***********************************/


    /* get path of loaded library */
    std::string origin()
    {
        if (!lib_loaded()) {
            set_error_invalid_handle();
            return {};
        }

        struct link_map *lm = nullptr;

        int ret = ::dlinfo(m_handle, RTLD_DI_LINKMAP, reinterpret_cast<void *>(&lm));
        save_error();

        if (ret != -1 && lm->l_name) {
            return lm->l_name;
        }

        return {};
    }


    /* retrieve the last error */
    std::string error()
    {
        return m_errmsg;
    }

#endif // !GDO_WINAPI

};
/******************************* end of class ********************************/


/*****************************************************************************/
/*                                wrap code                                  */
/*****************************************************************************/
#ifdef GDO_HAS_WRAP_CODE

    namespace /* anonymous */
    {
        void error_exit(const char *s1, const char *s2, const char *s3, const std::string &s4)
        {
            if (message_callback) {
                std::string msg = s1 + (s2 + (s3 + s4));
                message_callback(msg.c_str());
            } else {
                std::cerr << s1 << s2 << s3 << s4 << std::endl;
            }

            std::exit(1);
        }
    } /* anonymous namespace */


    /* wrapped functions
     * (creating wrapped symbols doesn't work well with pointers to objects) */
    namespace wrapped
    {
        GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
            if (!loaded::GDO_SYMBOL) {@
                error_exit("error: symbol `GDO_SYMBOL' was not loaded", "", "", "");@
            }@
            GDO_RET ptr::GDO_SYMBOL(GDO_NOTYPE_ARGS);@
        }@

    } /* namespace wrapped */


#ifdef GDO_ENABLE_AUTOLOAD

    namespace autoload
    {
        namespace /* anonymous */
        {
            auto al = dl(GDO_DEFAULT_LIB);

            /* used internally by wrapper functions, symbol is never NULL */
            void quick_load(const char *symbol)
            {
                if (!al.load()) {
                    error_exit("error loading library `", GDO_DEFAULT_LIB,
                        "':\n", al.error());
                }

            #ifdef GDO_DELAYLOAD
                if (!al.load_symbol(symbol))
            #else
                if (!al.load_symbols())
            #endif
                {
                    error_exit("error in auto-loading wrapper function "
                        "`gdo::autoload::", symbol, "': ", al.error());
                }
            }
        } /* anonymous namespace */


        GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
            quick_load("GDO_SYMBOL");@
            GDO_RET wrapped::GDO_SYMBOL(GDO_NOTYPE_ARGS);@
        }@

    } /* namespace autoload */

#endif //GDO_ENABLE_AUTOLOAD

#endif //GDO_HAS_WRAP_CODE
/***************************** end of wrap code ******************************/

} /* namespace gdo */
/***************************** end of namespace ******************************/



#if defined(GDO_HAS_WRAP_CODE) && defined(GDO_WRAP_FUNCTIONS)

/* function wrappers */
@
GDO_VISIBILITY GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
    GDO_RET gdo::wrapped::GDO_SYMBOL(GDO_NOTYPE_ARGS);@
}

#elif defined(GDO_HAS_WRAP_CODE) && defined(GDO_ENABLE_AUTOLOAD)

/* autoload function wrappers */
@
GDO_VISIBILITY GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
    GDO_RET gdo::autoload::GDO_SYMBOL(GDO_NOTYPE_ARGS);@
}

#else

/* aliases to raw function pointers */
#define GDO_SYMBOL gdo::ptr::GDO_SYMBOL

#endif

/* aliases to raw object pointers */
#define GDO_OBJ_SYMBOL *gdo::ptr::GDO_OBJ_SYMBOL
