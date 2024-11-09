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

/* a more complex real-life example */

//#include <gtk/gtk.h>
//#include <libappindicator/app-indicator.h>

#include "example_appindicator.h"
#include "example_appindicator_gobject.h"
#include "example_appindicator_gtk.h"


int indicator(const char *appID, const char *icon, void (*callback)())
{
    GtkWidget *window;
    GtkMenu *menu;
    GtkActionGroup *action_group;
    GtkUIManager *uim;
    AppIndicator *indicator;

    if (!appID || !*appID) {
        appID = "my-app";
    }

    if (!icon || !*icon) {
        icon = "dialog-information";
    }

    const GtkActionEntry entries[] = {
        { "Action", "dialog-information", "_Action", NULL, NULL, callback },
        { "Close",  "dialog-close",       "_Close",  NULL, NULL, gtk_main_quit },
    };

    const gchar *ui_info =
        "<ui>"
            "<popup name='IndicatorPopup'>"
                "<menuitem action='Action' />"
                "<menuitem action='Close' />"
            "</popup>"
        "</ui>";

    gtk_init(NULL, NULL);
    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

    action_group = gtk_action_group_new ("AppActions");
    gtk_action_group_add_actions (action_group, entries, 2, window);

    uim = gtk_ui_manager_new();
    g_object_set_data (G_OBJECT(window), "ui-manager", uim);
    gtk_ui_manager_insert_action_group (uim, action_group, 0);

    if (!gtk_ui_manager_add_ui_from_string (uim, ui_info, -1, NULL)) {
        return 1;
    }

    indicator = app_indicator_new (appID, icon, APP_INDICATOR_CATEGORY_OTHER);
    menu = (GtkMenu *)gtk_ui_manager_get_widget (uim, "/ui/IndicatorPopup");
    app_indicator_set_status (indicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_menu (indicator, menu);

    gtk_main();

    return 0;
}

static void callback() {
    printf("callback!\n");
}

int main()
{
#define XLOAD(x) \
    if (!x##_load_lib_and_symbols()) { \
        fprintf(stderr, "an error has occurred trying to dynamically load `%s':\n", default_lib); \
        fprintf(stderr, "%s\n", x##_last_error()); \
        return 1; \
    }

    const char *default_lib = XGOBJECT_DEFAULT_LIBA;
    XLOAD(xgobject)

    default_lib = XGTK_DEFAULT_LIBA;
    XLOAD(xgtk)

    default_lib = XAPPINDICATOR_DEFAULT_LIBA;
    XLOAD(xappindicator)

    return indicator(NULL, NULL, callback);
}


