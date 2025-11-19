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
    /* symbol pointers; symbol names must be prefixed to avoid macro expansion */
    %%type%% (*GDO_PTR_%%func_symbol%%)(%%args%%) = nullptr;
    %%obj_type%% *GDO_PTR_%%obj_symbol%% = nullptr;


    /**
     * Library handle
     */
#ifdef GDO_WINAPI
    HMODULE handle = nullptr;
#else
    void *handle = nullptr;
#endif


    /**
     * Free library handle without error checks.
     * Internal handle and pointers are always set back to NULL.
     */
    void free_library(void)
    {
        if (handle) {
#ifdef GDO_WINAPI
            ::FreeLibrary(handle);
#else
            ::dlclose(handle);
#endif
        }

        handle = nullptr;
        GDO_PTR_%%symbol%% = nullptr;
    }


    /**
     * Base error class
     */
    class Error : public std::runtime_error
    {
        public:
            Error(const std::string &message) : std::runtime_error(message) {}
            virtual ~Error() {}
    };


    /**
     * An error has occurred trying to load a library.
     */
    class LibraryError : public Error
    {
        public:
            LibraryError(const std::string &message) : Error(message) {}
            virtual ~LibraryError() {}
    };


    /**
     * An error has occurred trying to load a symbol.
     */
    class SymbolError : public Error
    {
        public:
            SymbolError(const std::string &message) : Error(message) {}
            virtual ~SymbolError() {}
    };


    /**
     * Load library and all symbols.
     *
     * filename:
     *   Library filename or path to load. Must not be empty or NULL.
     *
     * If an error occurs the library handle is freed and an exception is thrown.
     */
    void load_library_and_symbols(const char *filename) noexcept(false)
    {
        if (filename == NULL) {
            throw LibraryError("<NULL>");
        } else if (*filename == 0) {
            throw LibraryError("<EMPTY>");
        }

        /* load library */
#ifdef GDO_WINAPI
        handle = ::LoadLibraryA(filename);
#elif defined(_AIX)
        handle = ::dlopen(filename, RTLD_LAZY | RTLD_MEMBER);
#else
        handle = ::dlopen(filename, RTLD_LAZY);
#endif

        if (!handle) {
            throw LibraryError(filename);
        }

        /* load symbols */
        auto load_symbol = [] (const char *symbol) -> void*
        {
#ifdef GDO_WINAPI
            return reinterpret_cast<void *>(::GetProcAddress(handle, symbol));
#else
            return ::dlsym(handle, symbol);
#endif
        };
@
        /* %%symbol%% */@
        GDO_PTR_%%symbol%% =@
            reinterpret_cast<%%sym_type%%>(@
                load_symbol("%%symbol%%"));@
        if (!GDO_PTR_%%symbol%%) {@
            free_library();@
            throw SymbolError("%%symbol%%");@
        }
    }

}; /* end namespace */


#if !defined(GDO_DISABLE_ALIASING)

/**
 * Aliases to raw symbol pointers
 */
#define %%func_symbol_pad%%  gdo::GDO_PTR_%%func_symbol%%
#define %%obj_symbol_pad%%  *gdo::GDO_PTR_%%obj_symbol%%

#endif // !GDO_DISABLE_ALIASING

