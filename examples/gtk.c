/**
 Licensed under the MIT License <http://opensource.org/licenses/MIT>.
 SPDX-License-Identifier: MIT
 Copyright (c) 2025 Carsten Janssen

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

/* Chose between Gtk2 and Gtk3 during runtime */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

#include "example_gtk.h"


static void show_gtk_message_box(const char *title, const char *msg1, const char *msg2)
{
    gtk_init(NULL, NULL);

    GtkWidget *dialog = gtk_message_dialog_new(
                            NULL,
                            0,
                            GTK_MESSAGE_INFO,
                            GTK_BUTTONS_OK,
                            "%s%s",
                            msg1, msg2);
    gtk_window_set_title(GTK_WINDOW(dialog), title);
    gtk_dialog_run(GTK_DIALOG(dialog));

    gtk_widget_destroy(dialog);
}


int main(int argc, char **argv)
{
    const char *title = "Gtk 3";
    const char *lib = LIBNAME(gtk-3, 0);
    const char *msg = "Library loaded:\n";
    const char *orig = "";

    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            printf("usage: %s [--help|--gtk2|--gtk3]\n", argv[0]);
            return 0;
        } else if (strcmp(argv[1], "--gtk2") == 0) {
            title = "Gtk 2";
            lib = LIBNAME(gtk-x11-2.0, 0);
        } else if (strcmp(argv[1], "--gtk3") == 0) {
            /* use default values */
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[1]);
            return 1;
        }
    }

    if (!gdo_load_lib_name_and_symbols(lib)) {
        fprintf(stderr, "error: %s: %s\n", lib, gdo_last_error());
        gdo_free_lib();
        return 1;
    }

    if ((orig = gdo_lib_origin()) == NULL) {
        msg = "error: gdo_lib_origin()";
    }

    show_gtk_message_box(title, msg, orig);

    gdo_free_lib();

    return 0;
}
