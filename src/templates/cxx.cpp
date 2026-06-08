#if defined(_MSC_VER) && defined(GDO_USE_MESSAGE_BOX)
# pragma comment(lib, "user32.lib")  /* dependency for MessageBox() */
#endif

#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>


#ifdef GDO_WINAPI
# define GDO_SET_LAST_ERRNO(x)  do { m_last_errno = x; } while(0)
#else
# define GDO_SET_LAST_ERRNO(x)  /**/
#endif


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
gdo::dl::msgcb_t gdo::dl::m_message_callback = gdo::dl::default_message_callback;

#endif // GDO_WRAP_FUNCTIONS || GDO_ENABLE_AUTOLOAD


/* library handle */
gdo_hmod_t gdo::dl::m_handle = nullptr;


/* symbol pointers; symbol names must be prefixed to avoid macro expansion */
%%type%% (*GDO_RAWPTR_%%func_symbol%%)(%%args%%) = nullptr;
%%obj_type%% *GDO_RAWPTR_%%obj_symbol%% = nullptr;


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


bool gdo::dl::mbs_wcs_conv(size_t *retval, wchar_t *out, size_t size, const char *in) {
    return (::mbstowcs_s(retval, out, size, in, _TRUNCATE) == 0);
}

bool gdo::dl::mbs_wcs_conv(size_t *retval, char *out, size_t size, const wchar_t *in) {
    return (::wcstombs_s(retval, out, size, in, _TRUNCATE) == 0);
}


/* convert between char and wchar_t */
template<typename T_in, typename T_out>
bool gdo::dl::convert_string(const std::basic_string<T_in> &str_in, std::basic_string<T_out> &str_out)
{
    size_t len = 0;
    size_t n = 0;

    if (str_in.empty()) {
        /* empty string in, empty string out */
        str_out.clear();
        return true;
    }

    /* get length of converted string (including null terminator) */
    if (mbs_wcs_conv(&len, nullptr, 0, str_in.c_str()) && len > 1) {
        /* allocate buffer with zeros */
        str_out.assign(len, 0);

        /* get pointer to buffer */
        auto buf = const_cast<T_out *>(str_out.data());

        /* convert string */
        if (mbs_wcs_conv(&n, buf, len, str_in.c_str()) && n == len) {
            return true;
        }
    }

    /* clear output buffer on error */
    str_out.clear();

    return false;
}


/* clear error */
void gdo::dl::clear_error()
{
    m_last_errno = 0;
    m_errmsg.clear();
    m_werrmsg.clear();
    m_formatted.clear();
    m_wformatted.clear();
}


/* save last error (no extra message) */
void gdo::dl::save_error()
{
    clear_error();
    m_last_errno = ::GetLastError();
}


/* save last error (narrow char message) */
void gdo::dl::save_error(const std::string &msg)
{
    clear_error();
    m_last_errno = ::GetLastError();
    m_errmsg = msg;
}


/* save last error (wide char message) */
void gdo::dl::save_error(const std::wstring &msg)
{
    clear_error();
    m_last_errno = ::GetLastError();
    m_werrmsg = msg;
}


/* if m_handle is NULL */
void gdo::dl::set_error_invalid_handle()
{
    clear_error();
    m_last_errno = ERROR_INVALID_HANDLE;
}


HMODULE gdo::dl::load_library_ex(const std::wstring &filename) {
    return ::LoadLibraryExW(filename.c_str(), NULL, m_flags);
}


HMODULE gdo::dl::load_library_ex(const std::string &filename)
{
    std::wstring wfilename;

    if (m_convert_filename_to_wcs) {
        if (convert_string<char, wchar_t>(filename, wfilename)) {
            return ::LoadLibraryExW(wfilename.c_str(), NULL, m_flags);
        }

        m_last_errno = ERROR_INVALID_NAME;
        m_errmsg = "mbstowcs_s: failed to convert filename";
        return NULL;
    }

    return ::LoadLibraryExA(filename.c_str(), NULL, m_flags);
}


