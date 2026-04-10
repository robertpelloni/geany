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

#include <bobgui/bobguienums.h>
#include <bobgui/bobguigesturedrag.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_GESTURE_PAN         (bobgui_gesture_pan_get_type ())
#define BOBGUI_GESTURE_PAN(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_GESTURE_PAN, BobguiGesturePan))
#define BOBGUI_GESTURE_PAN_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_GESTURE_PAN, BobguiGesturePanClass))
#define BOBGUI_IS_GESTURE_PAN(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_GESTURE_PAN))
#define BOBGUI_IS_GESTURE_PAN_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_GESTURE_PAN))
#define BOBGUI_GESTURE_PAN_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_GESTURE_PAN, BobguiGesturePanClass))

typedef struct _BobguiGesturePan BobguiGesturePan;
typedef struct _BobguiGesturePanClass BobguiGesturePanClass;

GDK_AVAILABLE_IN_ALL
GType             bobgui_gesture_pan_get_type        (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
BobguiGesture *      bobgui_gesture_pan_new             (BobguiOrientation  orientation);

GDK_AVAILABLE_IN_ALL
BobguiOrientation    bobgui_gesture_pan_get_orientation (BobguiGesturePan  *gesture);

GDK_AVAILABLE_IN_ALL
void              bobgui_gesture_pan_set_orientation (BobguiGesturePan  *gesture,
                                                   BobguiOrientation  orientation);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiGesturePan, g_object_unref)

G_END_DECLS

