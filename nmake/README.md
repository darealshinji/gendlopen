Important main targets are: `all check clean clean-check run_tests`

To enable verbosity: `nmake V=1`

The Makefile will check for `clang` as an optional dependency,
to check if reading input from a Clang AST works.
You can specify a different binary like this: `nmake CLANG=clang-16.exe`