/**
 * load library
 *
 * According to MSDN only backward slash path separators shall be used,
 * so the path is transformed if needed.
 */
template<typename T, typename U = typename T::value_type>
void gdo::dl::load_lib(const T &filename)
{
    const auto fwd_slash = get_char<U>('/', L'/');
    const auto bwd_slash = get_char<U>('\\', L'\\');

    clear_error();

    if (filename.find(fwd_slash) == T::npos) {
        /* no forward slash found */
        m_handle = load_library_ex(filename);
        return;
    }

    auto replace = [&] (U &c) {
        if (c == fwd_slash) {
            c = bwd_slash;
        }
    };

    auto copy = filename;
    std::for_each(copy.begin(), copy.end(), replace);

    m_handle = load_library_ex(copy);
}


DWORD gdo::dl::get_module_filename(wchar_t *buf, DWORD len) {
    return ::GetModuleFileNameW(m_handle, buf, len);
}

DWORD gdo::dl::get_module_filename(char *buf, DWORD len) {
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

    T buf[GDO_BUFLEN];
    DWORD nSize = get_module_filename(buf, _countof(buf));

    if (nSize == 0 || nSize == _countof(buf)) {
        auto msg = get_string<T>("GetModuleFileNameA()", L"GetModuleFileNameW()");
        save_error(msg);
        return {};
    }

    return buf;
}


void gdo::dl::format_message(DWORD msgId, DWORD langId, wchar_t *buf) {
    ::FormatMessageW(GDO_FORMAT_MESSAGE_FLAGS, NULL, msgId, langId, buf, 0, NULL);
}

void gdo::dl::format_message(DWORD msgId, DWORD langId, char *buf) {
    ::FormatMessageA(GDO_FORMAT_MESSAGE_FLAGS, NULL, msgId, langId, buf, 0, NULL);
}


