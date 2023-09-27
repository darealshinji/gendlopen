Gendlopen is a small tool intended to help with the creation of code that
dynamically loads shared libraries.
It takes text files with C prototype declarations as input and creates C or C++
header files as output.

This is still experimental and the main goal is to simply try out how much can
be automized and find out how useful it is.

Features:
 * can generate code for C and C++
 * win32 API (LoadLibraryEx()) and POSIX (dlopen())
 * wide characters on win32 API
 * option to automatically load library and symbols

Limitations:
 * it's not a code analysis tool, so make sure the input is correct
 * function pointers must be typedef'd
 * parameter names must be included
 * loading symbols of variables is not supported
 * C++11 is the minimum requirement for the generated C++ files
 * C++23 is the minimum requirement to compile the tool


Input format
------------

Here's how the input text format must be:

 * all functions that should be loaded must be listed as modern C-style prototypes, ending on semi-colon (;)
 * function pointers MUST be typedef'd; the typedef can optionally be part of the input file
 * parameter names must be included
 * comments are ignored
 * line-breaks are treated like spaces
 * any other code will throw an error


Example
-------

Let's assume you want to load `int foo(foo_t *f)` and `void bar(bar_t b)` from `foo.so`.
First create a text file with the prototypes, each function prototype ending on a semicolon:

``` C
int foo(foo_t *f);
void bar(bar_t b);
```

Create a header file `load_foo.h` from the input:
`gendlopen --input=foo.txt --output=load_foo.h`

Include it in your source file and use the provided functions to load the symbols:
``` C
    /* load library and symbols */
    if (!gdo_load_lib_args("foo.so", GDO_DEFAULT_FLAGS, true) ||
        !gdo_load_symbols())
    {
        fprintf(stderr, "%s\n", gdo_last_error());
        gdo_free_lib();
        return 1;
    }

    /* your code */
    foo(x);
    bar(y);

    /* free library */
    gdo_free_lib();
```

Or in C++ using the `gdo::dl` class:
``` C++
    /* load library; resources are freed by the class d'tor */
    gdo::dl loader("foo.so");

    if (!loader.load() || !loader.load_symbols()) {
        std::cerr << loader.error() << std::endl;
        return 1;
    }

    /* your code */
    foo(x);
    bar(y);
```

You can find more information in the files from the `examples` directory or
in the files `template_c.h` and `template_cxx.hpp` as well as the generated
output files.


Cross-compiling
---------------

You can compile for Windows using MinGW/GCC:
``` sh
mkdir build
cd build
cmake .. -DCMAKE_C_COMPILER=x86_64-w64-mingw32-gcc \
  -DCMAKE_CXX_COMPILER=x86_64-w64-mingw32-g++ \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_CROSSCOMPILING=ON \
  -DCMAKE_BUILD_TYPE=Release
```

Or you can use clang-cl:
``` sh
mkdir build
cd build
cmake .. \
  -DCMAKE_C_COMPILER=clang-cl \
  -DCMAKE_CXX_COMPILER=clang-cl \
  -DCMAKE_SYSTEM_NAME=Windows \
  -DCMAKE_CROSSCOMPILING=ON \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_EXE_LINKER_FLAGS="/manifest:no"
```

The `/manifest:no` option is only needed if CMake's configure phase will otherwise
fail with error messages related to `mt` or `llvm-mt`.

