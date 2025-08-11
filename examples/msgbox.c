/**
 * This example should illustrate how to use dynamic loading to show a message window.
 * We try Gtk+, SDL, FLTK and X11 in that order.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/* toolkit/X11 headers */
#include <gtk/gtk.h>
#include <SDL.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/XKBlib.h>

/* FLTK, declared in "FL/fl_ask.H" */
#define fl_message_MANGLED        _Z10fl_messagePKcz
#define fl_message_title_MANGLED  _Z16fl_message_titlePKc
extern void fl_message_MANGLED(const char *format, const char *message);
extern void fl_message_title_MANGLED(const char *title);

/* generated header files */
#include "example_msgbox_gtk.h"
#include "example_msgbox_sdl.h"
#include "example_msgbox_fltk.h"
#include "example_msgbox_x11.h"

#define LIBNAME(...)  DL_GTK_LIBNAME(__VA_ARGS__)
#define LIBEXT        DL_GTK_LIBEXT

/* same as _countof() */
#define COUNTOF(array) (sizeof(array) / sizeof(array[0]))


enum {
    TK_ALL  = 0,
    TK_GTK  = 1,
    TK_SDL  = 2,
    TK_FLTK = 3,
    TK_X11  = 4
};


/* print error message, avoid double printing of library name */
static void print_error(const char *title, const char *library, const char *errmsg)
{
    if (strstr(errmsg, library)) {
        fprintf(stderr, "%s: %s\n", title, errmsg);
    } else {
        fprintf(stderr, "%s: %s: %s\n", title, library, errmsg);
    }
}


/* Gtk+ message box */
static void show_gtk_message_box(const char *msg)
{
    const char *title = "Gtk+ Info";

    gtk_init(NULL, NULL);

    GtkWidget *dialog = gtk_message_dialog_new(
                            NULL,
                            0,
                            GTK_MESSAGE_INFO,
                            GTK_BUTTONS_OK,
                            "%s",
                            msg);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_destroy(dialog);
}


/* SDL message box */
static void show_sdl_message_box(const char *msg)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "SDL Info", msg, NULL);
}


/* FLTK message box */
static void show_fltk_message_box(const char *msg)
{
    fl_message_title_MANGLED("FLTK Info");
    fl_message_MANGLED("%s", msg);
}


