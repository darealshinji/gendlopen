#include <iostream>
#include "helloworld.h"

/* include generated header file */
#include "cxx_with_c_minimal_header.h"


void cb(const char *msg)
{
    std::cout << msg << std::endl;
}

int main()
{
    /* quick load and error check */
    const char *lib = GDO_LIBNAME(helloworld,0);
    const char *err = gdo_load_library_and_symbols(lib);

    if (err) {
        std::cerr << "error: " << lib << ": " << err << std::endl;
        return 1;
    }

    helloworld *hw = helloworld_init();
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_release(hw);

    /* release */
    gdo_free_library();

    return 0;
}
