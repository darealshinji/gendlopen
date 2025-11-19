#if defined(_MSC_VER) && defined(GDO_USE_MESSAGE_BOX)
# pragma comment(lib, "user32.lib")
#endif

#ifdef GDO_WINAPI
# include <cstdlib>
# include <cstring>
#endif
#include <algorithm>
#include <iostream>
#include <cstdlib>


#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

/* the default message callback function to use */
void gdo::dl::default_message_callback(const char *msg)
{
#if defined(_WIN32) && defined(GDO_USE_MESSAGE_BOX)
    MessageBoxA(NULL, msg, "Error", MB_OK | MB_ICONERROR);
#else
    std::cerr << msg << std::endl;
#endif
}

/* save callback function pointer */
gdo::dl::message_callback_t gdo::dl::m_message_callback = gdo::dl::default_message_callback;

#endif // GDO_WRAP_FUNCTIONS || GDO_ENABLE_AUTOLOAD


/* library handle */
gdo::dl::handle_t gdo::dl::m_handle = nullptr;


/* symbol pointers; symbol names must be prefixed to avoid macro expansion */
%%type%% (*gdo::GDO_PTR_%%func_symbol%%)(%%args%%) = nullptr;
%%obj_type%% *gdo::GDO_PTR_%%obj_symbol%% = nullptr;


/* silence `unused reference' compiler warnings */
namespace gdo {
    template<typename T>
    void UNUSED_REF(T x) {
        static_cast<void>(x);
    }
}


/* Create versioned library names for DLLs, dylibs and DSOs.
 * make_libname("z",1) for example will return "libz.1.dylib" on macOS */
std::string gdo::make_libname(const std::string &name, const size_t api)
{
    /* prefix + name + middle + api + suffix */
#ifdef _WIN32
# ifdef __MINGW32__
    return "lib" + name + '-'    + std::to_string(api) + ".dll";
# else
    return         name + '-'    + std::to_string(api) + ".dll";
# endif
#elif defined(__APPLE__)
    return "lib" + name + '.'    + std::to_string(api) + ".dylib";
#elif defined(_AIX)
    UNUSED_REF(api);
    return "lib" + name                                + ".a";
#elif defined(__ANDROID__)
    UNUSED_REF(api);
    return "lib" + name                                + ".so";
#else
    return "lib" + name + ".so." + std::to_string(api);
#endif
}

#ifdef GDO_WINAPI
std::wstring gdo::make_libname(const std::wstring &name, const size_t api)
{
#ifdef __MINGW32__
    return L"lib" + name + L'-' + std::to_wstring(api) + L".dll";
#else
    return          name + L'-' + std::to_wstring(api) + L".dll";
#endif
}
#endif //GDO_WINAPI


#ifdef GDO_WINAPI


inline errno_t gdo::dl::mbs_wcs_conv(size_t *rv, wchar_t *out, size_t sz, const char *in, size_t count) {
    return ::mbstowcs_s(rv, out, sz, in, count);
}

inline errno_t gdo::dl::mbs_wcs_conv(size_t *rv, char *out, size_t sz, const wchar_t *in, size_t count) {
    return ::wcstombs_s(rv, out, sz, in, count);
}


/* convert between char and wchar_t */
template<typename T_out, typename T_in>
std::basic_string<T_out> gdo::dl::convert_string(const std::basic_string<T_in> &str_in)
{
    size_t len, n;
    T_out *buf;
    std::basic_string<T_out> str_out;

    if (str_in.empty()) {
        return {};
    }

    if (mbs_wcs_conv(&len, nullptr, 0, str_in.c_str(), 0) != 0 || len == 0) {
        return {};
    }

    buf = new T_out[len + 1];
    if (!buf) return {};

    if (mbs_wcs_conv(&n, buf, len+1, str_in.c_str(), len) != 0 || n == 0) {
        return {};
    }

    buf[len] = 0;
    str_out = buf;
    delete[] buf;

    return str_out;
}


/* clear error */
void gdo::dl::clear_error()
{
    m_errmsg.clear();
    m_werrmsg.clear();
    m_last_error = 0;
}


/* save last error (no extra message) */
void gdo::dl::save_error()
{
    m_last_error = ::GetLastError();
    m_errmsg.clear();
    m_werrmsg.clear();
}


