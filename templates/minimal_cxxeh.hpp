/* whether to use WinAPI */
#if defined(_WIN32) && !defined(GDO_USE_DLOPEN)
    #define GDO_WINAPI
#endif

#include <stdexcept>
#include <string>
#ifdef GDO_WINAPI
    #include <windows.h>
#else
    #include <dlfcn.h>
#endif

#ifdef GDO_STATIC
    #define GDO_LINKAGE  static
#else
    #define GDO_LINKAGE  /**/
#endif



namespace gdo
{
    /* Our library and symbols handle */
    typedef struct handle
    {
#ifdef GDO_WINAPI
        HMODULE handle;
#else
        void *handle;
#endif

        GDO_TYPE (*GDO_SYMBOL_ptr_)(GDO_ARGS);
        GDO_OBJ_TYPE *GDO_OBJ_SYMBOL_ptr_;

    } handle_t;

    GDO_LINKAGE handle_t handle =
    {
        .handle = nullptr,
        .GDO_SYMBOL_ptr_ = nullptr,
        .GDO_OBJ_SYMBOL_ptr_ = nullptr
    };


#ifdef GDO_WINAPI

    GDO_LINKAGE inline
    HMODULE load_lib(const char *filename, int flags=0) {
        return ::LoadLibraryExA(filename, nullptr, flags);
    }

    GDO_LINKAGE inline
    bool free_lib(HMODULE handle) {
        return (::FreeLibrary(handle) == TRUE);
    }

    GDO_LINKAGE inline
    void *get_symbol(HMODULE handle, const char *symbol) {
        return reinterpret_cast<void *>(::GetProcAddress(handle, symbol));
    }

#else /* dlfcn */

    GDO_LINKAGE inline
    void *load_lib(const char *filename, int flags=RTLD_LAZY) {
        return ::dlopen(filename, flags);
    }

    GDO_LINKAGE inline
    bool free_lib(void *handle) {
        return (::dlclose(handle) == 0);
    }

    GDO_LINKAGE inline
    void *get_symbol(void *handle, const char *symbol) {
        return ::dlsym(handle, symbol);
    }

#endif //GDO_WINAPI


    class Error : public std::runtime_error
    {
        public:
            Error(const std::string &message) : std::runtime_error(message) {}
            virtual ~Error() {}
    };

    class LibraryError : public Error
    {
        public:
            LibraryError(const std::string &message) : Error(message) {}
            virtual ~LibraryError() {}
    };

    class SymbolError : public Error
    {
        public:
            SymbolError(const std::string &message) : Error(message) {}
            virtual ~SymbolError() {}
    };


    /* throw an exception on error */
    void load_library_and_symbols(const char *filename) noexcept(false)
    {
        handle.handle = load_lib(filename);

        if (!handle.handle) {
            throw LibraryError(filename ? filename : "<NULL>");
        }
    @
        /* GDO_SYMBOL */@
        handle.GDO_SYMBOL_ptr_ =@
            reinterpret_cast<GDO_TYPE (*)(GDO_ARGS)>(@
                get_symbol(handle.handle, "GDO_SYMBOL"));@
        if (!handle.GDO_SYMBOL_ptr_) {@
            free_lib(handle.handle);@
            throw SymbolError("GDO_SYMBOL");@
        }
    @
        /* GDO_OBJ_SYMBOL */@
        handle.GDO_OBJ_SYMBOL_ptr_ =
            reinterpret_cast<GDO_OBJ_TYPE *>(@
                get_symbol(handle.handle, "GDO_OBJ_SYMBOL"));@
        if (!handle.GDO_OBJ_SYMBOL_ptr_) {@
            free_lib(handle.handle);@
            throw SymbolError("GDO_OBJ_SYMBOL");@
        }
    }

}; /* end namespace */


/* aliases to raw function pointers */
#define GDO_SYMBOL gdo::handle.GDO_SYMBOL_ptr_

/* aliases to raw object pointers */
#define GDO_OBJ_SYMBOL *gdo::handle.GDO_OBJ_SYMBOL_ptr_