/* return a formatted error message */
template<typename T_in, typename T_out>
std::basic_string<T_out> gdo::dl::format_error_message(const std::basic_string<T_in> &msg_Tin,
                                                       const std::basic_string<T_out> &msg_Tout)
{
    std::basic_string<T_out> str, tmp;
    T_out *buf = nullptr;

    format_message(m_last_errno,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        reinterpret_cast<T_out *>(&buf));

    if (buf) {
        str = buf;
        ::LocalFree(buf);
    } else {
        /* FormatMessage() failed, save the error code */
        str = get_string<T_out>("error code: ", L"error code: ");
        str += to_string<T_out>(m_last_errno);
    }

    /* put custom message in front */
    if (!msg_Tout.empty()) {
        str.insert(0, get_string<T_out>(": ", L": "));
        str.insert(0, msg_Tout);
    } else if (!msg_Tin.empty() && convert_string<T_in, T_out>(msg_Tin, tmp)) {
        str.insert(0, get_string<T_out>(": ", L": "));
        str.insert(0, tmp);
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
void gdo::dl::save_error(const std::string&)
{
    const char *ptr = ::dlerror();
    m_errmsg = ptr ? ptr : "";
}


/* if m_handle is NULL */
void gdo::dl::set_error_invalid_handle()
{
    clear_error();
    m_errmsg = "no library was loaded";
}


/* load library */
void gdo::dl::load_lib(const std::string &filename)
{
    clear_error();
    m_handle = _gdo_call_dlopen(filename.c_str(), m_flags, m_new_namespace);
}


#ifdef _AIX
std::string gdo::dl::aix_fname_from_symbol(struct ld_info *info, uint8_t *sym)
{
    const char *member = NULL;
    const char *path = _gdo_aix_parse_ldinfo(info, sym, reinterpret_cast<const char **>(&member));

    if (!path) {
        return {};
    }

    /* check for an archive member name */
    if (member && member[0] != 0) {
        std::string str = path;
        str += '(';
        str += member;
        str += ')';

        return str;
    }

    return path;
}
#endif //_AIX

#endif // !GDO_WINAPI


/* load symbol address */
template<typename T>
T gdo::dl::sym_load(const char *symbol)
{
    T ptr = reinterpret_cast<T>(_gdo_call_dlsym(m_handle, symbol));

    if (!ptr) {
        save_error();
    }

    return ptr;
}


/* load library by filename */
template<typename T>
bool gdo::dl::load_filename(const T &filename)
{
    clear_error();

    /* consider it an error if the library was already loaded */
    if (lib_loaded()) {
        m_errmsg = "library already loaded";
        return false;
    }

    if (filename.empty()) {
        GDO_SET_LAST_ERRNO(ERROR_INVALID_NAME);
        m_errmsg = "empty filename";
        return false;
    }

    load_lib(filename);

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

/* c'tor (set narrow character filename and whether to convert name) */
gdo::dl::dl(bool convert, const std::string &filename, int flags)
 : m_convert_filename_to_wcs(convert),
   m_filename(filename),
   m_flags(flags)
{
}


/* c'tor (set wide character filename) */
gdo::dl::dl(const std::wstring &filename, int flags)
 : m_wfilename(filename),
   m_flags(flags)
{
}

#endif //GDO_WINAPI


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
#ifdef GDO_WINAPI
    m_wfilename.clear();
#endif
    m_filename      = filename;
    m_flags         = flags;
    m_new_namespace = new_namespace;

    return load_filename(m_filename);
}


#ifdef GDO_WINAPI

/* load library */
bool gdo::dl::load(bool convert, const std::string &filename, int flags)
{
    m_convert_filename_to_wcs = convert;
    m_wfilename.clear();

    m_filename = filename;
    m_flags    = flags;

    return load_filename(m_filename);
}


/* load library (wide characters version) */
bool gdo::dl::load(const std::wstring &filename, int flags)
{
    m_filename.clear();
    m_wfilename = filename;
    m_flags     = flags;

    return load_filename(m_wfilename);
}

#endif //GDO_WINAPI


/* load library */
bool gdo::dl::load()
{
#ifdef GDO_WINAPI
    if (!m_wfilename.empty()) {
        return load_filename(m_wfilename);
    }
#endif

    return load_filename(m_filename);
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
    if ((GDO_RAWPTR_%%symbol%% =@
        sym_load<%%sym_type%%>@
            ("%%symbol%%")) == nullptr) {@
        return false;@
    }@

    return true;
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
        if (!GDO_RAWPTR_%%symbol%%) {@
            GDO_RAWPTR_%%symbol%% =@
                sym_load<%%sym_type%%>@
                    ("%%symbol%%");@
        }@
        return (GDO_RAWPTR_%%symbol%% != nullptr);@

    default:
        break;
    }

    GDO_SET_LAST_ERRNO(ERROR_NOT_FOUND);
    m_errmsg = "unknown symbol number: " + std::to_string(symbol_num);

    return false;
}


/* load a specific symbol by name */
bool gdo::dl::load_symbol(const char *symbol)
{
    clear_error();

    if (!lib_loaded()) {
        set_error_invalid_handle();
        return false;
    }

    if (!symbol || *symbol == 0) {
        GDO_SET_LAST_ERRNO(ERROR_INVALID_PARAMETER);
        m_errmsg = "empty symbol name";
        return false;
    }

    /* check symbol prefix */
    const char pfx[] = GDO_COMMON_PREFIX;
    const size_t pfxlen = sizeof(pfx) - 1;

    if (pfxlen == 0 || ::strncmp(symbol, pfx, pfxlen) == 0) {
        const size_t len = ::strlen(symbol);
        const char *curr;
        size_t curr_len;
@
        curr = "%%symbol%%";@
        curr_len = sizeof("%%symbol%%") - 1;@
        if (len == curr_len && ::strcmp(symbol + pfxlen, curr + pfxlen) == 0) {@
            if (!GDO_RAWPTR_%%symbol%%) {@
                GDO_RAWPTR_%%symbol%% =@
                    sym_load<%%sym_type%%>@
                        ("%%symbol%%");@
            }@
            return (GDO_RAWPTR_%%symbol%% != nullptr);@
        }
    }

    GDO_SET_LAST_ERRNO(ERROR_NOT_FOUND);
    m_errmsg = "unknown symbol: " + std::string(symbol);

    return false;
}


/* check if ALL symbols were loaded */
bool gdo::dl::all_symbols_loaded() const
{
    if (true
        && GDO_RAWPTR_%%symbol%% != nullptr
    ) {
        return true;
    }

    return false;
}


/* check if NO symbols were loaded */
bool gdo::dl::no_symbols_loaded() const
{
    if (true
        && GDO_RAWPTR_%%symbol%% == nullptr
    ) {
        return true;
    }

    return false;
}


/* check if ANY symbol was loaded */
bool gdo::dl::any_symbol_loaded() const
{
    if (false
        || GDO_RAWPTR_%%symbol%% != nullptr
    ) {
        return true;
    }

    return false;
}


/* free library */
bool gdo::dl::free(bool force)
{
    bool rv = true;

    clear_error();

    if (lib_loaded()) {
        rv = _gdo_call_dlclose(m_handle);

        /* always succeed if `force == true` */
        if (force) {
            rv = true;
        }
    }

    if (!rv) {
#ifdef GDO_WINAPI
        save_error("FreeLibrary()");
#else
        save_error();
#endif
        return false;
    }

    /* set pointers back to NULL */
    m_handle = nullptr;
    GDO_RAWPTR_%%symbol%% = nullptr;

    return true;
}


/* whether to free the library in the class destructor */
void gdo::dl::free_lib_in_dtor(bool b)
{
    m_free_lib_in_dtor = b;
}

bool gdo::dl::free_lib_in_dtor() const
{
    return m_free_lib_in_dtor;
}


#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

/* Set a message callback function. */
void gdo::dl::message_callback(gdo::dl::msgcb_t cb)
{
    m_message_callback = cb;
}


/* Get a pointer to the message callback function. */
gdo::dl::msgcb_t gdo::dl::message_callback()
{
    if (m_message_callback) {
        return m_message_callback;
    }

    return default_message_callback;
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
    if (m_formatted.empty()) {
        m_formatted = format_error_message<wchar_t, char>(m_werrmsg, m_errmsg);
    }

    return m_formatted;
}

std::wstring gdo::dl::error_w()
{
    if (m_wformatted.empty()) {
        m_wformatted = format_error_message<char, wchar_t>(m_errmsg, m_werrmsg);
    }

    return m_wformatted;
}


/* get the unmodified filename passed to load the library */
std::string gdo::dl::filename()
{
    if (m_filename.empty() && !m_wfilename.empty()) {
        convert_string<wchar_t, char>(m_wfilename, m_filename);
    }

    return m_filename;
}

std::wstring gdo::dl::filename_w()
{
    if (m_wfilename.empty() && !m_filename.empty()) {
        convert_string<char, wchar_t>(m_filename, m_wfilename);
    }

    return m_wfilename;
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

#ifdef GDO_DLFCN_WIN32

    /* dlfcn-win32:
     * The handle returned by dlopen() is a `HMODULE' casted to `void *'.
     * We can directly use GetModuleFileNameA() to receive the DLL path
     * and don't need to invoke dladdr() on a loaded symbol address. */

    char buf[GDO_BUFLEN];

    DWORD nSize = ::GetModuleFileNameA(reinterpret_cast<HMODULE>(m_handle),
        buf, sizeof(buf));

    if (nSize == 0) {
        m_errmsg = "failed to get the library path";
        return {};
    } else if (nSize == sizeof(buf)) {
        m_errmsg = "buffer is too small to hold the library path";
        return {};
    }

    return buf;

#elif defined(_AIX)

    uint8_t buf[GDO_AIX_LOADQUERY_BUFLEN];
    std::string fname;

    if (no_symbols_loaded()) {
        m_errmsg = "no symbols were loaded";
        return {};
    }

    if (loadquery(L_GETINFO, buf, sizeof(buf)) != -1) {
        auto info = reinterpret_cast<struct ld_info *>(buf);
        uint8_t *ptr;
@
        ptr = reinterpret_cast<uint8_t *>(GDO_RAWPTR_%%symbol%%);@
        fname = aix_fname_from_symbol(info, ptr);@
        if (!fname.empty()) { return fname; }
    }

    m_errmsg = "failed to get the library path";
    return {};

#elif defined(GDO_HAVE_INFO)

    /* use dlinfo() to get a link map */
    struct link_map *lm = nullptr;

    if (::dlinfo(m_handle, RTLD_DI_LINKMAP, &lm) == -1) {
        save_error();
        return {};
    }

    if (!lm->l_name || lm->l_name[0] == 0) {
        m_errmsg = "dlinfo() failed to get library path";
        return {};
    }

# ifdef __linux__
    char *path;

    /* try to get the full library path from /proc/self/maps */
    if (lm->l_name[0] != '/' && (path = _gdo_fullpath_proc_self_maps(lm)) != NULL) {
        std::string s = path;
        free(path);
        return s;
    }
# endif

    return lm->l_name;

#elif defined(GDO_HAVE_DLADDR)

    /* use dladdr() to get the library path from a symbol pointer */
    Dl_info info;

    if (no_symbols_loaded()) {
        m_errmsg = "no symbols were loaded";
        return {};
    }

    if (_gdo_call_dladdr(reinterpret_cast<void *>(GDO_RAWPTR_%%symbol%%), &info)) {@
        return info.dli_fname;@
    }@

    m_errmsg = "dladdr() failed to get library path";
    return {};

#else

    m_errmsg = "function not implemented";
    return {};

#endif
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
        auto _loader = dl(GDO_DEFAULT_LIB);
#endif

        /* used by wrapper functions (assuming symbol was not loaded) */
        void not_loaded(int load, const char *sym)
        {
            std::stringstream sstr;

            auto msg_callback = dl::message_callback();

#ifdef GDO_ENABLE_AUTOLOAD

            /* load library */
            if (!_loader.lib_loaded()) {
                _loader.load();
            }

# ifdef GDO_ENABLE_AUTOLOAD_LAZY
            /* load a specific symbol */
            if (_loader.load_symbol(load)) {
                return;
            }
# else
            /* load all symbols */
            if (_loader.load_all_symbols()) {
                return;
            }

            UNUSED_REF(load);
# endif

            std::string msg = _loader.error();

            if (msg.find(GDO_DEFAULT_LIBA) != std::string::npos) {
                /* library name is already part of error message */
                sstr << "error: " << sym << ": " << msg;
            } else {
                sstr << "error: " GDO_DEFAULT_LIB ": " << sym << ": " << msg;
            }

            msg_callback(sstr.str().c_str());
            std::exit(1);

#else //!GDO_ENABLE_AUTOLOAD

            /* don't load anything, print an error message and abort */

            if (!dl::lib_loaded()) {
                sstr << "fatal error: " << sym << ": library not loaded";
            } else {
                sstr << "fatal error: " << sym << ": symbol not loaded";
            }

            UNUSED_REF(load);
            msg_callback(sstr.str().c_str());
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
#if !defined(GDO_WRAP_VISIBILITY)
# define %%func_symbol_pad%% GDO_FUNC_ALIAS(%%func_symbol%%)
#endif

/* aliases to raw object pointers */
#define %%obj_symbol_pad%% *GDO_RAWPTR_%%obj_symbol%%

#endif //!GDO_SEPARATE

#undef GDO_SET_LAST_ERRNO