/* save last error (narrow char message) */
void gdo::dl::save_error(const std::string &msg)
{
    m_last_error = ::GetLastError();
    m_errmsg = msg;
    m_werrmsg.clear();
}


/* save last error (wide char message) */
void gdo::dl::save_error(const std::wstring &msg)
{
    m_last_error = ::GetLastError();
    m_errmsg.clear();
    m_werrmsg = msg;
}


/* if m_handle is NULL */
void gdo::dl::set_error_invalid_handle()
{
    clear_error();
    m_last_error = ERROR_INVALID_HANDLE;
}


inline HMODULE gdo::dl::load_library_ex(const wchar_t *path) {
    return ::LoadLibraryExW(path, NULL, m_flags);
}

inline HMODULE gdo::dl::load_library_ex(const char *path) {
    return ::LoadLibraryExA(path, NULL, m_flags);
}


/**
 * Call the underlying LoadLibraryEx() function.
 *
 * According to MSDN only backward slash path separators shall be used,
 * so the path is transformed if needed.
 */
template<typename T>
void gdo::dl::transform_path_and_load_library(const std::basic_string<T> &filename, const T &fwd_slash, const T &bwd_slash)
{
    if (filename.find(fwd_slash) == std::basic_string<T>::npos) {
        /* no forward slash found */
        m_handle = load_library_ex(filename.c_str());
        return;
    }

    auto repl = [&] (T &c) {
        if (c == fwd_slash) {
            c = bwd_slash;
        }
    };

    auto copy = filename;
    std::for_each(copy.begin(), copy.end(), repl);

    m_handle = load_library_ex(copy.c_str());
}


/* load library */
void gdo::dl::load_lib(const std::string &filename, int flags, bool new_namespace)
{
    UNUSED_REF(new_namespace);

    m_wfilename.clear();
    m_filename = filename;
    m_flags = flags;

    transform_path_and_load_library<char>(filename, '/', '\\');
}


/* load library (wide character) */
void gdo::dl::load_lib(const std::wstring &filename, int flags, bool new_namespace)
{
    UNUSED_REF(new_namespace);

    m_filename.clear();
    m_wfilename = filename;
    m_flags = flags;

    transform_path_and_load_library<wchar_t>(filename, L'/', L'\\');
}


/* load symbol address */
template<typename T>
T gdo::dl::sym_load(const char *symbol)
{
    clear_error();

    /* cast to void* to supress compiler warnings */
    void *ptr = reinterpret_cast<void *>(::GetProcAddress(m_handle, symbol));

    if (!ptr) {
        save_error(symbol);
        return nullptr;
    }

    /* cast to function pointer type */
    return reinterpret_cast<T>(ptr);
}


inline DWORD gdo::dl::get_module_filename(wchar_t *buf, DWORD len) {
    return ::GetModuleFileNameW(m_handle, buf, len);
}

inline DWORD gdo::dl::get_module_filename(char *buf, DWORD len) {
    return ::GetModuleFileNameA(m_handle, buf, len);
}


/* get the module's full path using GetModuleFileName() */
template<typename T>
std::basic_string<T> gdo::dl::get_origin_from_module_handle()
{
    if (!lib_loaded()) {
        set_error_invalid_handle();
        return {};
    }

    T buf[32*1024];
    DWORD nSize = get_module_filename(buf, _countof(buf));

    if (nSize == 0 || nSize == _countof(buf)) {
        save_error("GetModuleFileName");
        return {};
    }

    return buf;
}


inline void gdo::dl::format_message(DWORD flags, DWORD msgId, DWORD langId, wchar_t *buf) {
    ::FormatMessageW(flags, NULL, msgId, langId, buf, 0, NULL);
}

inline void gdo::dl::format_message(DWORD flags, DWORD msgId, DWORD langId, char *buf) {
    ::FormatMessageA(flags, NULL, msgId, langId, buf, 0, NULL);
}


/* return a formatted error message */
template<typename T>
std::basic_string<T> gdo::dl::format_last_error_message()
{
    std::basic_string<T> str;
    T *buf = nullptr;

    format_message(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS |
        FORMAT_MESSAGE_MAX_WIDTH_MASK,
        m_last_error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<T*>(&buf));

    if (buf) {
        str = buf;
        ::LocalFree(buf);
    }

    return str;
}


