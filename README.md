# About

Gendlopen is a small tool intended to help with the creation of code that
dynamically loads shared libraries. It takes text files with C prototype
declarations as input and creates C or C++ header files as output.
It's designed to add dynamic loading support to existing code with minimal effort.

Features:
 * can generate code for C and C++
 * win32 API `LoadLibraryEx()` and POSIX `dlopen()`
 * wide and narrow characters on win32 API
 * option to automatically load library and symbols

Limitations:
 * auto-loading only works on functions
 * auto-loading functions with variable arguments require GNU builtins (C only) and inlining
 * any type declaration more complicated than a function pointer will not be recognized


# Motivation

Writing macros and formatted prototype lists to be used with `dlopen()` and
`dlsym()` is annoying, so I wanted to automate this process as much as possible.
I could have written a script but I wanted the tool to be able to be used in
Windows too without the need to install a script interpreter first.
I also didn't want to just write a header-only library relying on X-macros, that
would still require formatting lists by hand. And X-macros are complicated and ugly.
So in the end I wrote my own small macro tool.


# Compiling

You can use the provided Makefiles or the Meson build system to build gendlopen.
No thirdparty build dependencies are required.

Makefile
--------

Use the Makefile with `make -f Makefile.posix.mk` or to build with MSVC
run `nmake -f Makefile.msvc.mk` from a Visual Studio Developer Command Prompt.

Meson
-----

To compile with tests and examples enabled:
``` sh
meson setup -Denable_tests=true -Denable_examples=true build
meson compile -C build
```

Run the tests with `meson test -C build`.

Cross-compiling
---------------

You can find configurations for cross-compiling in the directory `cross-files`:
``` sh
meson setup --cross-file cross-files/{cross-toolchain}.ini build
meson compile -C build
```

Small wrapper scripts are provided to make cross-compiling with clang-cl easier.


# Input format

gendlopen reads C prototype declarations from text files or STDIN.

Here's how the input text format must be:
 * all symbols that should be loaded must be listed as C-style prototypes, ending on semi-colon (;)
 * comments, preprocessor lines, structs, unions, enums and typedefs are ignored
 * line-breaks are treated as spaces
 * you can set some options on a line beginning with `%option`

This means you can normally copy the declarations verbatim from header files or other documentation.


Get list of symbols
-------------------

If you're unsure which symbols of a library you need to dynamically load you can for example
try to link your binary without the library and take a look at the error messages.

Or you can use tools such as `nm`.
For example to list all symbols prefixed with `gtk_` from a binary linked against libgtk
run `nm --dynamic myexecutable | grep gtk_`.

On Windows you can use `dumpbin` from Visual Studio Developer Command Prompt:
`dumpbin /imports myexecutable.exe | findstr gtk_`


Copying these declarations verbatim from i.e. online documentation into a text file:
``` C
guint
gtk_get_major_version (
  void
);

guint
gtk_get_micro_version (
  void
);

guint
gtk_get_minor_version (
  void
);

void
gtk_init (
  int* argc,
  char*** argv
);
```

Since comments, preprocessor lines, structs, etc. are ignored this would be fine too:
``` C
#include <xyz.h> // ignored!

/* ignored */
typedef struct _mystruct {
  struct nested_struct {
    int a;
    int b;
  };
  long l;
  int i;
} mystruct_t;

guint gtk_get_major_version (void);
guint gtk_get_micro_version (void);
guint gtk_get_minor_version (void);

// this conditional has no effect
#ifdef USE_GTK_INIT
void gtk_init (int* argc, char*** argv);
#endif
```

However I recommend to keep it simple.
I also prefer to use the file ending `.txt` for the input list to separate it
from regular source code, but it's not a requirement.


Options
-------

You can set some options on lines beginning with `%option` instead of passing
them through command line.

Line splitting and multiple `%option` lines are supported:
```
%option opt1
%option opt2
%option opt3 \
        opt4 \
        opt5
```

Available options:
```
format=<string>
prefix=<string>
library=[<mode>:]<lib>
include=[nq:]<file>
define=<string>
param=[skip|create|read]
no-date
no-pragma-once
line
```

For an explanation look for the corresponding command line options in `gendlopen -full-help`.


Macros
------

