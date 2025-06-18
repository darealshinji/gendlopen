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