#else
/*********************************** dlfcn ***********************************/


/* clear error */
void gdo::dl::clear_error()
{
    m_errmsg.clear();
    ::dlerror();
}


/* save last error */
void gdo::dl::save_error()
{
    const char *ptr = ::dlerror();
    m_errmsg = ptr ? ptr : "";
}

void gdo::dl::save_error(const std::string&)
{
    save_error();
}


/* if m_handle is NULL */
void gdo::dl::set_error_invalid_handle()
{
    clear_error();
    m_errmsg = "no library was loaded";
}


/* load library */
void gdo::dl::load_lib(const std::string &filename, int flags, bool new_namespace)
{
    clear_error();

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
    UNUSED_REF(new_namespace);
    m_handle = ::dlopen(filename.c_str(), m_flags);
#endif
}


/* load symbol address */
template<typename T>
T gdo::dl::sym_load(const char *symbol)
{
    clear_error();

    T ptr = reinterpret_cast<T>(::dlsym(m_handle, symbol));

    if (!ptr) {
        save_error();
        return nullptr;
    }

    return ptr;
}


#endif // !GDO_WINAPI


/* load library by filename */
template<typename T>
bool gdo::dl::load_filename(const T &filename, int flags, bool new_namespace)
{
    clear_error();

    /* consider it an error if the library was already loaded */
    if (lib_loaded()) {
        m_errmsg = "library already loaded";
        return false;
    }

    if (filename.empty()) {
        clear_error();
#ifdef GDO_WINAPI
        m_last_error = ERROR_INVALID_NAME;
#endif
        m_errmsg = "empty filename";
        return false;
    }

    load_lib(filename, flags, new_namespace);

    if (!lib_loaded()) {
        save_error(filename);
        return false;
    }

    return true;
}


/* c'tor (empty) */
gdo::dl::dl()
{
}


/* c'tor (set filename) */
gdo::dl::dl(const std::string &filename, int flags, bool new_namespace)
 : m_filename(filename),
   m_flags(flags),
   m_new_namespace(new_namespace)
{
}


#ifdef GDO_WINAPI
/* c'tor (set wide character filename) */
gdo::dl::dl(const std::wstring &filename, int flags, bool new_namespace)
 : m_wfilename(filename),
   m_flags(flags),
   m_new_namespace(new_namespace)
{
}
#endif


/* d'tor */
gdo::dl::~dl()
{
    if (m_free_lib_in_dtor) {
        free(true);
    }
}

/* load library */
bool gdo::dl::load(const std::string &filename, int flags, bool new_namespace)
{
    return load_filename(filename, flags, new_namespace);
}


#ifdef GDO_WINAPI
/* load library (wide characters version) */
bool gdo::dl::load(const std::wstring &filename, int flags, bool new_namespace)
{
    return load_filename(filename, flags, new_namespace);
}
#endif


/* load library */
bool gdo::dl::load()
{
#ifdef GDO_WINAPI
    /* prefer wide characters filename */
    if (!m_wfilename.empty()) {
        return load(m_wfilename, m_flags, m_new_namespace);
    }
#endif
    return load(m_filename, m_flags, m_new_namespace);
}


/* load library and symbols */
bool gdo::dl::load_lib_and_symbols()
{
    return (load() && load_all_symbols());
}


/* check if library is loaded */
bool gdo::dl::lib_loaded()
{
    return (m_handle != nullptr);
}


/* return the flags used to load the library */
int gdo::dl::flags() const
{
    return m_flags;
}


/* load all symbols */
bool gdo::dl::load_all_symbols()
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
    if ((GDO_PTR_%%symbol%% =@
        sym_load<%%sym_type%%>@
            ("%%symbol%%")) == nullptr) {@
        return false;@
    }@

    clear_error();

    return all_symbols_loaded();
}


