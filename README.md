# About

Gendlopen is a small tool intended to help with the creation of code that
dynamically loads shared libraries. It takes text files with C prototype
declarations as input and creates C header files as output.
It's designed to add dynamic loading support to existing code with minimal effort.

Features:
 * can generate different types of output
 * win32 API `LoadLibraryEx()` and POSIX `dlopen()`
 * wide and narrow characters on win32 API
 * option to automatically load library and symbols

Limitations:
 * auto-loading only works on functions
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

## Makefile

Use the Makefile with `make -f Makefile.posix.mk` or to build with MSVC
run `nmake -f Makefile.msvc.mk` from a Visual Studio Developer Command Prompt.

## Meson

To compile with tests and examples enabled:
``` sh
meson setup -Denable_tests=true -Denable_examples=true build
meson compile -C build
```

Run the tests with `meson test -C build`.

## Cross-compiling

You can find configurations for cross-compiling in the directory `cross-files`:
``` sh
meson setup --cross-file cross-files/{cross-toolchain}.ini build
meson compile -C build
```

Small wrapper scripts are provided to make cross-compiling with clang-cl easier.


# How to use it

See `GUIDE.md` for a full explanation with examples.

