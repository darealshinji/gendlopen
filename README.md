gendlopen is a tool that generates code to dynamically load functions using
the dlopen() or LoadLibrary() API from minimal input.

Let's assume you want to load `int foo(foo_t *)` and `void bar(bar_t)` from `foo.so`.
First create a text file with the prototypes, each function prototype ending on a semicolon:

``` C
int foo(foo_t *);
void bar(bar_t);
```

Create a header file `load_foo.h` from the input:
`gendlopen --input foo.txt --output load_foo.h`

Include it in your source file and use the macros from it to load the symbols:
``` C
#include <stdio.h>
#include <foo.h> // keep original headers for typedefs
#include "load_foo.h"

int main()
{
  // first argument: filename (library) to load
  // second argument: code to execute on an error
  GENDLOPEN_LOAD_ALL_FN(
    "foo.so",
    fprintf(stderr, "%s\n", GENDLOPEN_LAST_ERROR); return 1
  );

  // run your code
  foo(NULL);
  bar(0);

  // free library
  GENDLOPEN_FREE_LIB;

  return 0;
}
```

You can also generate a C++ class with
`gendlopen --input foo.txt --output load_foo.h --target cpp-static`:
``` C++
#include <iostream>
#include <foo.h> // keep headers for typedefs
#include "load_foo.h"

int main()
{
  gendlopen gen("foo.so");

  if (!gen.load() || !gen.sym()) {
    std::cerr << gen.error() << std::endl;
    return 1;
  }

  // run your code
  foo(NULL);
  bar(0);

  // library will be freed automatically
  // from the class destructor
  return 0;
}
```

Take a look at the generated header to see the available macros or class member functions.
Run `gendlopen -h` for a full list of options.

The code is generated from template files. You can use your own custom templates with the `--custom` option.
Take a look at the template files `c_header.template` and `cxx_header.template` on how to write your own.

This is a list of macros to use in the template files.

@NAME@:<br>
  name to use i.e. as macro prefixes or C++ class names

@NAMEUPPER@:<br>
  uppercase variant of @NAME@

@DEFAULT_LIB@:<br>
  default library name

@LIB_LIBEXT@:<br>
  convenience macros to handle library names

@R@:<br>
  this will be replaced with random code - useful to avoid symbol clashes

@X@:<br>
  Any line that begins or end with this macro (whitespaces are ignored) will not
  be processed at all.

@D:VALUE@<br>
@D!VALUE@:<br>
  A custom boolean value that can be set with the `--defs` argument. If a
  line begins or ends (whitespaces are ignored) with '@D:VALUE@' and 'VALUE' was
  set, or if it begins/ends with '@D!VALUE@' and 'VALUE' was NOT set, the line
  will be left in, otherwise the entire line is skipped. This can be used for
  example to put conditionals inside C/C++ preprocessor macro define blocks.

@TYPE@<br>
@SYMBOL@<br>
@ARGS@:<br>
  Any line containing at least one of these macros will be repeated as many
  times as there are prototype functions read from the file provided via the
  `--list` argument. On each iteration @TYPE@, @SYMBOL@ and @ARGS@ will be
  replaced with the return type, symbol name and function arguments list
  (without surrounding brackets). This is intended to be used to create lists
  with typedefs and function declarations.

Similar to C preprocessor lines the following macros must be on their own lines
without code around them, or else they will be ignored. Only whitespace
characters before or after them will be ignored.

@IF:VALUE@<br>
@IFNOT:VALUE@<br>
@ELSE:VALUE@<br>
@ENDIF:VALUE@:<br>
  Pre-processor conditionals similar to the ones used in C/C++. These will be
  processed before anything else.

@MACRO_START@<br>
@MACRO_END@:<br>
  On all lines between these two macros a trailing backslash ('\') will be added
  before the newline character. This should help writing long macros on editors
  that won't highlight code embedded in a macro block. Don't put a "#define"
  inside this block. The line before @MACRO_START@ must not end on a backslash.

@LOOP_START@<br>
@LOOP_END@:<br>
  Basically the same as @TYPE@, @SYMBOL@ and @ARGS@ but for multiline output.
  All lines between these two macros are repeated as many times as there are
  prototype functions read and the macros @TYPE@, @SYMBOL@ and @ARGS@ are
  replaced with the return type, symbol name and function arguments list of
  these prototypes on each iteration.


This is a list of standard definitions:

_HEADER<br>
_SOURCE:<br>
  Defined for the header and source parts when `--separate-files` was given

_W32:<br>
  Defined if --win32 was given

_CXXSTATIC:<br>
  Defined if "cpp-static" was set as output target

_DEFLIB:<br>
  Defined if a default library name was set

