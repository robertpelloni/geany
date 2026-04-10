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
 * Author(s): Matthias Clasen <mclasen@redhat.com>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguieventcontroller.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_EVENT_CONTROLLER_MOTION         (bobgui_event_controller_motion_get_type ())
#define BOBGUI_EVENT_CONTROLLER_MOTION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_EVENT_CONTROLLER_MOTION, BobguiEventControllerMotion))
#define BOBGUI_EVENT_CONTROLLER_MOTION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_EVENT_CONTROLLER_MOTION, BobguiEventControllerMotionClass))
#define BOBGUI_IS_EVENT_CONTROLLER_MOTION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_EVENT_CONTROLLER_MOTION))
#define BOBGUI_IS_EVENT_CONTROLLER_MOTION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_EVENT_CONTROLLER_MOTION))
#define BOBGUI_EVENT_CONTROLLER_MOTION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_EVENT_CONTROLLER_MOTION, BobguiEventControllerMotionClass))

typedef struct _BobguiEventControllerMotion BobguiEventControllerMotion;
typedef struct _BobguiEventControllerMotionClass BobguiEventControllerMotionClass;

GDK_AVAILABLE_IN_ALL
GType               bobgui_event_controller_motion_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiEventController *bobgui_event_controller_motion_new      (void);

GDK_AVAILABLE_IN_ALL
gboolean            bobgui_event_controller_motion_contains_pointer   (BobguiEventControllerMotion *self);
GDK_AVAILABLE_IN_ALL
gboolean            bobgui_event_controller_motion_is_pointer         (BobguiEventControllerMotion *self);

G_END_DECLS

