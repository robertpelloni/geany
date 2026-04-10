/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2017, Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author(s): Carlos Garnacho <carlosg@gnome.org>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguieventcontroller.h>
#include <bobgui/bobguiimcontext.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_EVENT_CONTROLLER_KEY         (bobgui_event_controller_key_get_type ())
#define BOBGUI_EVENT_CONTROLLER_KEY(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_EVENT_CONTROLLER_KEY, BobguiEventControllerKey))
#define BOBGUI_EVENT_CONTROLLER_KEY_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_EVENT_CONTROLLER_KEY, BobguiEventControllerKeyClass))
#define BOBGUI_IS_EVENT_CONTROLLER_KEY(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_EVENT_CONTROLLER_KEY))
#define BOBGUI_IS_EVENT_CONTROLLER_KEY_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_EVENT_CONTROLLER_KEY))
#define BOBGUI_EVENT_CONTROLLER_KEY_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_EVENT_CONTROLLER_KEY, BobguiEventControllerKeyClass))

typedef struct _BobguiEventControllerKey BobguiEventControllerKey;
typedef struct _BobguiEventControllerKeyClass BobguiEventControllerKeyClass;

GDK_AVAILABLE_IN_ALL
GType               bobgui_event_controller_key_get_type  (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiEventController *bobgui_event_controller_key_new (void);

GDK_AVAILABLE_IN_ALL
void                bobgui_event_controller_key_set_im_context (BobguiEventControllerKey *controller,
                                                             BobguiIMContext          *im_context);
GDK_AVAILABLE_IN_ALL
BobguiIMContext *      bobgui_event_controller_key_get_im_context (BobguiEventControllerKey *controller);

GDK_AVAILABLE_IN_ALL
gboolean            bobgui_event_controller_key_forward        (BobguiEventControllerKey *controller,
                                                             BobguiWidget             *widget);
GDK_AVAILABLE_IN_ALL
guint               bobgui_event_controller_key_get_group      (BobguiEventControllerKey *controller);


G_END_DECLS

