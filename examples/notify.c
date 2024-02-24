/* a more complex real-life example */

#include <libnotify/notify.h>

#ifdef USE_DLOPEN
#include "example_notify.h"
#include "example_notify_gobject.h"
#endif


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

#define XLOAD(x) \
  if (!x##_load_lib_and_symbols()) { \
    fprintf(stderr, "%s\n", x##_last_error()); \
    return 1; \
  }

  XLOAD(xgobject)
  XLOAD(xnotify)

#endif /* USE_DLOPEN */

  return send_notification(argv[0], "Hello", "Hi there!", NULL);
}


