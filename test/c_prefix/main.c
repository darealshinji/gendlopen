#include <stdio.h>
#include <assert.h>
#include "helloworld.h"

#define MYPREFIX_DEFAULT_LIB LIBNAME(helloworld,0)

/* include generated header file */
#include "c_test.h"


static void cb(const char *msg)
{
    puts(msg);
}

static void load_lib_args()
{
    /* filename of the library we want to load;
     * using the macro `LIBNAME` for cross-platform compatibility */
    const myprefix_char_t *filename = LIBNAME(helloworld,0);

    /* flags that will be passed to dlopen() or LoadLibraryEx() */
    const int flags = MYPREFIX_DEFAULT_FLAGS;

    /* whether or not to load symbols into a new namespace */
#ifdef __illumos__
    /* libhelloworld fails to use the callback if we use dlmopen() */
    const bool new_namespace = false;
#else
    const bool new_namespace = true;
#endif

    /* load library and symbols */
    if (!myprefix_load_lib_args(filename, flags, new_namespace) ||
        !myprefix_load_all_symbols())
    {
        /* print the last saved error */
        fprintf(stderr, "%s\n", myprefix_last_error());

        /* free library */
        myprefix_free_lib();

        exit(1);
    }
}

static void load_lib_name()
{
    const myprefix_char_t *filename = LIBNAME(helloworld,0);

    /* load library and each individual symbol */
    if (!myprefix_load_lib_name(filename) ||
        !myprefix_load_symbol(MYPREFIX_LOAD_helloworld_init) ||
        !myprefix_load_symbol(MYPREFIX_LOAD_helloworld_callback) ||
        !myprefix_load_symbol(MYPREFIX_LOAD_helloworld_hello) ||
        !myprefix_load_symbol(MYPREFIX_LOAD_helloworld_hello2) ||
        !myprefix_load_symbol(MYPREFIX_LOAD_helloworld_release))
    {
        fprintf(stderr, "%s\n", myprefix_last_error());
        myprefix_free_lib();
        exit(1);
    }
}

static void load_lib_and_symbols()
{
    if (!myprefix_load_lib_and_symbols()) {
        fprintf(stderr, "%s\n", myprefix_last_error());
        myprefix_free_lib();
        exit(1);
    }
}

static void print_lib_origin()
{
    const myprefix_char_t *origin = myprefix_lib_origin();

    if (origin) {
        printf("library loaded at: %s\n", origin);
    } else {
        fprintf(stderr, "%s\n", myprefix_last_error());
    }
}

int main(int argc, char *argv[])
{
    // #1
    load_lib_args();
    myprefix_free_lib();

    // #2
    load_lib_name();
    myprefix_free_lib();

    // #3
    load_lib_and_symbols();

    print_lib_origin();

    assert(myprefix_all_symbols_loaded() == true);
    assert(myprefix_no_symbols_loaded() == false);
    assert(myprefix_any_symbol_loaded() == true);

    ///////////////////////////////////////////////////////////////////////////

    /* our code */
    helloworld *hw = helloworld_init_argv(argc, argv);
    helloworld_callback = cb;
    helloworld_hello(hw);
    helloworld_hello2(hw, cb);

    printf("helloworld_buffer content: %s\n", helloworld_buffer);
    memcpy(helloworld_buffer, "abc", 4);
    printf("helloworld_buffer new content: %s\n", helloworld_buffer);

    helloworld_release(hw);

    /* free library */
    myprefix_free_lib();

    return 0;
}
