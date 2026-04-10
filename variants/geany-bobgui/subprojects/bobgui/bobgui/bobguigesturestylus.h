/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2017-2018, Red Hat, Inc.
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

#include <bobgui/bobguigesture.h>

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

G_BEGIN_DECLS

#define BOBGUI_TYPE_GESTURE_STYLUS         (bobgui_gesture_stylus_get_type ())
#define BOBGUI_GESTURE_STYLUS(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_GESTURE_STYLUS, BobguiGestureStylus))
#define BOBGUI_GESTURE_STYLUS_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_GESTURE_STYLUS, BobguiGestureStylusClass))
#define BOBGUI_IS_GESTURE_STYLUS(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_GESTURE_STYLUS))
#define BOBGUI_IS_GESTURE_STYLUS_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_GESTURE_STYLUS))
#define BOBGUI_GESTURE_STYLUS_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_GESTURE_STYLUS, BobguiGestureStylusClass))

typedef struct _BobguiGestureStylus BobguiGestureStylus;
typedef struct _BobguiGestureStylusClass BobguiGestureStylusClass;

GDK_AVAILABLE_IN_ALL
GType             bobgui_gesture_stylus_get_type (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiGesture *      bobgui_gesture_stylus_new      (void);

GDK_AVAILABLE_IN_4_10
gboolean          bobgui_gesture_stylus_get_stylus_only (BobguiGestureStylus *gesture);
GDK_AVAILABLE_IN_4_10
void              bobgui_gesture_stylus_set_stylus_only (BobguiGestureStylus *gesture,
						      gboolean          stylus_only);

GDK_AVAILABLE_IN_ALL
gboolean          bobgui_gesture_stylus_get_axis (BobguiGestureStylus *gesture,
					       GdkAxisUse        axis,
					       double           *value);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_gesture_stylus_get_axes (BobguiGestureStylus  *gesture,
					       GdkAxisUse         axes[],
					       double           **values);
GDK_AVAILABLE_IN_ALL
gboolean          bobgui_gesture_stylus_get_backlog (BobguiGestureStylus  *gesture,
						  GdkTimeCoord     **backlog,
						  guint             *n_elems);
GDK_AVAILABLE_IN_ALL
GdkDeviceTool *   bobgui_gesture_stylus_get_device_tool (BobguiGestureStylus *gesture);

G_END_DECLS

