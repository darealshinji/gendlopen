/* whether to use WinAPI */
#if defined(_WIN32) && !defined(GDO_USE_DLOPEN) && !defined(__CYGWIN__) && !defined(__MSYS__)
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
        %%type%% (*%%func_symbol%%)(%%args%%) = nullptr;
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
        /* cast to void* to avoid warnings such as [-Wcast-function-type] */
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
# if defined(__FreeBSD__) || defined(__DragonFly__)
    inline dlfunc_t get_symbol(void *handle, const char *symbol) {
        return ::dlfunc(handle, symbol);
    }
# else
    inline void *get_symbol(void *handle, const char *symbol) {
        return ::dlsym(handle, symbol);
    }
# endif

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
            reinterpret_cast<%%sym_type%%>(@
                get_symbol(handle, "%%symbol%%"));@
        if (!ptr::%%symbol%%) {@
            free_lib(handle);@
            throw SymbolError("%%symbol%%");@
        }
    }

}; /* end namespace */


#if !defined(GDO_NOALIAS)

/* aliases to raw symbol pointers */
#define %%func_symbol%% gdo::ptr::%%func_symbol%%
#define %%obj_symbol%% *gdo::ptr::%%obj_symbol%%

#endif // !GDO_NOALIAS
