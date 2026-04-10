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

#include "bobguieventcontrollerprivate.h"
#include "bobguigesture.h"

struct _BobguiGesture
{
  BobguiEventController parent_instance;
};

struct _BobguiGestureClass
{
  BobguiEventControllerClass parent_class;

  gboolean (* check)  (BobguiGesture       *gesture);

  void     (* begin)  (BobguiGesture       *gesture,
                       GdkEventSequence *sequence);
  void     (* update) (BobguiGesture       *gesture,
                       GdkEventSequence *sequence);
  void     (* end)    (BobguiGesture       *gesture,
                       GdkEventSequence *sequence);

  void     (* cancel) (BobguiGesture       *gesture,
                       GdkEventSequence *sequence);

  void     (* sequence_state_changed) (BobguiGesture            *gesture,
                                       GdkEventSequence      *sequence,
                                       BobguiEventSequenceState  state);

  /*< private >*/
  gpointer padding[10];
};


G_BEGIN_DECLS

gboolean _bobgui_gesture_check                  (BobguiGesture       *gesture);

gboolean _bobgui_gesture_handled_sequence_press (BobguiGesture       *gesture,
                                              GdkEventSequence *sequence);

gboolean _bobgui_gesture_get_pointer_emulating_sequence
                                                (BobguiGesture        *gesture,
                                                 GdkEventSequence **sequence);

gboolean _bobgui_gesture_cancel_sequence        (BobguiGesture       *gesture,
                                              GdkEventSequence *sequence);

gboolean _bobgui_gesture_get_last_update_time   (BobguiGesture       *gesture,
                                              GdkEventSequence *sequence,
                                              guint32          *evtime);

BobguiWidget  *bobgui_gesture_get_last_target      (BobguiGesture       *gesture,
                                              GdkEventSequence *sequence);


G_END_DECLS

