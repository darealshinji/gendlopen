# Save output

Calling gendlopen with an input file will generate a header file and print
it to STDOUT: `gendlopen input.txt`

To save it into a file use `-o`: `gendlopen input.txt -o output.h`

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
gdo::dl(); // c'tor
bool gdo::load(const std::string &filename, int flags=default_flags, bool new_namespace=false);
bool gdo::load_all_symbols();
bool gdo::free(bool force=false); // called by d'tor
std::string error();
#ifdef GDO_WINAPI
bool gdo::load(const std::wstring &filename, int flags=default_flags, bool new_namespace=false);
std::wstring error_w();
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

