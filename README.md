Gendlopen is a small tool intended to help with the creation of code that
dynamically loads shared libraries.
It takes text files with C prototype declarations as input and creates C or C++
header files as output.
It's designed to add dynamic loading support to existing code with minimal effort.

Features:
 * can generate code for C and C++
 * win32 API `LoadLibraryEx()` and POSIX `dlopen()`
 * wide and narrow characters on win32 API
 * option to automatically load library and symbols

Limitations:
 * auto-loading only works on functions
 * auto-loading does not work on functions with variable arguments
 * any type declaration more complicated than a function pointer will not be recognized


Input format
------------

Here's how the input text format must be:

 * all symbols that should be loaded must be listed as C-style prototypes, ending on semi-colon (;)
 * comments, preprocessor lines, structs, unions, enums and typedefs are ignored
 * line-breaks are treated as spaces
 * you can set some options on a line beginning with `%option`,
 for example `%option format=c++ prefix=mydl library=libfoo.so`

You can create such a file with GCC:
`echo '#include "foobar.h"' | gcc -xc -c - -o /dev/null -aux-info aux.txt`

Alternatively the input text can be a Clang AST file:
`clang -Xclang -ast-dump foobar.h > foo.txt`

It's recommended to use the options `-S` or `-P` if you want to parse a Clang AST file or
GCC generated prototype list.


Example
-------

Let's assume you want to load `int foobar_foo(foo_t *f)` and `void foobar_bar(bar_t b)` from `foo.so`.
First create a text file with the prototypes, each function prototype ending on a semicolon:

``` C
int foobar_foo(foo_t *f);
void foobar_bar(bar_t b);
```

Create a header file `load_foo.h` from the input:
`gendlopen foo.txt -o load_foo.h`

Include `load_foo.h` it in your source file and use the provided functions to load the symbols:
``` C
    /* load library and symbols */
    if (!gdo_load_lib_name("foo.so") || !gdo_load_symbols())
    {
        fprintf(stderr, "%s\n", gdo_last_error());
        gdo_free_lib();
        return 1;
    }

    /* your code */
    foobar_foo(x);
    foobar_bar(y);

    /* free library */
    gdo_free_lib();
```

Or in C++ using the `gdo::dl` class:
`gendlopen foo.txt -format=c++ -o load_foo.hpp`

``` C++
    /* load library; resources are freed by the class d'tor */
    gdo::dl loader("foo.so");

    if (!loader.load_lib_and_symbols()) {
        std::cerr << loader.error() << std::endl;
        return 1;
    }

    /* your code */
    foobar_foo(x);
    foobar_bar(y);
```

You can find more information in the files from the `examples` and `tests`
directories, in the comments of the template files in `src/templates` as well as
in the generated output files.


Compiling
---------

Compile using Meson build system:
``` sh
meson setup -Denable_tests=true -Denable_examples=true build
meson compile -C build
meson test -C build
```

Or use the provided Makefiles:
`make -f Makefile.posix.mk` or for MSVC `nmake -f Makefile.msvc.mk`

This project is regularly tested on Linux (Ubuntu), OpenBSD, Haiku and Windows 11 (MSVC).
It's also tested with the following cross-toolchains on Ubuntu:
 * x86_64-w64-mingw32
 * x86_64-windows-msvc-clang (clang-cl)
 * x86_64-apple-darwin24
