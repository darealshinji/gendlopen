#include <iostream>
#include "load_plugins.h"


int main()
{
    const size_t num = 4;

    const char *files[num] = {
        "plugin_a" LIBEXT,
        "plugin_b" LIBEXT,
        "plugin_does_not_exist" LIBEXT,
        "plugin_c" LIBEXT
    };

    /* load plugins */
    gdo_plugin_t *plug = gdo_load_plugins(files, num);

    if (!plug) {
        std::cerr << "error: gdo_load_plugins() returned NULL" << std::endl;
        return 1;
    }

    std::cout << "number of plugins: " << plug->num << std::endl;

    /* check each entry and call `plugin_main()' if possible */
    for (size_t i = 0; i < plug->num; i++) {
        auto plugin_main = plug->list[i].ptr.plugin_main;

        if (plugin_main) {
            plugin_main(); /* prints a message */
        } else {
            std::cerr << "error: " << plug->list[i].filename << ":"
                " `plugin_main' not loaded" << std::endl;
        }
    }

    /* release plugins */
    gdo_release_plugins(plug);

    return 0;
}