/* X11 message box (lowlevel Xlib API) */
static void show_x11_message_box(const char *msg)
{
    const char *title = "X11 Info";

    Display *display = XOpenDisplay(NULL);

    if (!display) {
        return;
    }

    const int w = 240, h = 100;

    XTextItem items[1] = {
        { (char *)msg, strlen(msg), 0, None }
    };

    XSetWindowAttributes attributes;

    memset(&attributes, 0, sizeof(XSetWindowAttributes));
    attributes.background_pixel = 0x333333; /* dark grey */
    attributes.event_mask = ExposureMask | KeyPressMask;

    /* create window */
    int screen = DefaultScreen(display);

    Window window = XCreateWindow(
        display,
        DefaultRootWindow(display),
        0, 0,
        w, h,
        0,
        DefaultDepth(display, screen),
        InputOutput,
        DefaultVisual(display, screen),
        CWBackPixel | CWEventMask,
        &attributes);

    XStoreName(display, window, title);

    GC gc = XCreateGC(display, window, 0, NULL);
    XSetForeground(display, gc, WhitePixel(display, screen));

    Atom wm_delete = XInternAtom(display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(display, window, &wm_delete, 1);

    XMapWindow(display, window);

    /* move to screen center */
    int x = (DisplayWidth(display, screen) - w) / 2;
    int y = (DisplayHeight(display, screen) - h) / 2;
    XMoveWindow(display, window, x, y);

    /* event loop */
    bool loop = true;
    XEvent event;

    memset(&event, 0, sizeof(XEvent));

    while (loop)
    {
        XNextEvent(display, &event);

        switch (event.type)
        {
        case Expose:
            /* draw text */
            x = 16; y = h/2;
            XDrawText(display, window, gc, x, y, items, COUNTOF(items));
            break;

        case ClientMessage:
            if ((Atom)event.xclient.data.l[0] == wm_delete) {
                /* stop main loop */
                loop = false;
            }
            break;

        case KeyPress:
            /* stop if ESCAPE was pressed */
            if (XkbKeycodeToKeysym(display, event.xkey.keycode, 0, 0) == 0xFF1B) {
                loop = false;
            }
            break;

        default:
            break;
        }
    }

    /* free resources */
    XFreeGC(display, gc);
    XUnmapWindow(display, window);
    XDestroyWindow(display, window);
    XCloseDisplay(display);
}


/* try to load from a list of library names */
static bool load_from_list(
        const char *name,  /* generic name of library/API for error messages */
        const char **list, /* list of filenames; last element must be NULL */

        /* pointers to functions from the generated headers,
         * used to load library and symbols */
        bool        (*load_lib_name) (const char *),
        const char *(*last_error) (),
        bool        (*lib_is_loaded) (),
        bool        (*load_all_symbols) (),
        char *      (*lib_origin) ())
{
    const char *library = *list;

    for (const char **p = list; *p != NULL; p++) {
        library = *p;

        if (load_lib_name(library)) {
            break;
        } else {
            print_error("warning", library, last_error());
        }
    }

    if (!lib_is_loaded()) {
        fprintf(stderr, "error: %s not loaded\n", name);
        return false;
    }

    if (!load_all_symbols()) {
        print_error("error", library, last_error());
        return false;
    }

    char *orig = lib_origin();

    if (orig) {
        printf("loaded: %s\n", orig);
        free(orig);
    }

    return true;
}


#define MAKE_LIST(...) \
    const char *list[] = { __VA_ARGS__, NULL }

#define LOAD_FROM_LIST(NAME, PREFIX) \
    load_from_list(NAME, list, \
        PREFIX##load_lib_name, \
        PREFIX##last_error, \
        PREFIX##lib_is_loaded, \
        PREFIX##load_all_symbols, \
        PREFIX##lib_origin)


/* load Gtk+ v3 or v2 */
static bool load_gtk()
{
    MAKE_LIST(
        LIBNAME(gtk-3, 0),
        LIBNAME(gtk-x11-2.0, 0)
    );

    return LOAD_FROM_LIST("libgtk", dl_gtk_);
}


/* load SDL v3 or v2 */
static bool load_sdl()
{
    MAKE_LIST(
        LIBNAME(SDL3, 0),
        LIBNAME(SDL2-2.0, 0)
    );

    return LOAD_FROM_LIST("libSDL", dl_sdl_);
}


/* try to load different versions of FLTK */
static bool load_fltk()
{
    MAKE_LIST(
        LIBNAME(fltk, 1.5),
        LIBNAME(fltk, 1.4),
        LIBNAME(fltk, 1.3),
        "libfltk" LIBEXT,
        "fltk" LIBEXT
    );

    return LOAD_FROM_LIST("libfltk", dl_fltk_);
}


/* load libX11 */
static bool load_x11()
{
    MAKE_LIST( LIBNAME(X11, 6) );

    return LOAD_FROM_LIST("libX11", dl_x11_);
}


/* try to show a message box window */
static void show_message_box(const char *msg, int tk)
{
    switch (tk)
    {
    /* try them all */
    case TK_ALL:
        /* FALLTHROUGH */

    /* Gtk+ has a nice looking message box window
     * that we want to try first */
    case TK_GTK:
        if (load_gtk()) {
            show_gtk_message_box(msg);
            break;
        }
        if (tk != TK_ALL) {
            break;
        }
        /* FALLTHROUGH */

    /* try out SDL next */
    case TK_SDL:
        if (load_sdl()) {
            show_sdl_message_box(msg);
            break;
        }
        if (tk != TK_ALL) {
            break;
        }
        /* FALLTHROUGH */

    /* try FLTK before we fall back to Xlib */
    case TK_FLTK:
        if (load_fltk()) {
            show_fltk_message_box(msg);
            break;
        }
        if (tk != TK_ALL) {
            break;
        }
        /* FALLTHROUGH */

    /* fall back to plain low level X11 API */
    case TK_X11:
        if (load_x11()) {
            show_x11_message_box(msg);
        }
        break;

    default:
        break;
    }
}


int main(int argc, char **argv)
{
    const char *msg = "Very important information!";

    /* by default try through all the toolkits */
    int toolkit = TK_ALL;

    /* parse argument */
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            printf("usage:\n"
                " %s [gtk|sdl|fltk|x11]\n"
                " %s --help\n", argv[0], argv[0]);
            return 0;
        }

        /* test a specific toolkit */
        if (strcasecmp(argv[1], "gtk") == 0) {
            toolkit = TK_GTK;
        } else if (strcasecmp(argv[1], "sdl") == 0) {
            toolkit = TK_SDL;
        } else if (strcasecmp(argv[1], "fltk") == 0) {
            toolkit = TK_FLTK;
        } else if (strcasecmp(argv[1], "x11") == 0) {
            toolkit = TK_X11;
        } else {
            fprintf(stderr, "warning: command ignored: %s\n", argv[1]);
        }
    }

    printf("Message: %s\n", msg);

    show_message_box(msg, toolkit);

    /* free resources (can safely be called even if nothing was loaded) */
    dl_gtk_free_lib();
    dl_sdl_free_lib();
    dl_fltk_free_lib();
    dl_x11_free_lib();

    return 0;
}
