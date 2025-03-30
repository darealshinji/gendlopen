/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2024-2025 Carsten Janssen

 Permission is hereby  granted, free of charge, to any  person obtaining a copy
 of this software and associated  documentation files (the "Software"), to deal
 in the Software  without restriction, including without  limitation the rights
 to  use, copy,  modify, merge,  publish, distribute,  sublicense, and/or  sell
 copies  of  the Software,  and  to  permit persons  to  whom  the Software  is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all
 copies or substantial portions of the Software.

 THE SOFTWARE  IS PROVIDED "AS  IS", WITHOUT WARRANTY  OF ANY KIND,  EXPRESS OR
 IMPLIED,  INCLUDING BUT  NOT  LIMITED TO  THE  WARRANTIES OF  MERCHANTABILITY,
 FITNESS FOR  A PARTICULAR PURPOSE AND  NONINFRINGEMENT. IN NO EVENT  SHALL THE
 AUTHORS  OR COPYRIGHT  HOLDERS  BE  LIABLE FOR  ANY  CLAIM,  DAMAGES OR  OTHER
 LIABILITY, WHETHER IN AN ACTION OF  CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE  OR THE USE OR OTHER DEALINGS IN THE
 SOFTWARE.
**/

/**
 * This example should illustrate how to use dynamic loading to show a message window.
 * We try Gtk+, SDL, FLTK and X11 in that order.
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <strings.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <SDL.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

/* GNU name mangling scheme */
#define fl_message_MANGLED        _Z10fl_messagePKcz
#define fl_message_title_MANGLED  _Z16fl_message_titlePKc

/* <FL/fl_ask.H> */
extern void fl_message_MANGLED(const char *format, const char *message);
extern void fl_message_title_MANGLED(const char *title);

/* generated header files */
#include "example_msgbox_gtk.h"
#include "example_msgbox_sdl.h"
#include "example_msgbox_fltk.h"
#include "example_msgbox_x11.h"


enum {
    TK_ALL  = 0,
    TK_GTK  = 1,
    TK_SDL  = 2,
    TK_FLTK = 3,
    TK_X11  = 4
};


void show_gtk_message_box(const char *msg);  /* Gtk+ */
void show_sdl_message_box(const char *msg);  /* SDL */
void show_fltk_message_box(const char *msg); /* FLTK */
void show_x11_message_box(const char *msg);  /* X11 */

/* load libraries and symbols */
bool load_gtk();
bool load_sdl();
bool load_fltk();
bool load_x11();

/* load libraries and show message box window */
void show_message_box(const char *msg, int tk);



void print_origin(const char *path)
{
    if (path) {
        printf("loaded: %s\n", path);
    }
}

void show_gtk_message_box(const char *msg)
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

void show_sdl_message_box(const char *msg)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, "SDL Info", msg, NULL);
}

void show_fltk_message_box(const char *msg)
{
    fl_message_title_MANGLED("FLTK Info");
    fl_message_MANGLED("%s", msg);
}

void show_x11_message_box(const char *msg)
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
    const int ESCAPE_KEY = 9; /* Linux only? */
    bool loop = true;
    XEvent event;

    memset(&event, 0, sizeof(XEvent));

    while (loop)
    {
        XNextEvent(display, &event);

        switch (event.type)
        {
        case Expose:
            x = 16;
            y = h/2;
            /* _countof macro provided by generated headers */
            XDrawText(display, window, gc, x, y, items, _countof(items));
            break;

        case ClientMessage:
            if ((Atom)event.xclient.data.l[0] == wm_delete) {
                loop = false;
            }
            break;

        case KeyPress:
            if (event.xkey.keycode == ESCAPE_KEY) {
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

bool load_gtk()
{
    /* try Gtk3 first, then Gtk2 */
    if (!dl_gtk_load_lib_name( LIBNAME(gtk-3, 0) )) {
        fprintf(stderr, "warning: %s\n", dl_gtk_last_error());
        dl_gtk_load_lib_name( LIBNAME(gtk-x11-2.0, 0) );
    }

    if (dl_gtk_lib_is_loaded() && dl_gtk_load_all_symbols()) {
        print_origin(dl_gtk_lib_origin());
        return true;
    }

    fprintf(stderr, "error: %s\n", dl_gtk_last_error());

    return false;
}

bool load_sdl()
{
    /* try SDL3 first, then SDL2 */
    if (!dl_sdl_load_lib_name( LIBNAME(SDL3, 0) )) {
        fprintf(stderr, "warning: %s\n", dl_sdl_last_error());
        dl_sdl_load_lib_name( LIBNAME(SDL2-2.0, 0) );
    }

    if (dl_sdl_lib_is_loaded() && dl_sdl_load_all_symbols()) {
        print_origin(dl_sdl_lib_origin());
        return true;
    }

    fprintf(stderr, "error: %s\n", dl_sdl_last_error());

    return false;
}

bool load_fltk()
{
    /* try "libfltk.so.1.3" first, then "libfltk.so" */
    if (!dl_fltk_load_lib_name( LIBNAME(fltk, 1.3) )) {
        fprintf(stderr, "warning: %s\n", dl_fltk_last_error());
        dl_fltk_load_lib_name( "libfltk" LIBEXT );
    }

    if (dl_fltk_lib_is_loaded() && dl_fltk_load_all_symbols()) {
        print_origin(dl_fltk_lib_origin());
        return true;
    }

    fprintf(stderr, "error: %s\n", dl_fltk_last_error());

    return false;
}

bool load_x11()
{
    if (dl_x11_load_lib_name_and_symbols( LIBNAME(X11, 6) )) {
        print_origin(dl_x11_lib_origin());
        return true;
    }

    fprintf(stderr, "error: %s\n", dl_x11_last_error());

    return false;
}

void show_message_box(const char *msg, int tk)
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
    int tk = TK_ALL;

    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            printf("usage:\n"
                " %s [gtk|sdl|fltk|x11]\n"
                " %s --help\n", argv[0], argv[0]);
            return 0;
        }

        /* test a specific toolkit */
        if (strcasecmp(argv[1], "gtk") == 0) {
            tk = TK_GTK;
        } else if (strcasecmp(argv[1], "sdl") == 0) {
            tk = TK_SDL;
        } else if (strcasecmp(argv[1], "fltk") == 0) {
            tk = TK_FLTK;
        } else if (strcasecmp(argv[1], "x11") == 0) {
            tk = TK_X11;
        } else {
            fprintf(stderr, "warning: command ignored: %s\n", argv[1]);
        }
    }

    printf("Message: %s\n", msg);
    show_message_box(msg, tk);

    /* can safely be called even if nothing was loaded */
    dl_gtk_free_lib();
    dl_sdl_free_lib();
    dl_fltk_free_lib();
    dl_x11_free_lib();

    return 0;
}