Since macros are not supported it's recommended to remove `DLL_EXPORT` macros
from the declarations.

Sometimes declarations come as macros, for example:
``` C
PNG_EXPORT(1, png_uint_32, png_access_version_number, (void));
PNG_EXPORT(2, void, png_set_sig_bytes, (png_structrp png_ptr, int num_bytes));
```

You can use the C preprocessor to format that list.
Save the list above in a temporary file `temp.h` and add a simple `PNG_EXPORT`
macro definition at the top:
``` C
#define PNG_EXPORT(ignored, type, symbol, param)  type symbol param
PNG_EXPORT(1, png_uint_32, png_access_version_number, (void));
PNG_EXPORT(2, void, png_set_sig_bytes, (png_structrp png_ptr, int num_bytes));
```

Use the preprocessor: `gcc -E temp.h > proto.txt` or `cl.exe -E temp.h > proto.txt`

Possible output:
``` C
# 0 "temp.h"
# 0 "<built-in>"
# 0 "<command-line>"
# 1 "/usr/include/stdc-predef.h" 1 3 4
# 0 "<command-line>" 2
# 1 "temp.h"

png_uint_32 png_access_version_number (void);
void png_set_sig_bytes (png_structrp png_ptr, int num_bytes);
```

All lines beginning with `#` are ignored, so this can be used as input.


gcc -aux-info
-------------

Another way to format the input and get a clean list of prototypes is
to use `gcc -aux-info`.

Process `png.h` and save the list as `aux.txt`:
``` sh
echo '#include <png.h>' | gcc -xc -c - -o /dev/null -aux-info aux.txt
```

The output will contain lines like these:
``` C
/* /usr/include/png.h:2545:NC */ extern png_uint_32 png_get_uint_31 (png_const_structrp restrict , png_const_bytep);
/* /usr/include/png.h:2551:NC */ extern void png_save_uint_32 (png_bytep, png_uint_32);
/* /usr/include/png.h:2554:NC */ extern void png_save_int_32 (png_bytep, png_int_32);
```

Be careful: all kind of unwanted symbols from C standard headers are likely to be included too!
And there may be multiple definitions too. Delete all lines you don't need.


Clang AST
---------

Alternatively the input can be an Abstract Syntax Tree generated by Clang:
`clang -Xclang -ast-dump /usr/include/png.h > ast.txt`

Since all kind of unwanted symbols from C standard headers are likely to be
included too, it's recommened to later use gendlopen's command line options
`-P` (symbol prefix) or `-S` (symbol name) to filter the input.
But you can also force to read all symbols with `-ast-all-symbols`.


# Process input

Assuming the following input file `input.txt`:
``` C
guint gtk_get_major_version (void);
guint gtk_get_micro_version (void);
guint gtk_get_minor_version (void);
void gtk_init (int    *argc,
               char ***argv);
```

Calling gendlopen with this file as input will generate a header file and print
it to STDOUT: `gendlopen input.txt`

Reading from STDIN works too if the input filename is a dash:
`cat input.txt | gendlopen -` or `gendlopen - < input.txt`

We can also use the option `-print-symbols` to check which symbols were found:
`gendlopen -print-symbols input.txt` should print a list of all prototypes.

Use `-param=skip` or `-param=create` if parameter names are missing from the prototypes.

You can also filter the input with `-S` or `-P`.
`-S` will pick a specific symbol and `-P` will pick all symbols that start with a specified prefix.

`-Pgtk_get_` for example will pick only symbols prefixed with `gtk_get_` and
`-Sgtk_get_major_version -Sgtk_get_minor_version` will pick only those two symbols.

You can also combine both:
`-Pgtk_get_mi -Sgtk_init` will in this case pick the following three symbols:
```
gtk_get_micro_version
gtk_get_minor_version
gtk_init
```


# Save output

Calling gendlopen with an input file will generate a header file and print
it to STDOUT: `gendlopen input.txt`

To save it into a file use `-out`: `gendlopen input.txt -out=output.h`

A couple of different output "formats" are supported right now:
* `-format=C`: C header file with many features (this is the default)
* `-format=C++`: C++ header file with many features (no exception handling)
* `-format=plugin`: C header intended to help writing a plugin loader
* `-format=minimal`: small C header
* `-format=minimal-C++`: small C++ header with exception handling

