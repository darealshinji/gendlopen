/* Choose between Gtk2 and Gtk3 during runtime */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <gtk/gtk.h>

/* generated header */
#include "example_gtk.h"


/* Gtk+ code */
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
    const char *library = GDO_LIBNAME(gtk-3, 0);
    const char *msg, *ptr_origin;
    char *origin;

    /* argument parsing */
    if (argc > 1) {
        if (strcmp(argv[1], "--help") == 0) {
            printf("usage: %s [--help|--gtk2|--gtk3]\n", argv[0]);
            return 0;
        } else if (strcmp(argv[1], "--gtk2") == 0) {
            title = "Gtk 2";
            library = GDO_LIBNAME(gtk-x11-2.0, 0);
        } else if (strcmp(argv[1], "--gtk3") == 0) {
            /* use default values */
        } else {
            fprintf(stderr, "unknown argument: %s\n", argv[1]);
            return 1;
        }
    }

    /* load library and symbols */
    if (!gdo_load_lib_name_and_symbols(library)) {
        fprintf(stderr, "error: %s: %s\n", library, gdo_last_error());
        gdo_free_lib();
        return 1;
    }

    /* get origin path of loaded library */
    origin = gdo_lib_origin();

    if (origin) {
        msg = "Library loaded:\n";
        ptr_origin = origin;
    } else {
        msg = "error: gdo_lib_origin()";
        ptr_origin = "";
    }

    /* execute code */
    show_gtk_message_box(title, msg, ptr_origin);

    /* free resources */
    gdo_free_lib();
    free(origin);

    return 0;
}
