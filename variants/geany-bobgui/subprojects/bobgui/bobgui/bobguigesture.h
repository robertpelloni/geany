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

#include <bobgui/bobguieventcontroller.h>
#include <bobgui/bobguienums.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_GESTURE         (bobgui_gesture_get_type ())
#define BOBGUI_GESTURE(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_GESTURE, BobguiGesture))
#define BOBGUI_GESTURE_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_GESTURE, BobguiGestureClass))
#define BOBGUI_IS_GESTURE(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_GESTURE))
#define BOBGUI_IS_GESTURE_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_GESTURE))
#define BOBGUI_GESTURE_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_GESTURE, BobguiGestureClass))

typedef struct _BobguiGestureClass BobguiGestureClass;

GDK_AVAILABLE_IN_ALL
GType       bobgui_gesture_get_type             (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GdkDevice * bobgui_gesture_get_device           (BobguiGesture       *gesture);

GDK_AVAILABLE_IN_ALL
gboolean    bobgui_gesture_set_state            (BobguiGesture            *gesture,
                                              BobguiEventSequenceState  state);
GDK_AVAILABLE_IN_ALL
BobguiEventSequenceState
            bobgui_gesture_get_sequence_state   (BobguiGesture            *gesture,
                                              GdkEventSequence      *sequence);
GDK_DEPRECATED_IN_4_10
gboolean    bobgui_gesture_set_sequence_state   (BobguiGesture            *gesture,
                                              GdkEventSequence      *sequence,
                                              BobguiEventSequenceState  state);
GDK_AVAILABLE_IN_ALL
GList     * bobgui_gesture_get_sequences        (BobguiGesture       *gesture);

GDK_AVAILABLE_IN_ALL
GdkEventSequence * bobgui_gesture_get_last_updated_sequence
                                             (BobguiGesture       *gesture);

GDK_AVAILABLE_IN_ALL
gboolean    bobgui_gesture_handles_sequence     (BobguiGesture       *gesture,
                                              GdkEventSequence *sequence);
GDK_AVAILABLE_IN_ALL
GdkEvent *
            bobgui_gesture_get_last_event       (BobguiGesture       *gesture,
                                              GdkEventSequence *sequence);

GDK_AVAILABLE_IN_ALL
gboolean    bobgui_gesture_get_point            (BobguiGesture       *gesture,
                                              GdkEventSequence *sequence,
                                              double           *x,
                                              double           *y);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_gesture_get_bounding_box     (BobguiGesture       *gesture,
                                              GdkRectangle     *rect);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_gesture_get_bounding_box_center
                                             (BobguiGesture       *gesture,
                                              double           *x,
                                              double           *y);
GDK_AVAILABLE_IN_ALL
gboolean    bobgui_gesture_is_active            (BobguiGesture       *gesture);

GDK_AVAILABLE_IN_ALL
gboolean    bobgui_gesture_is_recognized        (BobguiGesture       *gesture);

GDK_AVAILABLE_IN_ALL
void        bobgui_gesture_group                (BobguiGesture       *group_gesture,
                                              BobguiGesture       *gesture);
GDK_AVAILABLE_IN_ALL
void        bobgui_gesture_ungroup              (BobguiGesture       *gesture);

GDK_AVAILABLE_IN_ALL
GList *     bobgui_gesture_get_group            (BobguiGesture       *gesture);

GDK_AVAILABLE_IN_ALL
gboolean    bobgui_gesture_is_grouped_with      (BobguiGesture       *gesture,
                                              BobguiGesture       *other);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiGesture, g_object_unref)

G_END_DECLS