/* load a specific symbol */
bool gdo::dl::load_symbol(int symbol_num)
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
        if (!GDO_PTR_%%symbol%%) {@
            GDO_PTR_%%symbol%% =@
                sym_load<%%sym_type%%>@
                    ("%%symbol%%");@
        }@
        return (GDO_PTR_%%symbol%% != nullptr);@

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
bool gdo::dl::load_symbol(const char *symbol)
{
    auto error_unknown_symbol = [&] ()
    {
#ifdef GDO_WINAPI
        m_last_error = ERROR_NOT_FOUND;
#endif
        m_errmsg = "unknown symbol: ";
        m_errmsg += symbol;
    };

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
        return false;
    }

    /* check symbol prefix */
    const size_t n = sizeof(GDO_COMMON_PREFIX) - 1;

    if (n > 0 && ::strncmp(symbol, GDO_COMMON_PREFIX, n) != 0) {
        error_unknown_symbol();
        return false;
    }

    /* symbols */
    const size_t len = ::strlen(symbol);
    const char *ptr;

    ptr = "%%symbol%%";@
    @
    if (len == sizeof("%%symbol%%") - 1 &&@
        ::strcmp(symbol + n, ptr + n) == 0)@
    {@
        if (!GDO_PTR_%%symbol%%) {@
            GDO_PTR_%%symbol%% =@
                sym_load<%%sym_type%%>@
                    ("%%symbol%%");@
        }@
        return (GDO_PTR_%%symbol%% != nullptr);@
    }@

    error_unknown_symbol();

    return false;
}


/* check if ALL symbols were loaded */
bool gdo::dl::all_symbols_loaded() const
{
    if (true
        && GDO_PTR_%%symbol%% != nullptr
    ) {
        return true;
    }

    return false;
}


/* check if NO symbols were loaded */
bool gdo::dl::no_symbols_loaded() const
{
    if (true
        && GDO_PTR_%%symbol%% == nullptr
    ) {
        return true;
    }

    return false;
}


/* check if ANY symbol was loaded */
bool gdo::dl::any_symbol_loaded() const
{
    if (false
        || GDO_PTR_%%symbol%% != nullptr
    ) {
        return true;
    }

    return false;
}


/* free library; always succeeds if `force == true` */
bool gdo::dl::free(bool force)
{
    auto release_lib_handle = []() -> bool
    {
#ifdef GDO_WINAPI
        return (::FreeLibrary(m_handle) == TRUE);
#else
        return (::dlclose(m_handle) == 0);
#endif
    };

    clear_error();

    /* don't exit on error if `force` was enabled */
    const bool exit_on_error = !force;

    if (lib_loaded() && !release_lib_handle() && exit_on_error) {
        save_error();
        return false;
    }

    clear_error();

    m_handle = nullptr;
    GDO_PTR_%%symbol%% = nullptr;

    return true;
}


/* whether to free the library in the class destructor */
void gdo::dl::free_lib_in_dtor(bool b)
{
    m_free_lib_in_dtor = b;
}


#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

/* Set a message callback function. */
void gdo::dl::message_callback(gdo::dl::message_callback_t cb)
{
    m_message_callback = cb;
}


/* Get a pointer to the message callback function. */
gdo::dl::message_callback_t gdo::dl::message_callback()
{
    return m_message_callback;
}

#endif // GDO_WRAP_FUNCTIONS || GDO_ENABLE_AUTOLOAD


#ifdef GDO_WINAPI

/* get path of loaded library */
std::string gdo::dl::origin()
{
    return get_origin_from_module_handle<char>();
}

std::wstring gdo::dl::origin_w()
{
    return get_origin_from_module_handle<wchar_t>();
}


/* retrieve the last error */
std::string gdo::dl::error()
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
        buf.insert(0, convert_string<char, wchar_t>(m_werrmsg));
    }

    return buf;
}

std::wstring gdo::dl::error_w()
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
        buf.insert(0, convert_string<wchar_t, char>(m_errmsg));
    }

    return buf;
}


/* get filename passed to load */
std::string gdo::dl::filename()
{
    return m_filename.empty() ? convert_string<char, wchar_t>(m_wfilename) : m_filename;
}

std::wstring gdo::dl::filename_w()
{
    return m_wfilename.empty() ? convert_string<wchar_t, char>(m_filename) : m_wfilename;
}


#else
/*********************************** dlfcn ***********************************/


