/*
 * Copyright © 2020 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <gdk/gdk.h>
#include <bobgui/bobguieventcontroller.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_DROP_CONTROLLER_MOTION         (bobgui_drop_controller_motion_get_type ())
#define BOBGUI_DROP_CONTROLLER_MOTION(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_DROP_CONTROLLER_MOTION, BobguiDropControllerMotion))
#define BOBGUI_DROP_CONTROLLER_MOTION_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_DROP_CONTROLLER_MOTION, BobguiDropControllerMotionClass))
#define BOBGUI_IS_DROP_CONTROLLER_MOTION(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_DROP_CONTROLLER_MOTION))
#define BOBGUI_IS_DROP_CONTROLLER_MOTION_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_DROP_CONTROLLER_MOTION))
#define BOBGUI_DROP_CONTROLLER_MOTION_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_DROP_CONTROLLER_MOTION, BobguiDropControllerMotionClass))

typedef struct _BobguiDropControllerMotion BobguiDropControllerMotion;
typedef struct _BobguiDropControllerMotionClass BobguiDropControllerMotionClass;

GDK_AVAILABLE_IN_ALL
GType                   bobgui_drop_controller_motion_get_type             (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiEventController *    bobgui_drop_controller_motion_new                  (void);

GDK_AVAILABLE_IN_ALL
gboolean                bobgui_drop_controller_motion_contains_pointer     (BobguiDropControllerMotion        *self);
GDK_AVAILABLE_IN_ALL
GdkDrop *               bobgui_drop_controller_motion_get_drop             (BobguiDropControllerMotion        *self);
GDK_AVAILABLE_IN_ALL
gboolean                bobgui_drop_controller_motion_is_pointer           (BobguiDropControllerMotion        *self);

G_END_DECLS

