/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2012, One Laptop Per Child.
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

#include <bobgui/bobguigesture.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_GESTURE_SINGLE         (bobgui_gesture_single_get_type ())
#define BOBGUI_GESTURE_SINGLE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_GESTURE_SINGLE, BobguiGestureSingle))
#define BOBGUI_GESTURE_SINGLE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_GESTURE_SINGLE, BobguiGestureSingleClass))
#define BOBGUI_IS_GESTURE_SINGLE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_GESTURE_SINGLE))
#define BOBGUI_IS_GESTURE_SINGLE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_GESTURE_SINGLE))
#define BOBGUI_GESTURE_SINGLE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_GESTURE_SINGLE, BobguiGestureSingleClass))

typedef struct _BobguiGestureSingle BobguiGestureSingle;
typedef struct _BobguiGestureSingleClass BobguiGestureSingleClass;

GDK_AVAILABLE_IN_ALL
GType       bobgui_gesture_single_get_type       (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
gboolean    bobgui_gesture_single_get_touch_only (BobguiGestureSingle *gesture);

GDK_AVAILABLE_IN_ALL
void        bobgui_gesture_single_set_touch_only (BobguiGestureSingle *gesture,
                                               gboolean          touch_only);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_gesture_single_get_exclusive  (BobguiGestureSingle *gesture);

GDK_AVAILABLE_IN_ALL
void        bobgui_gesture_single_set_exclusive  (BobguiGestureSingle *gesture,
                                               gboolean          exclusive);

GDK_AVAILABLE_IN_ALL
guint       bobgui_gesture_single_get_button     (BobguiGestureSingle *gesture);

GDK_AVAILABLE_IN_ALL
void        bobgui_gesture_single_set_button     (BobguiGestureSingle *gesture,
                                               guint             button);

GDK_AVAILABLE_IN_ALL
guint       bobgui_gesture_single_get_current_button
                                              (BobguiGestureSingle *gesture);

GDK_AVAILABLE_IN_ALL
GdkEventSequence * bobgui_gesture_single_get_current_sequence
                                              (BobguiGestureSingle *gesture);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiGestureSingle, g_object_unref)

G_END_DECLS

