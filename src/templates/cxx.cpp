#ifdef GDO_HAS_MSG_CB
gdo::dl::message_callback_t gdo::dl::m_message_callback = nullptr;
#endif
gdo::dl::handle_t gdo::dl::m_handle = nullptr;

%%type%% (*gdo::ptr::%%func_symbol%%)(%%args%%) = nullptr;
%%obj_type%% *gdo::ptr::%%obj_symbol%% = nullptr;


/* Create versioned library names for DLLs, dylibs and DSOs.
 * make_libname("z",1) for example will return "libz-1.dll", "libz.1.dylib" or "libz.so.1" */
std::string gdo::make_libname(const std::string &name, const size_t api)
{
#ifdef _WIN32
    return "lib" + name + '-' + std::to_string(api) + ".dll";
#elif defined(__APPLE__) && defined(__MACH__)
    return "lib" + name + '.' + std::to_string(api) + ".dylib";
#elif defined(_AIX)
    UNUSED_VAL_(api);
    return "lib" + name + ".a";
#elif defined(__ANDROID__)
    UNUSED_VAL_(api);
    return "lib" + name + ".so";
#else
    return "lib" + name + ".so." + std::to_string(api);
#endif
}

#ifdef GDO_WINAPI
std::wstring gdo::make_libname(const std::wstring &name, const size_t api);
{
    return L"lib" + name + L'-' + std::to_wstring(api) + L".dll";
}
#endif //GDO_WINAPI
%PARAM_SKIP_REMOVE_BEGIN%


#ifdef GDO_HAS_MSG_CB

#include <iostream>
#include <cstdlib>


/* helpers used by function wrappers */
namespace gdo
{
    namespace helper
    {
        static void error_exit(const char *msg)
        {
            auto cb = gdo::dl::message_callback();

            if (cb) {
                cb(msg);
            } else {
                std::cerr << msg << std::endl;
            }

            std::exit(1);
        }
    }
}

#endif //GDO_HAS_MSG_CB


#if defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)


/* function wrappers */
@
GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    if (!gdo::ptr::%%func_symbol%%) {@
        gdo::helper::error_exit("error: symbol `%%func_symbol%%' was not loaded");@
    }@
    %%return%% gdo::ptr::%%func_symbol%%(%%notype_args%%);@
}


#elif defined(GDO_ENABLE_AUTOLOAD)


namespace gdo
{
    namespace /* anonymous */ {
        auto _gdo_al = gdo::dl(GDO_DEFAULT_LIBA);
    }

    namespace helper
    {
        /* used internally by wrapper functions, symbol is never NULL */
        static void quick_load(int symbol_num, const char *symbol)
        {
            if (!_gdo_al.load()) {
                std::string msg = "error loading library `" GDO_DEFAULT_LIBA "':\n" + _gdo_al.error();
                error_exit(msg.c_str());
            }

#ifdef GDO_DELAYLOAD
            bool loaded = _gdo_al.load_symbol(symbol_num);
#else
            bool loaded = _gdo_al.load_all_symbols();
            UNUSED_VAL_(symbol_num);
#endif

            if (!loaded) {
                std::string msg = "error in auto-loading wrapper function `gdo::autoload::";
                msg += symbol + ("': " + _gdo_al.error());
                error_exit(msg.c_str());
            }
        }
    }
}


/* autoload function wrappers */
@
GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    gdo::helper::quick_load(GDO_LOAD_%%func_symbol%%, "%%func_symbol%%");@
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
