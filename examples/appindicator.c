/* a more complex real-life example */

#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>


#ifdef USE_DLOPEN

#define XAPPINDICATOR_DEFAULT_LIB "libappindicator.so.1"
#define XAPPINDICATOR_ATEXIT
#include "example_appindicator_appindicator.h"

#define XGOBJECT_DEFAULT_LIB "libgobject-2.0.so.0"
#define XGOBJECT_ATEXIT
#include "example_appindicator_gobject.h"

#define XGTK_DEFAULT_LIB "libgtk-x11-2.0.so.0"
#define XGTK_ATEXIT
#include "example_appindicator_gtk.h"

#undef G_OBJECT
#define G_OBJECT(x) (GObject *)x

#endif /* USE_DLOPEN */


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
#ifdef USE_DLOPEN
  /* libappindicator */
  if (!xappindicator_load_lib_and_symbols()) {
    fprintf(stderr, "%s\n", xappindicator_last_error());
    return 1;
  }

  /* libgobject */
  if (!xgobject_load_lib_and_symbols()) {
    fprintf(stderr, "%s\n", xgobject_last_error());
    return 1;
  }

  /* libgtk */
  if (!xgtk_load_lib_and_symbols()) {
    fprintf(stderr, "%s\n", xgtk_last_error());
    return 1;
  }
#endif /* USE_DLOPEN */

  return indicator(NULL, NULL, callback);
}


