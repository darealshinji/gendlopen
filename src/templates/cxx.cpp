#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)
gdo::dl::message_callback_t gdo::dl::m_message_callback = nullptr;
#endif

/* library handle */
gdo::dl::handle_t gdo::dl::m_handle = nullptr;

/* symbol pointers */
%%type%% (*gdo::ptr::%%func_symbol%%)(%%args%%) = nullptr;
%%obj_type%% *gdo::ptr::%%obj_symbol%% = nullptr;


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
    UNUSED_VAL_(api);
    return "lib" + name                                + ".a";
#elif defined(__ANDROID__)
    UNUSED_VAL_(api);
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
%PARAM_SKIP_REMOVE_BEGIN%


#if defined(GDO_WRAP_FUNCTIONS) || defined(GDO_ENABLE_AUTOLOAD)

#include <iostream>
#include <cstdlib>

namespace gdo
{
    static inline void print_error_msg(const std::string &msg)
    {
        auto cb = dl::message_callback();

        if (cb) {
            cb(msg.c_str());
        } else {
            std::cerr << msg << std::endl;
        }
    }
}

#endif // GDO_WRAP_FUNCTIONS || GDO_ENABLE_AUTOLOAD


/* #define empty hooks by default */
#ifndef GDO_HOOK_%%func_symbol%%@
#define GDO_HOOK_%%func_symbol%%(...) /**/@
#endif

#if defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)


namespace gdo
{
    namespace wrap
    {
        static void check_if_loaded(bool sym_loaded, const char *sym)
        {
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

            print_error_msg(msg);
            std::abort();
        }
    }
}


/* function wrappers (functions with `...' arguments are omitted) */
@
GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    const bool sym_loaded = (gdo::ptr::%%func_symbol%% != nullptr);@
    gdo::wrap::check_if_loaded(sym_loaded, "%%func_symbol%%");@
    GDO_HOOK_%%func_symbol%%(%%notype_args%%);@
    %%return%% gdo::ptr::%%func_symbol%%(%%notype_args%%);@
}


#elif defined(GDO_ENABLE_AUTOLOAD)


namespace gdo
{
    namespace autoload
    {
        auto loader = dl(GDO_DEFAULT_LIBA);

        /* used internally by wrapper functions */
        void quick_load(int symbol_num, const char *sym)
        {
            if (!loader.lib_loaded()) {
                loader.load();
            }

#ifdef GDO_DELAYLOAD
            /* load a specific symbol */
            if (loader.load_symbol(symbol_num)) {
                return;
            }
#else
            /* load all symbols */
            UNUSED_VAL_(symbol_num);

            if (loader.load_all_symbols()) {
                return;
            }
#endif

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

            print_error_msg(s);
            std::exit(1);
        }
    }
}


/* autoload function wrappers (functions with `...' arguments are omitted) */
@
GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    gdo::autoload::quick_load(GDO_LOAD_%%func_symbol%%, "%%func_symbol%%");@
    GDO_HOOK_%%func_symbol%%(%%notype_args%%);@
    %%return%% gdo::ptr::%%func_symbol%%(%%notype_args%%);@
}

#endif //GDO_ENABLE_AUTOLOAD
%PARAM_SKIP_END%


#if !defined(GDO_SEPARATE) /* single header file */

/* aliases to raw function pointers */
#if !defined(GDO_DISABLE_ALIASING) && !defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)
#define %%func_symbol_pad%% GDO_ALIAS_%%func_symbol_pad%%
#endif

/* aliases to raw object pointers */
#if !defined(GDO_DISABLE_ALIASING)
#define %%obj_symbol_pad%% GDO_ALIAS_%%obj_symbol_pad%%
#endif

#endif //GDO_SEPARATE && !GDO_INCLUDED_IN_BODY
