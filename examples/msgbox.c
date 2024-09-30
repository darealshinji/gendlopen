/**
 * The MIT License (MIT)
 *
 * Copyright (c) 2024 Carsten Janssen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE
 */

/**
 * This example should illustrate how to use dynamic loading to show a message window.
 * First we try Gtk+, then SDL and then fall back to X11.
 */

//#define TEST_SDL
//#define TEST_X11

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <gtk/gtk.h>
#include <SDL.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "example_msgbox_gtk.h"
#include "example_msgbox_sdl.h"
#include "example_msgbox_x11.h"

#ifdef __APPLE__
# define ESCAPE_KEY 61
#else
# define ESCAPE_KEY 9
#endif


inline void print_error(const char *msg)
{
    fprintf(stderr, "error: %s\n", msg);
}

/* Gtk+ */
void show_gtk_message_box(const char *msg, const char *title)
{
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

/* SDL */
void show_sdl_message_box(const char *msg, const char *title)
{
    SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_INFORMATION, title, msg, NULL);
}

/* X11 */
void show_x11_message_box(const char *msg, const char *title)
{
    Display *display = XOpenDisplay(NULL);

    if (!display) {
        return;
    }

    const int w = 240, h = 100;

    XTextItem items[] = {
        { (char *)msg, strlen(msg), 0, None }
    };

    XSetWindowAttributes attributes = {
        .background_pixel = 0x333333, /* dark grey */
        .event_mask = ExposureMask | KeyPressMask
    };

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

    while (loop)
    {
        XNextEvent(display, &event);

        switch (event.type)
        {
        case Expose:
            XDrawText(display, window, gc, 12, 22, items, 1);
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
    if (dl_gtk_load_lib_name_and_symbols( LIBNAME(gtk-x11-2.0, 0) )) {
        return true;
    }

    print_error(dl_gtk_last_error());
    return false;
}

bool load_sdl()
{
    if (dl_sdl_load_lib_name_and_symbols( LIBNAME(SDL2-2.0, 0) )) {
        return true;
    }

    print_error(dl_sdl_last_error());
    return false;
}

bool load_x11()
{
    if (dl_x11_load_lib_name_and_symbols( LIBNAME(X11, 6) )) {
        return true;
    }

    print_error(dl_x11_last_error());
    return false;
}

/* load libraries and show message box window */
void show_message_box(const char *msg)
{
#ifdef TEST_SDL
    goto JMP_SDL;
#endif
#ifdef TEST_X11
    goto JMP_X11;
#endif

    /* Gtk+ has a nice looking message box window
     * that we want to try first */
    if (load_gtk()) {
        show_gtk_message_box(msg, "Gtk+ Info");
        return;
    }

    goto JMP_SDL; /* silence [-Wunused-label] */
JMP_SDL:

    /* try out SDL next */
    if (load_sdl()) {
        show_sdl_message_box(msg, "SDL Info");
        return;
    }

    goto JMP_X11; /* silence [-Wunused-label] */
JMP_X11:

    /* fall back to plain low level X11 API */
    if (load_x11()) {
        show_x11_message_box(msg, "X11 Info");
    }
}

/* can safely be called even if nothing was loaded */
void free_libraries()
{
    dl_gtk_free_lib();
    dl_sdl_free_lib();
    dl_x11_free_lib();
}


int main()
{
    const char *msg = "Very important information!";

    printf("Message >>> %s\n", msg);
    show_message_box(msg);
    free_libraries();

    return 0;
}
