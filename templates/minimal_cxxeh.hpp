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


/***

A small C++ library loader that throws exceptions on error.

Usage:

    try {
        gdo::load_library_and_symbols( LIBNAME(helloworld,0) );
    }
    catch (const gdo::LibraryError &e) {
        std::cerr << "error: failed to load library: " << e.what() << std::endl;
        return 1;
    }
    catch (const gdo::SymbolError &e) {
        std::cerr << "error: failed to load symbol: " << e.what() << std::endl;
        return 1;
    }
    catch (...) {
        std::cerr << "an unknown error has occurred" << std::endl;
        return 1;
    }

***/

namespace gdo
{
    /* symbol pointers */
    namespace ptr
    {
        %%type%% (*%%symbol%%)(%%args%%) = nullptr;
        %%obj_type%% *%%obj_symbol%% = nullptr;
    }

#ifdef GDO_WINAPI

    /* library handle */
    HMODULE handle = nullptr;

    /* load library */
    inline HMODULE load_lib(const char *filename, int flags=0) {
        return ::LoadLibraryExA(filename, nullptr, flags);
    }

    /* free library */
    inline bool free_lib(HMODULE handle) {
        return (::FreeLibrary(handle) == TRUE);
    }

    /* get symbol */
    inline void *get_symbol(HMODULE handle, const char *symbol) {
        return reinterpret_cast<void *>(::GetProcAddress(handle, symbol));
    }

#else /* dlfcn */

    /* library handle */
    void *handle = nullptr;

    /* load library */
    inline void *load_lib(const char *filename, int flags=RTLD_LAZY) {
        return ::dlopen(filename, flags);
    }

    /* free library */
    inline bool free_lib(void *handle) {
        return (::dlclose(handle) == 0);
    }

    /* get symbol */
    inline void *get_symbol(void *handle, const char *symbol) {
        return ::dlsym(handle, symbol);
    }

#endif //GDO_WINAPI


    /* base error class */
    class Error : public std::runtime_error
    {
        public:
            Error(const std::string &message) : std::runtime_error(message) {}
            virtual ~Error() {}
    };

    /* library loading error */
    class LibraryError : public Error
    {
        public:
            LibraryError(const std::string &message) : Error(message) {}
            virtual ~LibraryError() {}
    };

    /* symbol loading error */
    class SymbolError : public Error
    {
        public:
            SymbolError(const std::string &message) : Error(message) {}
            virtual ~SymbolError() {}
    };


    /* throw an exception on error */
    void load_library_and_symbols(const char *filename) noexcept(false)
    {
        handle = load_lib(filename);

        if (!handle) {
            if (filename == NULL) {
                filename = "<NULL>";
            } else if (*filename == 0) {
                filename = "<EMPTY>";
            }

            throw LibraryError(filename);
        }
    @
        /* %%symbol%% */@
        ptr::%%symbol%% =@
            reinterpret_cast<%%type%% (*)(%%args%%)>(@
                get_symbol(handle, "%%symbol%%"));@
        if (!ptr::%%symbol%%) {@
            free_lib(handle);@
            throw SymbolError("%%symbol%%");@
        }
    @
        /* %%obj_symbol%% */@
        ptr::%%obj_symbol%% =@
            reinterpret_cast<%%obj_type%% *>(@
                get_symbol(handle, "%%obj_symbol%%"));@
        if (!ptr::%%obj_symbol%%) {@
            free_lib(handle);@
            throw SymbolError("%%obj_symbol%%");@
        }
    }

}; /* end namespace */


#if !defined(GDO_NOALIAS)

/* aliases to raw function pointers */
#define %%symbol%% gdo::ptr::%%symbol%%

/* aliases to raw object pointers */
#define %%obj_symbol%% *gdo::ptr::%%obj_symbol%%

#endif // !GDO_NOALIAS