Furthermore you can save the output into separate body and header files
using the option `-separate`.
The filename extensions will be set to .c/.h or .cpp/.hpp accordingly.
This is ignored however for "minimal-C" and "minimal-C++".

You can force to overwrite an existing output file with `-force`.

To avoid conflicts all functions/macros/etc. are prefixed with `gdo_` or `GDO_`.
If you want to use more than one generated header you can change the prefix
in the output with `-prefix=<string>`.


# Use output

The output files contain information about how to use the API.
In most cases however you will only need a function to load a
library, load all symbols and to free the library.

You don't need to manually assign any function pointers, that's all
done by these helper functions. The functions you're loading are
called/used the same way in your source code as before.

Here's a list of the most important functions and methods for each output format:

``` C
//-format=C
bool gdo_load_lib_name(const gdo_char_t *filename);
bool gdo_load_all_symbols();
bool gdo_free_lib();
const gdo_char_t *gdo_last_error();
```

``` C++
//-format=C++
gdo::dl(); // empty c'tor
bool gdo::load(const std::string &filename, int flags=default_flags, bool new_namespace=false);
bool gdo::load_all_symbols();
bool gdo::free(bool force=false); // called by d'tor
std::string gdo::error();

#ifdef GDO_WINAPI
bool gdo::load(const std::wstring &filename, int flags=default_flags, bool new_namespace=false);
std::wstring gdo::error_w();
#endif
```

``` C
//-format=minimal-C
const char *gdo_load_library_and_symbols(const char *filename);
void gdo_free_library();
```

``` C++
//-format=minimal-C++
void gdo::load_library_and_symbols(const char *filename); // throws exceptions
void gdo::free_library();
```

``` C
//-format=plugin
gdo_plugin_t *gdo_load_plugins(const gdo_char_t **files, size_t num);
void gdo_release_plugins(gdo_plugin_t *plug);
```


Short examples
--------------

Theses are short examples quickly illustrating how to use the API.
For more examples you can take a look at the `tests` and `examples` files.

``` C
//-format=C
if (gdo_load_lib_name("mylib.so") && gdo_load_all_symbols()) {
    // run optional code
} else {
    fprintf(stderr, "%s\n", gdo_last_error());
}

gdo_free_lib(); // always safe to call
```

``` C++
//-format=C++
gdo::dl mydl();

if (mydl.load("mylib.so") && mydl.load_all_symbols()) {
    // run optional code
} else {
    std::cerr << mydl.error() << std::endl;
}
```

``` C
//-format=minimal-C
const char *error_message = gdo_load_library_and_symbols("mylib.so");

if (error_message) {
    fprintf(stderr, "%s\n", error_message);
} else {
    // run optional code
}

gdo_free_library();  // always safe to call
```

``` C++
//-format=minimal-C++
try {
    gdo::load_library_and_symbols("mylib.so");
    // run optional code
}
catch (const gdo::LibraryError &e) {
    std::cerr << "error: failed to load library: " << e.what() << std::endl;
}
catch (const gdo::SymbolError &e) {
    std::cerr << "error: failed to load symbol: " << e.what() << std::endl;
}
catch (...) {
    std::cerr << "an unknown error has occurred" << std::endl;
}

gdo::free_library();  // always safe to call
```


Filename macros
---------------

Macros are available to help with using filenames for different targets:

* `GDO_LIBNAME(NAME, API)` will create a default library name with API number;
  `GDO_LIBNAME(xyz,0)` for example will become `libxyz.so.0` on Linux,
  `libxyz.0.dylib` on macOS or `xyz-0.dll` on Windows (MSVC)
* `GDO_LIBEXT` is the library file extension including dot (i.e. `.dll` or `.so`)
* for compatibility with Windows there are always specific wide characters (`wchar_t`)
  and narrow characters (`char`) versions available for these macros:
  `GDO_LIBNAMEA GDO_LIBNAMEW` and `GDO_LIBEXTA GDO_LIBEXTW`


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
Save it as **sdl.txt** and generate a header file **sdl_dynload.h**: `gendlopen sdl.txt -out=sdl_dynload.h`


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

