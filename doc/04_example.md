# Example

A step-by-step example using the cross-platform library SDL.

Let's start with this simple C code:

``` C
#include <SDL.h>

int main()
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Test", "Hello", NULL);
    return 0;
}
```

This will show up a window with a message. Save it as **test.c** and compile:
``` sh
cc $(pkg-config --cflags sdl2) -o sdl_test test.c $(pkg-config --libs sdl2)
```

Inspecting the compiled binary file should reveal that it's linked against SDL2:
```
$ LANG=C readelf -d sdl_test | grep NEEDED
 0x0000000000000001 (NEEDED)             Shared library: [libSDL2-2.0.so.0]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
```

Let's now try to dynamically load it instead and use a fallback solution if loading it wasn't successful.
We _want_ something like this:
``` C
    if (LOAD_LIBRARY_AND_SYMBOLS()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Test", "Hello", NULL);
    } else {
        puts("Hello"); //fallback solution
    }
```


## Prototype list

First step is to create a list of prototype declarations of all symbols we want to load.
In this case it's only `SDL_ShowSimpleMessageBox`:
``` C
extern int SDL_ShowSimpleMessageBox(Uint32 flags, const char *title, const char *message, SDL_Window *window);
```
Save it as **sdl.txt** and generate a header file **sdl_dynload.h**: `gendlopen sdl.txt -o sdl_dynload.h`


## gendlopen C API

Let's include the header into our source.
The generated header file needs to be included _after_ the SDL header because it uses typedefs from there:
``` C
#include <SDL.h>
#include "sdl_dynload.h"
```

Several functions are provided to load the library and symbols.
The functions usually return a boolean value (`<stdbool.h>`).
Let's use `gdo_load_lib_name()` to load libSDL and `gdo_load_all_symbols()` to load the function pointer(s):
``` C
    if (gdo_load_lib_name("libSDL2-2.0.so.0") && gdo_load_all_symbols()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Test", "Hello", NULL);
    } else {
        puts("Hello"); //fallback solution
    }
```

We can also print a useful error message using `gdo_last_error()`:
``` C
    if (gdo_load_lib_name("libSDL2-2.0.so.0") && gdo_load_all_symbols()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Test", "Hello", NULL);
    } else {
        fprintf(stderr, "%s\n", gdo_last_error());
        puts("Hello"); //fallback solution
    }
```

Don't forget to free the library at the end with `gdo_free_lib()`.
We can also use the macro `LIBNAME()` to provide the correct filename for each platform.
Here's our updated source code:
``` C
#include <SDL.h>
#include "sdl_dynload.h"

int main()
{
    if (gdo_load_lib_name(LIBNAME(SDL2-2.0,0)) && gdo_load_all_symbols()) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "Test", "Hello", NULL);
    } else {
        fprintf(stderr, "%s\n", gdo_last_error());
        puts("Hello"); //fallback solution
    }

    gdo_free_lib(); //can always be called safely
    return 0;
}
```

Compile again but this time don't link against libSDL. You may however need to explicitly link against libdl on some targets.
``` sh
cc $(pkg-config --cflags sdl2) -o sdl_test test.c -ldl
```

The program should work the same as before but now it's no longer linked against libSDL:
```
$ LANG=C readelf -d sdl_test | grep NEEDED
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
```

To test the fallback solution you can change the library name with something invalid:
```
$ ./sdl_test 
invalid.so: cannot open shared object file: No such file or directory
Hello
```
As expected our program prints out a diagnostic error message and falls back to printing the message to stdout.
