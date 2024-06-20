#ifdef GDO_HAS_MSG_CB
gdo::dl::message_callback_t gdo::dl::m_message_callback = nullptr;
#endif
gdo::dl::handle_t gdo::dl::m_handle = nullptr;

%%type%% (*gdo::dl::m_ptr_%%func_symbol%%)(%%args%%) = nullptr;
%%obj_type%% *gdo::dl::m_ptr_%%obj_symbol%% = nullptr;



%SKIP_PARAM_UNUSED_BEGIN%
//%DNL%//  comment out this whole section if "--skip-param" was set
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
GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    if (!gdo::dl::m_ptr_%%func_symbol%%) {@
        error_exit("error: symbol `%%func_symbol%%' was not loaded");@
    }@
    %%return%% gdo::dl::m_ptr_%%func_symbol%%(%%notype_args%%);@
}


#elif defined(GDO_ENABLE_AUTOLOAD)


namespace /* anonymous */
{
    auto al = gdo::dl(GDO_DEFAULT_LIBA);

    void error_exit4(const char *s1, const char *s2, const char *s3, const std::string &s4)
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
            error_exit4("error loading library `", GDO_DEFAULT_LIBA, "':\n", al.error());
        }

#ifdef GDO_DELAYLOAD
        if (!al.load_symbol(symbol))
#else
        if (!al.load_symbols())
#endif
        {
            error_exit4("error in auto-loading wrapper function "
                "`gdo::autoload::", symbol, "': ", al.error());
        }
    }
} /* anonymous namespace */


/* autoload function wrappers */
@
GDO_VISIBILITY %%type%% %%func_symbol%%(%%args%%) {@
    quick_load("%%func_symbol%%");@
    %%return%% gdo::dl::m_ptr_%%func_symbol%%(%%notype_args%%);@
}

#endif //GDO_ENABLE_AUTOLOAD
%SKIP_PARAM_UNUSED_END%
