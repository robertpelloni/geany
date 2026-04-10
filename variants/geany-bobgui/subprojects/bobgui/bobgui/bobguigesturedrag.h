/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2014, Red Hat, Inc.
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

#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguigesturesingle.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_GESTURE_DRAG         (bobgui_gesture_drag_get_type ())
#define BOBGUI_GESTURE_DRAG(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_GESTURE_DRAG, BobguiGestureDrag))
#define BOBGUI_GESTURE_DRAG_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_GESTURE_DRAG, BobguiGestureDragClass))
#define BOBGUI_IS_GESTURE_DRAG(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_GESTURE_DRAG))
#define BOBGUI_IS_GESTURE_DRAG_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_GESTURE_DRAG))
#define BOBGUI_GESTURE_DRAG_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_GESTURE_DRAG, BobguiGestureDragClass))

typedef struct _BobguiGestureDrag BobguiGestureDrag;
typedef struct _BobguiGestureDragClass BobguiGestureDragClass;

GDK_AVAILABLE_IN_ALL
GType        bobgui_gesture_drag_get_type          (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiGesture * bobgui_gesture_drag_new               (void);

GDK_AVAILABLE_IN_ALL
gboolean     bobgui_gesture_drag_get_start_point   (BobguiGestureDrag *gesture,
                                                 double         *x,
                                                 double         *y);
GDK_AVAILABLE_IN_ALL
gboolean     bobgui_gesture_drag_get_offset        (BobguiGestureDrag *gesture,
                                                 double         *x,
                                                 double         *y);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiGestureDrag, g_object_unref)

G_END_DECLS

