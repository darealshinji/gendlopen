#include <stdexcept>
#include <string>

#if defined(_WIN32) && !defined(GDO_USE_DLOPEN)
# define GDO_WINAPI
# include <windows.h>
#else
# include <dlfcn.h>
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
    const int default_flags = 0;
#elif defined(_AIX)
    const int default_flags = RTLD_LAZY | RTLD_MEMBER;
#else
    const int default_flags = RTLD_LAZY;
#endif


    /* library handle */
    void *handle = nullptr;


    /* load library */
    void load_lib(const char *filename, int flags=default_flags)
    {
#ifdef GDO_WINAPI
        handle = reinterpret_cast<void *>(::LoadLibraryExA(filename, nullptr, flags));
#else
        handle = ::dlopen(filename, flags);
#endif
    }


    /* free library (no error checks) */
    void free_lib(void)
    {
#ifdef GDO_WINAPI
        ::FreeLibrary(reinterpret_cast<HMODULE>(handle));
#else
        ::dlclose(handle);
#endif

        handle = nullptr;
        ptr::%%symbol%% = nullptr;
    }


    /* get symbol */
    void *get_symbol(const char *symbol)
    {
#ifdef GDO_WINAPI
        return reinterpret_cast<void *>(::GetProcAddress(
            reinterpret_cast<HMODULE>(handle), symbol));
#else
        return ::dlsym(handle, symbol);
#endif
    }


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
        load_lib(filename);

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
                get_symbol("%%symbol%%"));@
        if (!ptr::%%symbol%%) {@
            free_lib();@
            throw SymbolError("%%symbol%%");@
        }
    }

}; /* end namespace */


#if !defined(GDO_NOALIAS)

/* aliases to raw symbol pointers */
#define %%func_symbol%%  gdo::ptr::%%func_symbol%%
#define %%obj_symbol%%  *gdo::ptr::%%obj_symbol%%

#endif // !GDO_NOALIAS
