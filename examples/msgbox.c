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

/* same as _countof() */
#define COUNTOF(array) (sizeof(array) / sizeof(array[0]))


enum {
    TK_ALL  = 0,
    TK_GTK  = 1,
    TK_SDL  = 2,
    TK_FLTK = 3,
    TK_X11  = 4
};

/* forward declarations */
static bool keycode_is_esc(Display *display, KeyCode keycode);
static void print_error(const char *title, const char *library, const char *errmsg);


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
            /* stop if ESC was pressed */
            if (keycode_is_esc(display, event.xkey.keycode)) {
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


/* check if keycode was Escape key */
static bool keycode_is_esc(Display *display, KeyCode keycode)
{
    char *keyname = XKeysymToString(XkbKeycodeToKeysym(display, keycode, 0, 0));

    return (keyname && (strcasecmp(keyname, "Escape") == 0 ||
                        strcasecmp(keyname, "ESC") == 0));
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


/* print error message, avoid double printing of library name */
static void print_error(const char *title, const char *library, const char *errmsg)
{
    if (strstr(errmsg, library)) {
        fprintf(stderr, "%s: %s\n", title, errmsg);
    } else {
        fprintf(stderr, "%s: %s: %s\n", title, library, errmsg);
    }
}


/* load Gtk+ v3 or v2 */
static bool load_gtk()
{
    const char *list[] = {
        LIBNAME(gtk-3, 0),
        LIBNAME(gtk-x11-2.0, 0),
        NULL
    };

    return load_from_list("libgtk", list,
        dl_gtk_load_lib_name,
        dl_gtk_last_error,
        dl_gtk_lib_is_loaded,
        dl_gtk_load_all_symbols,
        dl_gtk_lib_origin);
}


/* load SDL v3 or v2 */
static bool load_sdl()
{
    const char *list[] = {
        LIBNAME(SDL3, 0),
        LIBNAME(SDL2-2.0, 0),
        NULL
    };

    return load_from_list("libSDL", list,
        dl_sdl_load_lib_name,
        dl_sdl_last_error,
        dl_sdl_lib_is_loaded,
        dl_sdl_load_all_symbols,
        dl_sdl_lib_origin);

    return true;
}


/* try to load different versions of FLTK */
static bool load_fltk()
{
    const char *list[] = {
        LIBNAME(fltk, 1.5),
        LIBNAME(fltk, 1.4),
        LIBNAME(fltk, 1.3),
        "libfltk" LIBEXT,
        "fltk" LIBEXT,
        NULL
    };

    return load_from_list("libfltk", list,
        dl_fltk_load_lib_name,
        dl_fltk_last_error,
        dl_fltk_lib_is_loaded,
        dl_fltk_load_all_symbols,
        dl_fltk_lib_origin);

    return true;
}


/* load libX11 */
static bool load_x11()
{
    if (dl_x11_load_lib_name_and_symbols( LIBNAME(X11, 6) )) {
        char *orig = dl_x11_lib_origin();

        if (orig) {
            printf("loaded: %s\n", orig);
            free(orig);
        }
        return true;
    }

    fprintf(stderr, "error: %s\n", dl_x11_last_error());

    return false;
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
