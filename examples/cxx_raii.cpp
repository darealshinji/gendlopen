#include <iostream>
#include <SDL.h>
#include "example_cxx_sdl.h"


/* simple OOP/RAII example */
class sdl_loader
{
public:

    /* empty c'tor */
    sdl_loader() {}

    /* d'tor */
    ~sdl_loader() { unload();
                    std::cerr << "library freed in class d'tor" << std::endl; }

    /* load library and symbols */
    bool load(const char *libname) { return gdo_lib_is_loaded() ||
                                            gdo_load_lib_name_and_symbols(libname); }

    /* whether library and symbols were successfully loaded */
    bool loaded() { return gdo_lib_is_loaded() &&
                           gdo_all_symbols_loaded(); }

    /* free library */
    void unload() { gdo_force_free_lib(); }

    /* get last error message */
    const char *errmsg() { return gdo_last_error(); }

    /* get the loaded library path */
    const char *library_path() { const char *lib = gdo_library_path();
                                 return (lib && *lib) ? lib : "<EMPTY>"; }
};


int main()
{
    sdl_loader ld;

    if (!ld.load(GDO_LIBNAME(SDL3, 0))) {
        std::cerr << ld.errmsg() << std::endl;
        ld.unload();
    }

    if (!ld.loaded() && !ld.load(GDO_LIBNAME(SDL2-2.0, 0))) {
        std::cerr << ld.errmsg() << std::endl;
        ld.unload();
    }

    if (ld.loaded()) {
        std::string msg = "Library loaded:\n";
        msg += ld.library_path();
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "SDL Info", msg.c_str(), NULL);
    }

    return 0;
}
