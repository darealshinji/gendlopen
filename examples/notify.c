/* a more complex real-life example */

#include <libnotify/notify.h>


#ifdef USE_DLOPEN

#define XNOTIFY_DEFAULT_LIB "libnotify.so.4"
#define XNOTIFY_ATEXIT
#include "example_notify_notify.h"

#define XGOBJECT_DEFAULT_LIB "libgobject-2.0.so.0"
#define XGOBJECT_ATEXIT
#include "example_notify_gobject.h"

#undef G_OBJECT
#define G_OBJECT(x) (GObject *)x

#endif /* USE_DLOPEN */


int send_notification(const char *app_name, const char *summary, const char *body, const char *icon)
{
  if (!app_name || !*app_name || !summary || !*summary || !notify_init(app_name)) {
    return 1;
  }

  if (!icon || !*icon) {
    icon = "dialog-information";
  }

  int rv = 0;
  NotifyNotification *p = notify_notification_new(summary, body, icon);
  if (!notify_notification_show(p, NULL)) rv = 1;

  g_object_unref(G_OBJECT(p));
  notify_uninit();

  return rv;
}

int main(int, char **argv)
{
#ifdef USE_DLOPEN
  /* libnotify */
  if (!xnotify_load_lib_and_symbols()) {
    fprintf(stderr, "%s\n", xnotify_last_error());
    return 1;
  }

  /* libgobject */
  if (!xgobject_load_lib_and_symbols()) {
    fprintf(stderr, "%s\n", xgobject_last_error());
    return 1;
  }
#endif /* USE_DLOPEN */

  return send_notification(argv[0], "Hello", "Hi there!", NULL);
}