/* get path of loaded library */
std::string gdo::dl::origin()
{
    clear_error();

    if (!lib_loaded()) {
        set_error_invalid_handle();
        return {};
    }

#ifdef _WIN32

    /* dlfcn-win32:
     * The handle returned by dlopen() is a `HMODULE' casted to `void *'.
     * We can directly use GetModuleFileNameA() to receive the DLL path
     * and don't need to invoke dladdr() on a loaded symbol address. */

    char buf[32*1024];

    DWORD nSize = ::GetModuleFileNameA(reinterpret_cast<HMODULE>(m_handle),
        buf, sizeof(buf));

    if (nSize == 0 || nSize == sizeof(buf)) {
        m_errmsg = "GetModuleFileNameA() failed to get library path";
        return {};
    }

    return buf;

#elif defined(GDO_HAVE_DLINFO)

    struct link_map *lm = nullptr;

    if (::dlinfo(m_handle, RTLD_DI_LINKMAP, reinterpret_cast<void *>(&lm)) == -1) {
        save_error();
        return {};
    }

    return lm->l_name ? lm->l_name : "";

#else

    /* use dladdr() to get the library path from a symbol pointer */
    std::string fname;

    if (no_symbols_loaded()) {
        m_errmsg = "no symbols were loaded";
        return {};
    }

    auto get_fname = [&fname] (const void *ptr)
    {
        _GDO_Dl_info info;

        if (ptr && ::dladdr(ptr, &info) != 0 && info.dli_fname) {
            fname = info.dli_fname;
        }
    };

    get_fname(reinterpret_cast<void *>(GDO_PTR_%%symbol%%));@
    if (!fname.empty()) return fname;

    m_errmsg = "dladdr() failed to get library path";

    return {};

#endif // !_WIN32
}


/* retrieve the last error */
std::string gdo::dl::error() const
{
    return m_errmsg.empty() ? "no error" : m_errmsg;
}


/* get filename passed to load */
std::string gdo::dl::filename() const
{
    return m_filename;
}

#endif // !GDO_WINAPI
%PARAM_SKIP_REMOVE_BEGIN%


#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)
namespace gdo
{
    namespace wrap
    {
#ifdef GDO_ENABLE_AUTOLOAD
        auto loader = dl(GDO_DEFAULT_LIBA);
#endif

        /* used by wrapper functions */
        void _check(int load, bool sym_loaded, const char *sym)
        {
            /* error message lambda function */
            auto print_error = [] (const std::string &msg)
            {
                auto cb = dl::message_callback();

                if (cb) {
                    cb(msg.c_str());
                } else {
                    std::cerr << msg << std::endl;
                }
            };

#ifdef GDO_ENABLE_AUTOLOAD

            /* load library and function(s) if needed */

            UNUSED_REF(sym_loaded);

            if (!loader.lib_loaded()) {
                loader.load();
            }

# ifdef GDO_ENABLE_AUTOLOAD_LAZY
            /* load a specific symbol */
            if (loader.load_symbol(load)) {
                return;
            }
# else
            /* load all symbols */
            UNUSED_REF(load);

            if (loader.load_all_symbols()) {
                return;
            }
# endif

            /* error */

            std::string s = "error: ";
            std::string msg = loader.error();

            if (msg.find(GDO_DEFAULT_LIBA) == std::string::npos) {
                /* library name is not part of error message */
                s += GDO_DEFAULT_LIBA ": ";
            }

            s += sym;
            s += ": ";
            s += msg;

            print_error(s);
            std::exit(1);

#else //!GDO_ENABLE_AUTOLOAD

            /* check if library and symbol were loaded */

            UNUSED_REF(load);

            if (dl::lib_loaded() && sym_loaded) {
                return;
            }

            /* error */

            std::string msg = "fatal error: ";
            msg += sym;

            if (!dl::lib_loaded()) {
                msg += ": library not loaded";
            } else {
                msg += ": symbol not loaded";
            }

            print_error(msg);
            std::abort();

#endif //!GDO_ENABLE_AUTOLOAD
        }
    } /* namespace wrap */
} /* namespace gdo */

#endif // GDO_WRAP_FUNCTIONS || GDO_ENABLE_AUTOLOAD
%PARAM_SKIP_END%


#if !defined(GDO_SEPARATE) && \
    !defined(GDO_DISABLE_ALIASING)

/* aliases to raw function pointers */
#if !defined(GDO_WRAP_IS_VISIBLE)
# define %%func_symbol_pad%% GDO_FUNC_ALIAS(%%func_symbol%%)
#endif

/* aliases to raw object pointers */
#define %%obj_symbol_pad%% GDO_ALIAS_%%obj_symbol%%

#endif //!GDO_SEPARATE
