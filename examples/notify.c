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

/* a more complex real-life example */

#include <libnotify/notify.h>
#include <assert.h>

#define XNOTIFY_ENABLE_AUTOLOAD   1
#define XGOBJECT_ENABLE_AUTOLOAD  1
#include "example_notify.h"
#include "example_notify_gobject.h"


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

int main(int argc, char **argv)
{
    (void)argc;
    return send_notification(argv[0], "Hello", "Hi there!", NULL);
}

