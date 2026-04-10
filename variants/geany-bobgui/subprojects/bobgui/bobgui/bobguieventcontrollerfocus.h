/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2020, Red Hat, Inc.
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
 * Author(s): Matthias Clasen <mclasen@redhat.com>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguieventcontroller.h>
#include <bobgui/bobguiimcontext.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_EVENT_CONTROLLER_FOCUS         (bobgui_event_controller_focus_get_type ())
#define BOBGUI_EVENT_CONTROLLER_FOCUS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_EVENT_CONTROLLER_FOCUS, BobguiEventControllerFocus))
#define BOBGUI_EVENT_CONTROLLER_FOCUS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_EVENT_CONTROLLER_FOCUS, BobguiEventControllerFocusClass))
#define BOBGUI_IS_EVENT_CONTROLLER_FOCUS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_EVENT_CONTROLLER_FOCUS))
#define BOBGUI_IS_EVENT_CONTROLLER_FOCUS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_EVENT_CONTROLLER_FOCUS))
#define BOBGUI_EVENT_CONTROLLER_FOCUS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_EVENT_CONTROLLER_FOCUS, BobguiEventControllerFocusClass))

typedef struct _BobguiEventControllerFocus BobguiEventControllerFocus;
typedef struct _BobguiEventControllerFocusClass BobguiEventControllerFocusClass;

GDK_AVAILABLE_IN_ALL
GType               bobgui_event_controller_focus_get_type  (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiEventController *bobgui_event_controller_focus_new (void);

GDK_AVAILABLE_IN_ALL
gboolean            bobgui_event_controller_focus_contains_focus     (BobguiEventControllerFocus  *self);
GDK_AVAILABLE_IN_ALL
gboolean            bobgui_event_controller_focus_is_focus           (BobguiEventControllerFocus  *self);


G_END_DECLS

