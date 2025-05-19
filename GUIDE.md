As an example let's assume a program prints a notification on startup using libnotify.
This is useful but not crucial for the program to work, so we decide to load it
dynamically at runtime instead. That's where gendlopen can be useful.

Here's our program:
``` C
#include <stdio.h>
#include <glib.h>
#include <libnotify/notify.h>

static void send_notification(const char *app, const char *msg)
{
    NotifyNotification *notify;

    printf(">> %s\n", msg);

    if (notify_init(app) && notify_is_initted()) {
        notify = notify_notification_new(msg, NULL, "dialog-information");
        notify_notification_show(notify, NULL);
        g_object_unref(G_OBJECT(notify));
        notify_uninit();
    }
}

static void my_program()
{
    // this would be our program's actual code ...
}

int main(int argc, char **argv)
{
    (void)argc;

    send_notification(argv[0], "program was started");
    my_program();

    return 0;
}
```

Compile and run it:
``` sh
gcc -Wall -O2 $(pkg-config --cflags libnotify gobject-2.0) main.c -o prog $(pkg-config --libs libnotify gobject-2.0)
./prog
```

It should show up a notification on the desktop.
It should also be linked against libnotify and glib:
``` sh
$ readelf -d prog | grep NEEDED
 0x0000000000000001 (NEEDED)             Shared library: [libnotify.so.4]
 0x0000000000000001 (NEEDED)             Shared library: [libgobject-2.0.so.0]
 0x0000000000000001 (NEEDED)             Shared library: [libc.so.6]
```

Now we want to be able to use libnotify dynamically so that the program continues to work
even if the notification feature isn't available.

First we need to figure out what symbols from libnotify are used.
You can use tools like `nm` to list them up:
``` sh
$ nm -g prog | grep 'notify_'
                 U notify_init
                 U notify_is_initted
                 U notify_notification_new
                 U notify_notification_show
                 U notify_uninit
```

We need to lookup their C prototype declarations and write them down in a text file.
This text file will be read by gendlopen to generate source code for dynamically loading these functions.

We can find the prototype declarations online, in the header files or sometimes in man pages.
In most cases the prototype declarations can be copied verbatim.
Commentary lines will be ignored by gendlopen when reading from the text.

Let's name this text file `input.txt`:
``` C
/* from libnotify/notify.h */
gboolean
notify_init (const char *app_name);
gboolean
notify_is_initted (void);
void
notify_uninit (void);

/* from libnotify/notification.h */
NotifyNotification *
notify_notification_new (const char *summary,
                         const char *body,
                         const char *icon);
gboolean
notify_notification_show (NotifyNotification *notification,
                          GError **error);
```

It's time to create the header file: `gendlopen input.txt -o libnotify_dynamic.h`

How to use it? The provided functions can be found with a description in the generated file.
The important ones for us now are the following:
 * `bool gdo_load_lib_name_and_symbols (const gdo_char_t *filename)`
 * `bool gdo_free_lib ()`
 * `const gdo_char_t *gdo_last_error ()`

`gdo_load_lib_name_and_symbols()` attempts to load the provided library name plus all
symbols from the input text file and returns `false` on error.
`gdo_last_error()` is used to print a meaningful error message and `gdo_free_lib()`
will free the library. `gdo_free_lib()` can always safely be called, even if nothing was loaded.
By default `gdo_char_t` is a typedef for `char`, on Windows it can also be `wchar_t`.

A few useful macros are provided too, such as `LIBNAME()` to deal with library filename
differences on different operating systems.
`LIBNAME(notify,4)` for example will be resolved to `"libnotify.so.4"` on Linux and
other ELF systems.

Here's our updated source:
``` C
#include <stdio.h>
#include <glib.h>
#include <libnotify/notify.h>

/// NEW ///
#include "libnotify_dynamic.h"
///////////

static void send_notification(const char *app, const char *msg)
{
    NotifyNotification *notify;

    printf(">> %s\n", msg);

    /// NEW ///
    if (!gdo_load_lib_name_and_symbols( LIBNAME(notify,4) )) {
        fprintf(stderr, LIBNAME(notify,4) ": error: %s\n", gdo_last_error());
        gdo_free_lib();
        return;
    }
    ///////////

    if (notify_init(app) && notify_is_initted()) {
        notify = notify_notification_new(msg, NULL, "dialog-information");
        notify_notification_show(notify, NULL);
        g_object_unref(G_OBJECT(notify));
        notify_uninit();
    }

    /// NEW ///
    gdo_free_lib();
    ///////////
}

static void my_program()
{
    // this would be our program's actual code ...
}

int main(int argc, char **argv)
{
    (void)argc;

    send_notification(argv[0], "program was started");
    my_program();

    return 0;
}
```

Compile but don't link against libnotify.
``` sh
gcc -Wall -O2 $(pkg-config --cflags libnotify gobject-2.0) main.c -o prog $(pkg-config --libs gobject-2.0) -ldl
./prog
```

The output of `readelf -d prog | grep NEEDED` should now be without `[libnotify.so.4]` and
`nm -g prog | grep 'notify_'` should print nothing.

If you want to check what happens if the library wasn't loaded correctly you can
replace `LIBNAME(notify,4)` with something like `LIBNAME(notify,99)`:
```
$ ./prog 
>> program was started
libnotify.so.99: error: libnotify.so.99: cannot open shared object file: No such file or directory
```

Instead of doing all the loading inside one function you can also load at the beginning
of your `main()` function.
For convenience you can set automatic release of resources with `gdo_enable_autorelease()`.
``` C
#include <stdio.h>
#include <glib.h>
#include <libnotify/notify.h>

/// NEW ///
#include "libnotify_dynamic.h"
///////////

static void send_notification(const char *app, const char *msg)
{
    NotifyNotification *notify;

    printf(">> %s\n", msg);

    /// NEW ///
    if (!gdo_all_symbols_loaded()) {
        return;
    }
    ///////////

    if (notify_init(app) && notify_is_initted()) {
        notify = notify_notification_new(msg, NULL, "dialog-information");
        notify_notification_show(notify, NULL);
        g_object_unref(G_OBJECT(notify));
        notify_uninit();
    }
}

static void my_program()
{
    // this would be our program's actual code ...
}

int main(int argc, char **argv)
{
    (void)argc;

    /// NEW ///
    if (gdo_enable_autorelease()) {
        gdo_load_lib_name_and_symbols( LIBNAME(notify,4) );
    }
    ///////////

    send_notification(argv[0], "program was started");
    my_program();

    return 0;
}
```

Compile and test:
``` sh
gcc -Wall -O2 $(pkg-config --cflags libnotify gobject-2.0) main.c -o prog $(pkg-config --libs gobject-2.0) -ldl
./prog
```
