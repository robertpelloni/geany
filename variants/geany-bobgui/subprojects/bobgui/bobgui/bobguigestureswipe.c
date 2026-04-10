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

/**
 * BobguiGestureSwipe:
 *
 * Recognizes swipe gestures.
 *
 * After a press/move/.../move/release sequence happens, the
 * [signal@Bobgui.GestureSwipe::swipe] signal will be emitted,
 * providing the velocity and directionality of the sequence
 * at the time it was lifted.
 *
 * If the velocity is desired in intermediate points,
 * [method@Bobgui.GestureSwipe.get_velocity] can be called in a
 * [signal@Bobgui.Gesture::update] handler.
 *
 * All velocities are reported in pixels/sec units.
 */

#include "config.h"
#include "bobguigestureswipe.h"
#include "bobguigestureswipeprivate.h"
#include "bobguigestureprivate.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"

#define CAPTURE_THRESHOLD_MS 150

typedef struct _BobguiGestureSwipePrivate BobguiGestureSwipePrivate;
typedef struct _EventData EventData;

struct _EventData
{
  guint32 evtime;
  int x;
  int y;
};

struct _BobguiGestureSwipePrivate
{
  GArray *events;
};

enum {
  SWIPE,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (BobguiGestureSwipe, bobgui_gesture_swipe, BOBGUI_TYPE_GESTURE_SINGLE)

static void
bobgui_gesture_swipe_finalize (GObject *object)
{
  BobguiGestureSwipePrivate *priv;

  priv = bobgui_gesture_swipe_get_instance_private (BOBGUI_GESTURE_SWIPE (object));
  g_array_free (priv->events, TRUE);

  G_OBJECT_CLASS (bobgui_gesture_swipe_parent_class)->finalize (object);
}

static gboolean
bobgui_gesture_swipe_filter_event (BobguiEventController *controller,
                                GdkEvent           *event)
{
  /* Let touchpad swipe and hold events go through, only if they match n-points */
  if (gdk_event_get_event_type (event) == GDK_TOUCHPAD_SWIPE ||
      gdk_event_get_event_type (event) == GDK_TOUCHPAD_HOLD)
    {
      guint n_points;
      guint n_fingers;

      g_object_get (G_OBJECT (controller), "n-points", &n_points, NULL);

      n_fingers = gdk_touchpad_event_get_n_fingers (event);

      if (n_fingers == n_points)
        return FALSE;
      else
        return TRUE;
    }

  return BOBGUI_EVENT_CONTROLLER_CLASS (bobgui_gesture_swipe_parent_class)->filter_event (controller, event);
}

static void
_bobgui_gesture_swipe_clear_backlog (BobguiGestureSwipe *gesture,
                                  guint32          evtime)
{
  BobguiGestureSwipePrivate *priv;
  int i, length = 0;

  priv = bobgui_gesture_swipe_get_instance_private (gesture);

  for (i = 0; i < (int) priv->events->len; i++)
    {
      EventData *data;

      data = &g_array_index (priv->events, EventData, i);

      if (data->evtime >= evtime - CAPTURE_THRESHOLD_MS)
        {
          length = i - 1;
          break;
        }
    }

  if (length > 0)
    g_array_remove_range (priv->events, 0, length);
}

static void
bobgui_gesture_swipe_append_event (BobguiGestureSwipe  *swipe,
                                GdkEventSequence *sequence)
{
  BobguiGestureSwipePrivate *priv;
  EventData new;
  double x, y;

  priv = bobgui_gesture_swipe_get_instance_private (swipe);
  _bobgui_gesture_get_last_update_time (BOBGUI_GESTURE (swipe), sequence, &new.evtime);
  bobgui_gesture_get_point (BOBGUI_GESTURE (swipe), sequence, &x, &y);

  new.x = x;
  new.y = y;

  _bobgui_gesture_swipe_clear_backlog (swipe, new.evtime);
  g_array_append_val (priv->events, new);
}

static void
bobgui_gesture_swipe_update (BobguiGesture       *gesture,
                          GdkEventSequence *sequence)
{
  BobguiGestureSwipe *swipe = BOBGUI_GESTURE_SWIPE (gesture);

  bobgui_gesture_swipe_append_event (swipe, sequence);
}

static void
_bobgui_gesture_swipe_calculate_velocity (BobguiGestureSwipe *gesture,
                                       double          *velocity_x,
                                       double          *velocity_y)
{
  BobguiGestureSwipePrivate *priv;
  GdkEventSequence *sequence;
  guint32 evtime, diff_time;
  EventData *start, *end;
  double diff_x, diff_y;

  priv = bobgui_gesture_swipe_get_instance_private (gesture);
  *velocity_x = *velocity_y = 0;

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  _bobgui_gesture_get_last_update_time (BOBGUI_GESTURE (gesture), sequence, &evtime);
  _bobgui_gesture_swipe_clear_backlog (gesture, evtime);

  if (priv->events->len == 0)
    return;

  start = &g_array_index (priv->events, EventData, 0);
  end = &g_array_index (priv->events, EventData, priv->events->len - 1);

  diff_time = end->evtime - start->evtime;
  diff_x = end->x - start->x;
  diff_y = end->y - start->y;

  if (diff_time == 0)
    return;

  /* Velocity in pixels/sec */
  *velocity_x = diff_x * 1000 / diff_time;
  *velocity_y = diff_y * 1000 / diff_time;
}

static void
bobgui_gesture_swipe_end (BobguiGesture       *gesture,
                       GdkEventSequence *sequence)
{
  BobguiGestureSwipe *swipe = BOBGUI_GESTURE_SWIPE (gesture);
  BobguiGestureSwipePrivate *priv;
  double velocity_x, velocity_y;
  GdkEventSequence *seq;

  seq = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  if (bobgui_gesture_get_sequence_state (gesture, seq) == BOBGUI_EVENT_SEQUENCE_DENIED)
    return;

  if (bobgui_gesture_is_active (gesture))
    return;

  bobgui_gesture_swipe_append_event (swipe, sequence);

  priv = bobgui_gesture_swipe_get_instance_private (swipe);
  _bobgui_gesture_swipe_calculate_velocity (swipe, &velocity_x, &velocity_y);
  g_signal_emit (gesture, signals[SWIPE], 0, velocity_x, velocity_y);

  if (priv->events->len > 0)
    g_array_remove_range (priv->events, 0, priv->events->len);
}

static void
bobgui_gesture_swipe_class_init (BobguiGestureSwipeClass *klass)
{
  BobguiGestureClass *gesture_class = BOBGUI_GESTURE_CLASS (klass);
  BobguiEventControllerClass *event_controller_class = BOBGUI_EVENT_CONTROLLER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bobgui_gesture_swipe_finalize;

  event_controller_class->filter_event = bobgui_gesture_swipe_filter_event;

  gesture_class->update = bobgui_gesture_swipe_update;
  gesture_class->end = bobgui_gesture_swipe_end;

  /**
   * BobguiGestureSwipe::swipe:
   * @gesture: object which received the signal
   * @velocity_x: velocity in the X axis, in pixels/sec
   * @velocity_y: velocity in the Y axis, in pixels/sec
   *
   * Emitted when the recognized gesture is finished.
   *
   * Velocity and direction are a product of previously recorded events.
   */
  signals[SWIPE] =
    g_signal_new (I_("swipe"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureSwipeClass, swipe),
                  NULL, NULL,
                  _bobgui_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[SWIPE],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__DOUBLE_DOUBLEv);
}

static void
bobgui_gesture_swipe_init (BobguiGestureSwipe *gesture)
{
  BobguiGestureSwipePrivate *priv;

  priv = bobgui_gesture_swipe_get_instance_private (gesture);
  priv->events = g_array_new (FALSE, FALSE, sizeof (EventData));
}

/**
 * bobgui_gesture_swipe_new:
 *
 * Returns a newly created `BobguiGesture` that recognizes swipes.
 *
 * Returns: a newly created `BobguiGestureSwipe`
 */
BobguiGesture *
bobgui_gesture_swipe_new (void)
{
  return g_object_new (BOBGUI_TYPE_GESTURE_SWIPE,
                       NULL);
}

/**
 * bobgui_gesture_swipe_get_velocity:
 * @gesture: a `BobguiGestureSwipe`
 * @velocity_x: (out): return value for the velocity in the X axis, in pixels/sec
 * @velocity_y: (out): return value for the velocity in the Y axis, in pixels/sec
 *
 * Gets the current velocity.
 *
 * If the gesture is recognized, this function returns %TRUE and fills
 * in @velocity_x and @velocity_y with the recorded velocity, as per the
 * last events processed.
 *
 * Returns: whether velocity could be calculated
 */
gboolean
bobgui_gesture_swipe_get_velocity (BobguiGestureSwipe *gesture,
                                double          *velocity_x,
                                double          *velocity_y)
{
  double vel_x, vel_y;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);

  if (!bobgui_gesture_is_recognized (BOBGUI_GESTURE (gesture)))
    return FALSE;

  _bobgui_gesture_swipe_calculate_velocity (gesture, &vel_x, &vel_y);

  if (velocity_x)
    *velocity_x = vel_x;
  if (velocity_y)
    *velocity_y = vel_y;

  return TRUE;
}
