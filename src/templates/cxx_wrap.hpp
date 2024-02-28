
namespace gdo
{
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

        /* used internally by wrapper functions, `symbol' is never NULL */
        void symbol_error(const char *symbol)
        {
            std::string msg = "error: symbol `" + std::string(symbol)
                + "' was not loaded";
            print_error(msg);
            std::exit(1);
        }

    #ifdef GDO_ENABLE_AUTOLOAD

    #if !defined(GDO_DEFAULT_LIB)
    #error "You need to define GDO_DEFAULT_LIB if you want to make use of GDO_ENABLE_AUTOLOAD"
    #endif

        auto al = dl(GDO_DEFAULT_LIB);

        /* used internally by wrapper functions, `calling_function' is never NULL */
        void autoload(const char *calling_function)
        {
            if (!al.load()) {
                std::string msg = "error loading library `" GDO_DEFAULT_LIB "':\n"
                    + al.error();
                print_error(msg);
                std::exit(1);
            }

            if (!al.load_symbols()) {
                std::string msg = "error in auto-loading wrapper function `"
                    + std::string(calling_function) + "': " + al.error();
                print_error(msg);
                std::exit(1);
            }
        }

    #else // !GDO_ENABLE_AUTOLOAD

        /* dummy */
        void autoload(const char *) {}

    #endif // !GDO_ENABLE_AUTOLOAD

    } /* anonymous namespace */


    /* wrapped functions */
    namespace wrapped
    {
        GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
            autoload(__FUNCTION__);@
            if (!loaded::GDO_SYMBOL) symbol_error("GDO_SYMBOL");@
            GDO_RET ptr::GDO_SYMBOL(GDO_NOTYPE_ARGS);@
        }@

    } /* namespace wrapped */

} /* namespace gdo */


#ifdef GDO_WRAP_FUNCTIONS

/* function wrappers */
@
GDO_VISIBILITY GDO_TYPE GDO_SYMBOL(GDO_ARGS) {@
    GDO_RET gdo::wrapped::GDO_SYMBOL(GDO_NOTYPE_ARGS);@
}

#elif defined(GDO_ENABLE_AUTOLOAD)

/* aliases to autoload function names */
#define GDO_SYMBOL gdo::wrapped::GDO_SYMBOL

#endif