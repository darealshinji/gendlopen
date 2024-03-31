gdo::message_callback_t gdo::dl::m_message_callback = nullptr;
gdo::dl::handle_t gdo::dl::m_handle = nullptr;

gdo::dl::fptr_%%symbol%%_t gdo::dl::m_ptr_%%symbol%% = nullptr;
%%obj_type%% *gdo::dl::m_ptr_%%obj_symbol%% = nullptr;

bool gdo::dl::m_loaded_%%symbol%% = false;
bool gdo::dl::m_loaded_%%obj_symbol%% = false;


%SKIP_BEGIN%
#if defined(GDO_WRAP_FUNCTIONS) && !defined(GDO_ENABLE_AUTOLOAD)

namespace /* anonymous */
{
    void error_exit(const char *msg)
    {
        auto cb = gdo::dl::message_callback();

        if (cb) {
            cb(msg);
        } else {
            std::cerr << msg << std::endl;
        }

        std::exit(1);
    }
} /* anonymous namespace */

/* function wrappers */
@
GDO_VISIBILITY %%type%% %%symbol%%(%%args%%) {@
    if (!gdo::dl::m_loaded_%%symbol%%) {@
        error_exit( "error: symbol `%%symbol%%' was not loaded");@
    }@
    %%return%% gdo::dl::m_ptr_%%symbol%%(%%notype_args%%);@
}

#elif defined(GDO_ENABLE_AUTOLOAD)

namespace /* anonymous */
{
    auto al = gdo::dl(GDO_DEFAULT_LIB);

    void error_exit(const char *s1, const char *s2, const char *s3, const std::string &s4)
    {
        auto cb = gdo::dl::message_callback();

        if (cb) {
            std::string msg = s1 + (s2 + (s3 + s4));
            cb(msg.c_str());
        } else {
            std::cerr << s1 << s2 << s3 << s4 << std::endl;
        }

        std::exit(1);
    }

    /* used internally by wrapper functions, symbol is never NULL */
    void quick_load(const char *symbol)
    {
        if (!al.load()) {
            error_exit("error loading library `", GDO_DEFAULT_LIB, "':\n", al.error());
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

/* autoload function wrappers */
@
GDO_VISIBILITY %%type%% %%symbol%%(%%args%%) {@
    quick_load("%%symbol%%");@
    %%return%% gdo::dl::m_ptr_%%symbol%%(%%notype_args%%);@
}

#endif //GDO_ENABLE_AUTOLOAD
%SKIP_END%
