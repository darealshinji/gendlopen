#if 0

/**************************************/
/*           quick overview           */
/**************************************/

namespace gdo {

/*** constants ***/

    /* API-agnostic default flags */
    const int default_flags;

    /* Shared library file extension without dot ("dll", "dylib" or "so").
     * Useful i.e. on plugins. */
    const char * const libext;

    /* function pointer to error message callback */
    void (*message_callback)(const char *) = nullptr;


    class dl {

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
#ifdef _$WINAPI
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
#ifdef _$WINAPI
        std::wstring origin_w();
#endif

        /* retrieve the last error message */
        std::string error();
#ifdef _$WINAPI
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

_$USE_DLOPEN
    If defined `dlopen()' API is used on win32 targets.
    On other targets `dlopen()' is always used.

_$NO_DLMOPEN
    If defined `dlmopen()` will never be used.

_$DEFAULT_LIB
    Set a default library name through this macro (including double quote
    marks). This macro must be defined if you want to set _$ENABLE_AUTOLOAD.

_$ENABLE_AUTOLOAD
    Define this macro if you want to use auto-loading wrapper functions.
    This means you don't need to explicitly call library load functions.
    It requires _$DEFAULT_LIB to be defined.
    If an error occures during loading these functions throw an error message
    and call `std::abort()'!



*****************
* Helper macros *
*****************

_$LIB(NAME, API)
    Convenience macro to create versioned library names for DLLs, dylibs and DSOs,
    including double quote marks.
    _$LIB(z,1) for example will become "libz-1.dll", "libz.1.dylib" or "libz.so.1".

***/


GDO_COMMON

#include <iostream>
#include <string>
#ifdef _$WINAPI
#include <locale>
#endif
#include <cstring>


/* begin namespace */
namespace gdo
{

/* anonymous namespace */
namespace {
    /* function pointers */
    using GDO_SYMBOL_t = GDO_TYPE (*)(GDO_ARGS);@
    GDO_SYMBOL_t GDO_SYMBOL_ptr_;

    /* object pointers */
    GDO_OBJ_TYPE *GDO_OBJ_SYMBOL_ptr_;
}

/* default flags */
const int default_flags = _$DEFAULT_FLAGS;

/* Shared library file extension without dot ("dll", "dylib" or "so").
 * Useful i.e. on plugins. */
const char * const libext = _$LIBEXTA;

/* function pointer to error message callback */
void (*message_callback)(const char *) = nullptr;


/* library loader class */
class dl
{
private:

#ifdef _$WINAPI
    /* utility wrapper to adapt locale-bound facets for wstring convert */
    template<class Facet>
    struct deletable_facet : Facet
    {
        template<class ...Args>
        deletable_facet(Args&& ...args) : Facet(std::forward<Args>(args)...) {}
        ~deletable_facet() {}
    };

    using WCharFacet = deletable_facet<std::codecvt<wchar_t, char, std::mbstate_t>>;

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
    bool m_symbols_loaded = false;
    bool m_free_lib_in_dtor = true;


    /* clear error */
    void clear_error()
    {
#ifdef _$WINAPI
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
#ifdef _$WINAPI
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
#ifdef _$WINAPI
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

#ifdef _$WINAPI
        m_last_error = ERROR_INVALID_HANDLE;
#else
        m_last_error = "no library was loaded";
#endif
    }


    /* load library */
    void load_lib(const char *filename, int flags, bool new_namespace)
    {
        m_flags = flags;

#if defined(_$WINAPI)
        /* win32 */
        (void)new_namespace;
        m_handle = ::LoadLibraryExA(filename, NULL, m_flags);
#elif defined(_$NO_DLMOPEN)
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
    bool sym(T &ptr, const char *symbol, bool rv=true)
    {
#ifdef _$WINAPI
        ptr = reinterpret_cast<T>(::GetProcAddress(m_handle, symbol));

        if (!rv) {
            return false;
        } else if (!ptr) {
            save_error(symbol);
            return false;
        }
#else
        ptr = reinterpret_cast<T>(::dlsym(m_handle, symbol));

        if (!rv) return false;

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


#ifdef _$WINAPI
    inline DWORD get_module_filename(HMODULE handle, wchar_t *buf, size_t len) {
        return ::GetModuleFileNameW(handle, buf, len);
    }

    inline DWORD get_module_filename(HMODULE handle, char *buf, size_t len) {
        return ::GetModuleFileNameA(handle, buf, len);
    }


    /* get the module's full path using GetModuleFileName() */
    template<typename T1, typename T2>
    T1 get_origin_from_module_handle()
    {
        size_t len = 260; /* MAX_PATH */
        T1 str;
        T2 *buf = reinterpret_cast<T2*>(malloc(len * sizeof(T2)));

        if (!buf) {
            save_error();
            return {};
        }

        if (get_module_filename(m_handle, buf, len-1) == 0) {
            save_error();
            ::free(buf);
            return {};
        }

        /* technically the path could exceed 260 characters, but in reality
         * it's practically still stuck at the old MAX_PATH value */
        while (::GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            len += 1024;
            buf = reinterpret_cast<T2*>(realloc(buf, len * sizeof(T2)));

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
    template<typename T1, typename T2>
    T1 format_last_error_message()
    {
        T1 str;
        T2 *buf = NULL;

        format_message(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                        FORMAT_MESSAGE_FROM_SYSTEM |
                        FORMAT_MESSAGE_MAX_WIDTH_MASK,
                    reinterpret_cast<T2 *>(&buf));

        if (buf) {
            str = buf;
            ::LocalFree(buf);
        }

        return str;
    }
#endif //_$WINAPI


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
        if (m_free_lib_in_dtor && lib_loaded()) {
#ifdef _$WINAPI
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


#ifdef _$WINAPI
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
        bool rv = true;

        clear_error();

        if (m_symbols_loaded) {
            return true;
        } else if (!lib_loaded()) {
            set_error_invalid_handle();
            return false;
        }

        if (ignore_errors) {
            /* load function pointer addresses */
            rv = sym<GDO_SYMBOL_t>(GDO_SYMBOL_ptr_, "GDO_SYMBOL", rv);

            /* load object addresses */
            rv = sym<GDO_OBJ_TYPE *>(GDO_OBJ_SYMBOL_ptr_, "GDO_OBJ_SYMBOL", rv);
        } else {
            /* load function pointer addresses */
            if (!sym<GDO_SYMBOL_t>(GDO_SYMBOL_ptr_, "GDO_SYMBOL")) {@
                return false;@
            }

            /* load object addresses */
            if (!sym<GDO_OBJ_TYPE *>(GDO_OBJ_SYMBOL_ptr_, "GDO_OBJ_SYMBOL")) {@
                return false;@
            }
        }

        if (rv) m_symbols_loaded = true;

        clear_error();

        return rv;
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

        /* function pointer addresses */
        if (strcmp("GDO_SYMBOL", symbol) == 0) {@
            return sym<GDO_SYMBOL_t>(GDO_SYMBOL_ptr_, "GDO_SYMBOL");@
        }

        /* load object addresses */
        if (strcmp("GDO_OBJ_SYMBOL", symbol) == 0) {@
            return sym<GDO_OBJ_TYPE *>(GDO_OBJ_SYMBOL_ptr_, "GDO_OBJ_SYMBOL");@
        }

        return false;
    }


    /* check if all symbols are loaded */
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

#ifdef _$WINAPI
        bool ret = (::FreeLibrary(m_handle) == TRUE);
#else
        bool ret = (::dlclose(m_handle) == 0);
#endif

        save_error();

        if (!ret) {
            return false;
        }

        m_handle = NULL;
        GDO_SYMBOL_ptr_ = nullptr;
        GDO_OBJ_SYMBOL_ptr_ = nullptr;

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

#ifdef _$WINAPI
        return get_origin_from_module_handle<std::string, char>();
#else
        struct link_map *lm = NULL;

        int ret = ::dlinfo(m_handle, RTLD_DI_LINKMAP, reinterpret_cast<void *>(&lm));
        save_error();

        if (ret != -1 && lm->l_name) {
            return lm->l_name;
        }

        return {};
#endif //_$WINAPI
    }


#ifdef _$WINAPI
    /* get path of loaded library (wide characters version) */
    std::wstring origin_w()
    {
        if (!lib_loaded()) {
            set_error_invalid_handle();
            return {};
        }

        return get_origin_from_module_handle<std::wstring, wchar_t>();
    }
#endif //_$WINAPI


    /* retrieve the last error */
    std::string error()
    {
#ifdef _$WINAPI
        std::string buf = format_last_error_message<std::string, char>();

        if (buf.empty()) {
            buf = "Last saved error code: ";
            buf += std::to_string(m_last_error);
        }

        if (m_errmsg && m_errmsg[0] != 0) {
            buf.insert(0, ": ");
            buf.insert(0, m_errmsg);
        } else if (m_werrmsg && m_werrmsg[0] != 0) {
            /* convert wchar_t to char */
            std::wstring_convert<WCharFacet> conv;
            buf.insert(0, ": ");
            buf.insert(0, conv.to_bytes(m_werrmsg));
        }

        return buf;
#else
        return m_last_error;
#endif //_$WINAPI
    }


#ifdef _$WINAPI
    /* retrieve the last error (wide characters version) */
    std::wstring error_w()
    {
        std::wstring buf = format_last_error_message<std::wstring, wchar_t>();

        if (buf.empty()) {
            buf = L"Last saved error code: ";
            buf += std::to_wstring(m_last_error);
        }

        if (m_werrmsg && m_werrmsg[0] != 0) {
            buf.insert(0, L": ");
            buf.insert(0, m_werrmsg);
        } else if (m_errmsg && m_errmsg[0] != 0) {
            /* convert char to wchar_t */
            std::wstring_convert<WCharFacet> conv;
            buf.insert(0, L": ");
            buf.insert(0, conv.from_bytes(m_errmsg));
        }

        return buf;
    }
#endif //_$WINAPI

}; /* end class dl */


    /* anonymous */
    namespace
    {
        void print_error(const std::string &msg)
        {
            if (message_callback) {
                message_callback(msg.c_str());
            } else {
                std::cerr << msg << std::endl;
            }
        }

#ifdef _$ENABLE_AUTOLOAD

#if !defined(_$DEFAULT_LIB)
    #error  _$DEFAULT_LIB was not defined!
#endif
        auto al = dl();

        void autoload(const char *symbol)
        {
            if (!al.load(_$DEFAULT_LIB)) {
                std::string msg = "error loading library `" _$DEFAULT_LIB "':\n";
                msg += al.error();
                print_error(msg);
                std::exit(1);
            }

            if (!al.load_symbols()) {
                std::string msg = "error in auto-loading wrapper function `";
                msg += symbol ? symbol : "(NULL)";
                msg += "': ";
                msg += al.error();
                print_error(msg);
                std::exit(1);
            }
        }

#else // !_$ENABLE_AUTOLOAD

        void autoload(const char *) {}

#endif // !_$ENABLE_AUTOLOAD

        void ptr_is_null(const char *symbol)
        {
            std::string msg = "error: pointer to symbol `";
            msg += symbol ? symbol : "(NULL)";
            msg += "' is NULL";
            print_error(msg);
            std::abort();
        }

    } /* anonymous namespace end */


    /* wrapped functions */

    GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
        autoload("GDO_SYMBOL");@
        if (!GDO_SYMBOL_ptr_) ptr_is_null("GDO_SYMBOL");@
        GDO_RET GDO_SYMBOL_ptr_(GDO_NOTYPE_ARGS);@
    }@

} /* namespace gdo */


/* aliases */
#define GDO_SYMBOL gdo::GDO_SYMBOL
#define GDO_OBJ_SYMBOL *gdo::GDO_OBJ_SYMBOL_ptr_

