/* a more complex real-life example */

#include <libnotify/notify.h>
#include <assert.h>

#ifdef USE_DLOPEN
#define XNOTIFY_ENABLE_AUTOLOAD 1
#define XGOBJECT_ENABLE_AUTOLOAD 1
#include "example_notify.h"
#include "example_notify_gobject.h"
#endif


int send_notification(const char *app_name, const char *summary, const char *body, const char *icon)
{
  assert(app_name && *app_name && summary && *summary);

  if (!notify_init(app_name) || !notify_is_initted()) {
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
  return send_notification(argv[0], "Hello", "Hi there!", NULL);
}

