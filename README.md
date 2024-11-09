Gendlopen is a small tool intended to help with the creation of code that
dynamically loads shared libraries.
It takes text files with C prototype declarations as input and creates C or C++
header files as output.

This is still experimental and the main goal is to simply try out how much can
be automized and find out how useful it is.

Features:
 * no build dependencies other than a C/C++ compiler and make
 * can generate code for C and C++
 * win32 API `LoadLibraryEx()` and POSIX `dlopen()`
 * wide and narrow characters on win32 API
 * option to automatically load library and symbols

Limitations:
 * it's not a code analysis tool, so make sure the input is correct and simple ¹
 * auto-loading only works on functions ²
 * auto-loading does not work on functions with variable arguments ³
 * C++11 is the minimum requirement for the generated C++ files
 * C++20 is the minimum requirement to compile the tool

¹ macros are not expanded, extra parentheses i.e. around function names cannot be handled<br>
² I recommend using your own get/set wrapper functions to auto-load objects<br>
³ You can however replace a prototype such as `int myprintf(const char *format, ...)`
  with one that has a fixed number of arguments: `int myprintf(const char *format, int arg1, const char *arg2, float arg3)`


Input format
------------

Here's how the input text format must be:

 * all symbols that should be loaded must be listed as C-style prototypes, ending on semi-colon (;)
 * old K&R syntax is not allowed
 * comments are ignored
 * line-breaks are treated as spaces
 * any other code will throw an error
 * some options can be set on a line beginning with `%option`,
 for example `%option format=c++ name=mydl library=libfoo.so`

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
    if (!gdo_load_lib_name("foo.so") || !gdo_load_symbols(false))
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

You can find more information in the files from the `examples` and `test`
directories, in the comments of the template files in `src/templates` as well as
in the generated output files.


Compiling
---------

`./configure && make && make test`

On Windows (MSVC) you can run `nmake` inside the `nmake` directory.

Cross-compiling with MinGW and clang-cl:
``` sh
./configure host=x86_64-w64-mingw32 && make
./configure cl=clang-cl && make
```

Cross-compiling tests:
```sh
# compile the native tool first
./configure && make
cp src/gendlopen xgendlopen

# MinGW
./configure gdo="$PWD/xgendlopen" host=x86_64-w64-mingw32
make clean && make check

# clang-cl with lld-link
./configure gdo="$PWD/xgendlopen" cl=clang-cl ld=lld-link
make clean && make check
```


Links
-----

A similar albeit less portable approach is the project [dynload-wrapper](https://github.com/hpvb/dynload-wrapper).

If you want to use `dlopen()` on Windows try [dlfcn-win32](https://github.com/dlfcn-win32/dlfcn-win32).
