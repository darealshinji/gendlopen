/*
An object to represent the application as an application indicator
in the system panel.

Copyright 2009 Canonical Ltd.

Authors:
    Ted Gould <ted@canonical.com>
    Cody Russell <cody.russell@canonical.com>

This program is free software: you can redistribute it and/or modify it
under the terms of either or both of the following licenses:

1) the GNU Lesser General Public License version 3, as published by the
   Free Software Foundation; and/or
2) the GNU Lesser General Public License version 2.1, as published by
   the Free Software Foundation.

This program is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranties of
MERCHANTABILITY, SATISFACTORY QUALITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the applicable version of the GNU Lesser General Public
License for more details.

You should have received a copy of both the GNU Lesser General Public
License version 3 and version 2.1 along with this program.  If not, see
<http://www.gnu.org/licenses/>
*/

/* shortened version */

#ifndef __APP_INDICATOR_H__
#define __APP_INDICATOR_H__

#include <gtk/gtk.h>

typedef struct _AppIndicator        AppIndicator;
typedef struct _AppIndicatorPrivate AppIndicatorPrivate;

typedef enum { /*< prefix=APP_INDICATOR_CATEGORY >*/
	APP_INDICATOR_CATEGORY_APPLICATION_STATUS, /*< nick=ApplicationStatus >*/
	APP_INDICATOR_CATEGORY_COMMUNICATIONS, /*< nick=Communications >*/
	APP_INDICATOR_CATEGORY_SYSTEM_SERVICES, /*< nick=SystemServices >*/
	APP_INDICATOR_CATEGORY_HARDWARE, /*< nick=Hardware >*/
	APP_INDICATOR_CATEGORY_OTHER /*< nick=Other >*/
} AppIndicatorCategory;

typedef enum { /*< prefix=APP_INDICATOR_STATUS >*/
	APP_INDICATOR_STATUS_PASSIVE, /*< nick=Passive >*/
	APP_INDICATOR_STATUS_ACTIVE, /*< nick=Active >*/
	APP_INDICATOR_STATUS_ATTENTION /*< nick=NeedsAttention >*/
} AppIndicatorStatus;

struct _AppIndicator {
	GObject parent;

	/*< Private >*/
	AppIndicatorPrivate *priv;
};


AppIndicator                   *app_indicator_new                (const gchar          *id,
                                                                  const gchar          *icon_name,
                                                                  AppIndicatorCategory  category);

void                            app_indicator_set_status         (AppIndicator       *self,
                                                                  AppIndicatorStatus  status);

void                            app_indicator_set_menu           (AppIndicator       *self,
                                                                  GtkMenu            *menu);

#endif /* __APP_INDICATOR_H__ */
