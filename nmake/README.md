important main targets: `all check clean clean-check run_tests`

enable verbosity: `nmake V=1`

The Makefile will check for `clang` as an optional dependency,
to check if reading input from a Clang AST works.
You can specify a different binary: `nmake CLANG=clang-16.exe`
