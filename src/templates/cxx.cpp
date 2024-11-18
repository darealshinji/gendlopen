#line 2 "<built-in>/cxx.cpp"

#ifdef GDO_HAS_MSG_CB
gdo::dl::message_callback_t gdo::dl::m_message_callback = nullptr;
#endif
gdo::dl::handle_t gdo::dl::m_handle = nullptr;

%%type%% (*gdo::dl::m_ptr_%%func_symbol%%)(%%args%%) = nullptr;
%%obj_type%% *gdo::dl::m_ptr_%%obj_symbol%% = nullptr;
%PARAM_SKIP_REMOVE_BEGIN%



/* helpers used by function wrappers */
namespace gdo
{
    namespace helper
    {
#ifdef GDO_HAS_MSG_CB
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
#endif //GDO_HAS_MSG_CB
    }
}


#if defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)


/* function wrappers */
@
GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    if (!gdo::dl::m_ptr_%%func_symbol%%) {@
        gdo::helper::error_exit("error: symbol `%%func_symbol%%' was not loaded");@
    }@
    %%return%% gdo::dl::m_ptr_%%func_symbol%%(%%notype_args%%);@
}


#elif defined(GDO_ENABLE_AUTOLOAD)


namespace gdo
{
    namespace helper
    {
        static auto al = gdo::dl(GDO_DEFAULT_LIBA);

        /* used internally by wrapper functions, symbol is never NULL */
        static void quick_load(int symbol_num, const char *symbol)
        {
            if (!al.load()) {
                std::string msg = "error loading library `" GDO_DEFAULT_LIBA "':\n" + al.error();
                error_exit(msg.c_str());
            }

#ifdef GDO_DELAYLOAD
            bool loaded = al.load_symbol(symbol_num);
#else
            bool loaded = al.load_all_symbols();
            UNUSED_VAL_(symbol_num);
#endif

            if (!loaded) {
                std::string msg = "error in auto-loading wrapper function `gdo::autoload::";
                msg += symbol + ("': " + al.error());
                error_exit(msg.c_str());
            }
        }
    }
}


/* autoload function wrappers */
@
GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    gdo::helper::quick_load(gdo::dl::LOAD_%%func_symbol%%, "%%func_symbol%%");@
    %%return%% gdo::dl::m_ptr_%%func_symbol%%(%%notype_args%%);@
}

#endif //GDO_ENABLE_AUTOLOAD
%PARAM_SKIP_END%
