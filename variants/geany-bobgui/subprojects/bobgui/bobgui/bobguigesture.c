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
 * BobguiGesture:
 *
 * The base class for gesture recognition.
 *
 * Although `BobguiGesture` is quite generalized to serve as a base for
 * multi-touch gestures, it is suitable to implement single-touch and
 * pointer-based gestures (using the special %NULL `GdkEventSequence`
 * value for these).
 *
 * The number of touches that a `BobguiGesture` need to be recognized is
 * controlled by the [property@Bobgui.Gesture:n-points] property, if a
 * gesture is keeping track of less or more than that number of sequences,
 * it won't check whether the gesture is recognized.
 *
 * As soon as the gesture has the expected number of touches, it will check
 * regularly if it is recognized, the criteria to consider a gesture as
 * "recognized" is left to `BobguiGesture` subclasses.
 *
 * A recognized gesture will then emit the following signals:
 *
 * - [signal@Bobgui.Gesture::begin] when the gesture is recognized.
 * - [signal@Bobgui.Gesture::update], whenever an input event is processed.
 * - [signal@Bobgui.Gesture::end] when the gesture is no longer recognized.
 *
 * ## Event propagation
 *
 * In order to receive events, a gesture needs to set a propagation phase
 * through [method@Bobgui.EventController.set_propagation_phase].
 *
 * In the capture phase, events are propagated from the toplevel down
 * to the target widget, and gestures that are attached to containers
 * above the widget get a chance to interact with the event before it
 * reaches the target.
 *
 * In the bubble phase, events are propagated up from the target widget
 * to the toplevel, and gestures that are attached to containers above
 * the widget get a chance to interact with events that have not been
 * handled yet.
 *
 * ## States of a sequence
 *
 * Whenever input interaction happens, a single event may trigger a cascade
 * of `BobguiGesture`s, both across the parents of the widget receiving the
 * event and in parallel within an individual widget. It is a responsibility
 * of the widgets using those gestures to set the state of touch sequences
 * accordingly in order to enable cooperation of gestures around the
 * `GdkEventSequence`s triggering those.
 *
 * Within a widget, gestures can be grouped through [method@Bobgui.Gesture.group].
 * Grouped gestures synchronize the state of sequences, so calling
 * [method@Bobgui.Gesture.set_state] on one will effectively propagate
 * the state throughout the group.
 *
 * By default, all sequences start out in the %BOBGUI_EVENT_SEQUENCE_NONE state,
 * sequences in this state trigger the gesture event handler, but event
 * propagation will continue unstopped by gestures.
 *
 * If a sequence enters into the %BOBGUI_EVENT_SEQUENCE_DENIED state, the gesture
 * group will effectively ignore the sequence, letting events go unstopped
 * through the gesture, but the "slot" will still remain occupied while
 * the touch is active.
 *
 * If a sequence enters in the %BOBGUI_EVENT_SEQUENCE_CLAIMED state, the gesture
 * group will grab all interaction on the sequence, by:
 *
 * - Setting the same sequence to %BOBGUI_EVENT_SEQUENCE_DENIED on every other
 *   gesture group within the widget, and every gesture on parent widgets
 *   in the propagation chain.
 * - Emitting [signal@Bobgui.Gesture::cancel] on every gesture in widgets
 *   underneath in the propagation chain.
 * - Stopping event propagation after the gesture group handles the event.
 *
 * Note: if a sequence is set early to %BOBGUI_EVENT_SEQUENCE_CLAIMED on
 * %GDK_TOUCH_BEGIN/%GDK_BUTTON_PRESS (so those events are captured before
 * reaching the event widget, this implies %BOBGUI_PHASE_CAPTURE), one similar
 * event will be emulated if the sequence changes to %BOBGUI_EVENT_SEQUENCE_DENIED.
 * This way event coherence is preserved before event propagation is unstopped
 * again.
 *
 * Sequence states can't be changed freely.
 * See [method@Bobgui.Gesture.set_state] to know about the possible
 * lifetimes of a `GdkEventSequence`.
 *
 * ## Touchpad gestures
 *
 * On the platforms that support it, `BobguiGesture` will handle transparently
 * touchpad gesture events. The only precautions users of `BobguiGesture` should
 * do to enable this support are:
 *
 * - If the gesture has %BOBGUI_PHASE_NONE, ensuring events of type
 *   %GDK_TOUCHPAD_SWIPE and %GDK_TOUCHPAD_PINCH are handled by the `BobguiGesture`
 */

#include "config.h"
#include "bobguigesture.h"
#include "bobguiwidgetprivate.h"
#include "bobguieventcontrollerprivate.h"
#include "bobguigestureprivate.h"
#include "bobguitypebuiltins.h"
#include "bobguiprivate.h"
#include "bobguimain.h"
#include "bobguimarshalers.h"
#include "bobguinative.h"

typedef struct _BobguiGesturePrivate BobguiGesturePrivate;
typedef struct _PointData PointData;

enum {
  PROP_N_POINTS = 1,
};

enum {
  BEGIN,
  END,
  UPDATE,
  CANCEL,
  SEQUENCE_STATE_CHANGED,
  N_SIGNALS
};

struct _PointData
{
  GdkEvent *event;
  BobguiWidget *target;
  double widget_x;
  double widget_y;

  /* Acummulators for touchpad events */
  double accum_dx;
  double accum_dy;

  guint press_handled : 1;
  guint state : 2;
};

struct _BobguiGesturePrivate
{
  GHashTable *points;
  GdkEventSequence *last_sequence;
  GdkDevice *device;
  GList *group_link;
  guint n_points;
  guint recognized : 1;
  guint touchpad : 1;
};

static guint signals[N_SIGNALS] = { 0 };

#define BUTTONS_MASK (GDK_BUTTON1_MASK | GDK_BUTTON2_MASK | GDK_BUTTON3_MASK)

#define EVENT_IS_TOUCHPAD_GESTURE(e) (gdk_event_get_event_type (e) == GDK_TOUCHPAD_SWIPE || \
                                      gdk_event_get_event_type (e) == GDK_TOUCHPAD_PINCH || \
                                      gdk_event_get_event_type (e) == GDK_TOUCHPAD_HOLD)

GList * _bobgui_gesture_get_group_link (BobguiGesture *gesture);

G_DEFINE_ABSTRACT_TYPE_WITH_PRIVATE (BobguiGesture, bobgui_gesture, BOBGUI_TYPE_EVENT_CONTROLLER)

static void
bobgui_gesture_get_property (GObject    *object,
                          guint       prop_id,
                          GValue     *value,
                          GParamSpec *pspec)
{
  BobguiGesturePrivate *priv = bobgui_gesture_get_instance_private (BOBGUI_GESTURE (object));

  switch (prop_id)
    {
    case PROP_N_POINTS:
      g_value_set_uint (value, priv->n_points);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_gesture_set_property (GObject      *object,
                          guint         prop_id,
                          const GValue *value,
                          GParamSpec   *pspec)
{
  BobguiGesturePrivate *priv = bobgui_gesture_get_instance_private (BOBGUI_GESTURE (object));

  switch (prop_id)
    {
    case PROP_N_POINTS:
      priv->n_points = g_value_get_uint (value);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_gesture_finalize (GObject *object)
{
  BobguiGesture *gesture = BOBGUI_GESTURE (object);
  BobguiGesturePrivate *priv = bobgui_gesture_get_instance_private (gesture);

  bobgui_gesture_ungroup (gesture);
  g_list_free (priv->group_link);

  g_hash_table_destroy (priv->points);

  G_OBJECT_CLASS (bobgui_gesture_parent_class)->finalize (object);
}

static guint
_bobgui_gesture_get_n_touchpad_points (BobguiGesture *gesture,
                                    gboolean    only_active)
{
  BobguiGesturePrivate *priv;
  PointData *data;
  GdkEventType event_type;
  GdkTouchpadGesturePhase phase = 0;
  guint n_fingers = 0;

  priv = bobgui_gesture_get_instance_private (gesture);

  if (!priv->touchpad)
    return 0;

  data = g_hash_table_lookup (priv->points, priv->last_sequence);

  if (!data)
    return 0;

  event_type = gdk_event_get_event_type (data->event);

  if (EVENT_IS_TOUCHPAD_GESTURE (data->event))
    {
      phase = gdk_touchpad_event_get_gesture_phase (data->event);
      n_fingers = gdk_touchpad_event_get_n_fingers (data->event);
    }

  if (only_active &&
      (data->state == BOBGUI_EVENT_SEQUENCE_DENIED ||
       (event_type == GDK_TOUCHPAD_SWIPE && phase == GDK_TOUCHPAD_GESTURE_PHASE_END) ||
       (event_type == GDK_TOUCHPAD_PINCH && phase == GDK_TOUCHPAD_GESTURE_PHASE_END) ||
       (event_type == GDK_TOUCHPAD_HOLD && phase == GDK_TOUCHPAD_GESTURE_PHASE_END)))
    return 0;

  return n_fingers;
}

static guint
_bobgui_gesture_get_n_touch_points (BobguiGesture *gesture,
                                 gboolean    only_active)
{
  BobguiGesturePrivate *priv;
  GHashTableIter iter;
  guint n_points = 0;
  PointData *data;
  GdkEventType event_type;

  priv = bobgui_gesture_get_instance_private (gesture);
  g_hash_table_iter_init (&iter, priv->points);

  while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &data))
    {
      event_type = gdk_event_get_event_type (data->event);

      if (only_active &&
          (data->state == BOBGUI_EVENT_SEQUENCE_DENIED ||
           event_type == GDK_TOUCH_END ||
           event_type == GDK_BUTTON_RELEASE))
        continue;

      n_points++;
    }

  return n_points;
}

static guint
_bobgui_gesture_get_n_physical_points (BobguiGesture *gesture,
                                    gboolean    only_active)
{
  BobguiGesturePrivate *priv;

  priv = bobgui_gesture_get_instance_private (gesture);

  if (priv->touchpad)
    return _bobgui_gesture_get_n_touchpad_points (gesture, only_active);
  else
    return _bobgui_gesture_get_n_touch_points (gesture, only_active);
}

static gboolean
bobgui_gesture_check_impl (BobguiGesture *gesture)
{
  BobguiGesturePrivate *priv;
  guint n_points;

  priv = bobgui_gesture_get_instance_private (gesture);
  n_points = _bobgui_gesture_get_n_physical_points (gesture, TRUE);

  return n_points == priv->n_points;
}

static void
_bobgui_gesture_set_recognized (BobguiGesture       *gesture,
                             gboolean          recognized,
                             GdkEventSequence *sequence)
{
  BobguiGesturePrivate *priv;

  priv = bobgui_gesture_get_instance_private (gesture);

  if (priv->recognized == recognized)
    return;

  priv->recognized = recognized;

  if (recognized)
    g_signal_emit (gesture, signals[BEGIN], 0, sequence);
  else
    g_signal_emit (gesture, signals[END], 0, sequence);
}

static gboolean
_bobgui_gesture_do_check (BobguiGesture *gesture)
{
  BobguiGestureClass *gesture_class;
  gboolean retval = FALSE;

  gesture_class = BOBGUI_GESTURE_GET_CLASS (gesture);

  if (!gesture_class->check)
    return retval;

  retval = gesture_class->check (gesture);
  return retval;
}

static gboolean
_bobgui_gesture_has_matching_touchpoints (BobguiGesture *gesture)
{
  BobguiGesturePrivate *priv = bobgui_gesture_get_instance_private (gesture);
  guint active_n_points, current_n_points;

  current_n_points = _bobgui_gesture_get_n_physical_points (gesture, FALSE);
  active_n_points = _bobgui_gesture_get_n_physical_points (gesture, TRUE);

  return (active_n_points == priv->n_points &&
          current_n_points == priv->n_points);
}

static gboolean
_bobgui_gesture_check_recognized (BobguiGesture       *gesture,
                               GdkEventSequence *sequence)
{
  BobguiGesturePrivate *priv = bobgui_gesture_get_instance_private (gesture);
  gboolean has_matching_touchpoints;

  has_matching_touchpoints = _bobgui_gesture_has_matching_touchpoints (gesture);

  if (priv->recognized && !has_matching_touchpoints)
    _bobgui_gesture_set_recognized (gesture, FALSE, sequence);
  else if (!priv->recognized && has_matching_touchpoints &&
           _bobgui_gesture_do_check (gesture))
    _bobgui_gesture_set_recognized (gesture, TRUE, sequence);

  return priv->recognized;
}

static void
_update_touchpad_deltas (PointData *data)
{
  GdkEvent *event = data->event;
  GdkTouchpadGesturePhase phase;
  double dx = 0;
  double dy = 0;

  if (!event)
    return;

  if (EVENT_IS_TOUCHPAD_GESTURE (event))
    {
      phase = gdk_touchpad_event_get_gesture_phase (event);

      if (gdk_event_get_event_type (event) != GDK_TOUCHPAD_HOLD)
        gdk_touchpad_event_get_deltas (event, &dx, &dy);

      if (phase == GDK_TOUCHPAD_GESTURE_PHASE_BEGIN)
        data->accum_dx = data->accum_dy = 0;
      else if (phase == GDK_TOUCHPAD_GESTURE_PHASE_UPDATE)
        {
          data->accum_dx += dx;
          data->accum_dy += dy;
        }
    }
}

static BobguiEventSequenceState
bobgui_gesture_get_group_state (BobguiGesture       *gesture,
                             GdkEventSequence *sequence)
{
  BobguiEventSequenceState state = BOBGUI_EVENT_SEQUENCE_NONE;
  GList *group_elem;

  group_elem = g_list_first (_bobgui_gesture_get_group_link (gesture));

  for (; group_elem; group_elem = group_elem->next)
    {
      if (group_elem->data == gesture)
        continue;
      if (!bobgui_gesture_handles_sequence (group_elem->data, sequence))
        continue;

      state = bobgui_gesture_get_sequence_state (group_elem->data, sequence);
      break;
    }

  return state;
}

static gboolean
_bobgui_gesture_update_point (BobguiGesture     *gesture,
                           GdkEvent       *event,
                           BobguiWidget      *target,
                           double          x,
                           double          y,
                           gboolean        add)
{
  GdkEventSequence *sequence;
  BobguiGesturePrivate *priv;
  GdkDevice *device;
  gboolean existed, touchpad;
  PointData *data;

  device = gdk_event_get_device (event);

  if (!device)
    return FALSE;

  priv = bobgui_gesture_get_instance_private (gesture);
  touchpad = EVENT_IS_TOUCHPAD_GESTURE (event);

  if (add)
    {
      /* If the event happens with the wrong device, or
       * on the wrong window, ignore.
       */
      if (priv->device && priv->device != device)
        return FALSE;

      /* Make touchpad and touchscreen gestures mutually exclusive */
      if (touchpad && g_hash_table_size (priv->points) > 0)
        return FALSE;
      else if (!touchpad && priv->touchpad)
        return FALSE;
    }
  else if (!priv->device)
    return FALSE;

  sequence = gdk_event_get_event_sequence (event);
  existed = g_hash_table_lookup_extended (priv->points, sequence,
                                          NULL, (gpointer *) &data);
  if (!existed)
    {
      if (!add)
        return FALSE;

      if (g_hash_table_size (priv->points) == 0)
        {
          priv->device = device;
          priv->touchpad = touchpad;
        }

      data = g_new0 (PointData, 1);
      g_hash_table_insert (priv->points, sequence, data);
    }

  if (data->event)
    gdk_event_unref (data->event);

  data->event = gdk_event_ref ((GdkEvent *)event);
  g_set_object (&data->target, target);
  _update_touchpad_deltas (data);
  data->widget_x = x + data->accum_dx;
  data->widget_y = y + data->accum_dy;

  if (!existed)
    {
      BobguiEventSequenceState state;

      /* Deny the sequence right away if the expected
       * number of points is exceeded, so this sequence
       * can be tracked with bobgui_gesture_handles_sequence().
       *
       * Otherwise, make the sequence inherit the same state
       * from other gestures in the same group.
       */
      if (_bobgui_gesture_get_n_physical_points (gesture, FALSE) > priv->n_points)
        state = BOBGUI_EVENT_SEQUENCE_DENIED;
      else
        state = bobgui_gesture_get_group_state (gesture, sequence);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
      bobgui_gesture_set_sequence_state (gesture, sequence, state);
G_GNUC_END_IGNORE_DEPRECATIONS
    }

  return TRUE;
}

static void
_bobgui_gesture_check_empty (BobguiGesture *gesture)
{
  BobguiGesturePrivate *priv;

  priv = bobgui_gesture_get_instance_private (gesture);

  if (g_hash_table_size (priv->points) == 0)
    {
      priv->device = NULL;
      priv->touchpad = FALSE;
    }
}

static void
_bobgui_gesture_remove_point (BobguiGesture     *gesture,
                           GdkEvent       *event)
{
  GdkEventSequence *sequence;
  BobguiGesturePrivate *priv;
  GdkDevice *device;

  sequence = gdk_event_get_event_sequence (event);
  device = gdk_event_get_device (event);
  priv = bobgui_gesture_get_instance_private (gesture);

  if (priv->device != device)
    return;

  g_hash_table_remove (priv->points, sequence);
  _bobgui_gesture_check_empty (gesture);
}

static void
_bobgui_gesture_cancel_all (BobguiGesture *gesture)
{
  GdkEventSequence *sequence;
  BobguiGesturePrivate *priv;
  GHashTableIter iter;

  priv = bobgui_gesture_get_instance_private (gesture);
  g_hash_table_iter_init (&iter, priv->points);

  while (g_hash_table_iter_next (&iter, (gpointer*) &sequence, NULL))
    {
      g_signal_emit (gesture, signals[CANCEL], 0, sequence);
      g_hash_table_iter_remove (&iter);
      _bobgui_gesture_check_recognized (gesture, sequence);
    }

  _bobgui_gesture_check_empty (gesture);
}

static gboolean
gesture_within_surface (BobguiGesture *gesture,
                        GdkSurface  *surface)
{
  BobguiWidget *widget;

  widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  return surface == bobgui_native_get_surface (bobgui_widget_get_native (widget));
}

static gboolean
bobgui_gesture_filter_event (BobguiEventController *controller,
                          GdkEvent           *event)
{
  /* Even though BobguiGesture handles these events, we want
   * touchpad gestures disabled by default, it will be
   * subclasses which punch the holes in for the events
   * they can possibly handle.
   */
  if (EVENT_IS_TOUCHPAD_GESTURE (event))
    return FALSE;

  return BOBGUI_EVENT_CONTROLLER_CLASS (bobgui_gesture_parent_class)->filter_event (controller, event);
}

static gboolean
bobgui_gesture_handle_event (BobguiEventController *controller,
                          GdkEvent           *event,
                          double              x,
                          double              y)
{
  BobguiGesture *gesture = BOBGUI_GESTURE (controller);
  GdkEventSequence *sequence;
  BobguiGesturePrivate *priv;
  GdkDevice *source_device;
  gboolean was_recognized;
  GdkEventType event_type;
  GdkTouchpadGesturePhase phase = 0;
  GdkModifierType state;
  BobguiWidget *target;

  source_device = gdk_event_get_device (event);

  if (!source_device)
    return FALSE;

  priv = bobgui_gesture_get_instance_private (gesture);
  sequence = gdk_event_get_event_sequence (event);
  was_recognized = bobgui_gesture_is_recognized (gesture);
  event_type = gdk_event_get_event_type (event);
  state = gdk_event_get_modifier_state (event);
  if (EVENT_IS_TOUCHPAD_GESTURE (event))
    phase = gdk_touchpad_event_get_gesture_phase (event);

  target = bobgui_event_controller_get_target (controller);

  if (bobgui_gesture_get_sequence_state (gesture, sequence) != BOBGUI_EVENT_SEQUENCE_DENIED)
    priv->last_sequence = sequence;

  if (event_type == GDK_BUTTON_PRESS ||
      event_type == GDK_TOUCH_BEGIN ||
      (EVENT_IS_TOUCHPAD_GESTURE (event) &&
       phase == GDK_TOUCHPAD_GESTURE_PHASE_BEGIN &&
       gdk_touchpad_event_get_n_fingers (event) == priv->n_points))
    {
      if (_bobgui_gesture_update_point (gesture, event, target, x, y, TRUE))
        {
          gboolean triggered_recognition;

          triggered_recognition =
            !was_recognized && _bobgui_gesture_has_matching_touchpoints (gesture);

          if (_bobgui_gesture_check_recognized (gesture, sequence))
            {
              PointData *data;

              data = g_hash_table_lookup (priv->points, sequence);

              /* If the sequence was claimed early, the press event will be consumed */
              if (bobgui_gesture_get_sequence_state (gesture, sequence) == BOBGUI_EVENT_SEQUENCE_CLAIMED)
                data->press_handled = TRUE;
            }
          else if (triggered_recognition && g_hash_table_size (priv->points) == 0)
            {
              /* Recognition was triggered, but the gesture reset during
               * ::begin emission. Still, recognition was strictly triggered,
               * so the event should be consumed.
               */
              return TRUE;
            }
        }
    }
  else if (event_type == GDK_BUTTON_RELEASE ||
           event_type == GDK_TOUCH_END ||
           (EVENT_IS_TOUCHPAD_GESTURE (event) &&
            phase == GDK_TOUCHPAD_GESTURE_PHASE_END &&
            gdk_touchpad_event_get_n_fingers (event) == priv->n_points))
    {
      gboolean was_claimed = FALSE;

      if (_bobgui_gesture_update_point (gesture, event, target, x, y, FALSE))
        {
          if (was_recognized &&
              _bobgui_gesture_check_recognized (gesture, sequence))
            g_signal_emit (gesture, signals[UPDATE], 0, sequence);

          was_claimed =
            bobgui_gesture_get_sequence_state (gesture, sequence) == BOBGUI_EVENT_SEQUENCE_CLAIMED;

          _bobgui_gesture_remove_point (gesture, event);
        }

      return was_claimed && was_recognized;
    }
  else if (event_type == GDK_MOTION_NOTIFY ||
           event_type == GDK_TOUCH_UPDATE ||
           (EVENT_IS_TOUCHPAD_GESTURE (event) &&
            phase == GDK_TOUCHPAD_GESTURE_PHASE_UPDATE &&
            gdk_touchpad_event_get_n_fingers (event) == priv->n_points))
    {
      if (event_type == GDK_MOTION_NOTIFY)
        {
          if ((state & BUTTONS_MASK) == 0)
            return FALSE;
        }

      if (_bobgui_gesture_update_point (gesture, event, target, x, y, FALSE) &&
          _bobgui_gesture_check_recognized (gesture, sequence))
        g_signal_emit (gesture, signals[UPDATE], 0, sequence);
    }
  else if (event_type == GDK_TOUCH_CANCEL)
    {
      if (!priv->touchpad)
        _bobgui_gesture_cancel_sequence (gesture, sequence);
    }
  else if (EVENT_IS_TOUCHPAD_GESTURE (event) &&
           phase == GDK_TOUCHPAD_GESTURE_PHASE_CANCEL &&
           gdk_touchpad_event_get_n_fingers (event) == priv->n_points)
    {
      if (priv->touchpad)
        _bobgui_gesture_cancel_sequence (gesture, sequence);
    }
  else if (event_type == GDK_GRAB_BROKEN)
    {
      GdkSurface *surface;

      surface = gdk_grab_broken_event_get_grab_surface (event);
      if (!surface || !gesture_within_surface (gesture, surface))
        _bobgui_gesture_cancel_all (gesture);

      return FALSE;
    }
  else
    {
      /* Unhandled event */
      return FALSE;
    }

  if (bobgui_gesture_get_sequence_state (gesture, sequence) != BOBGUI_EVENT_SEQUENCE_CLAIMED)
    return FALSE;

  return priv->recognized;
}

static void
bobgui_gesture_reset (BobguiEventController *controller)
{
  _bobgui_gesture_cancel_all (BOBGUI_GESTURE (controller));
}

static void
bobgui_gesture_class_init (BobguiGestureClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiEventControllerClass *controller_class = BOBGUI_EVENT_CONTROLLER_CLASS (klass);

  object_class->get_property = bobgui_gesture_get_property;
  object_class->set_property = bobgui_gesture_set_property;
  object_class->finalize = bobgui_gesture_finalize;

  controller_class->filter_event = bobgui_gesture_filter_event;
  controller_class->handle_event = bobgui_gesture_handle_event;
  controller_class->reset = bobgui_gesture_reset;

  klass->check = bobgui_gesture_check_impl;

  /**
   * BobguiGesture:n-points:
   *
   * The number of touch points that trigger
   * recognition on this gesture.
   */
  g_object_class_install_property (object_class,
                                   PROP_N_POINTS,
                                   g_param_spec_uint ("n-points", NULL, NULL,
                                                      1, G_MAXUINT, 1,
                                                      BOBGUI_PARAM_READWRITE |
                                                      G_PARAM_CONSTRUCT_ONLY));
  /**
   * BobguiGesture::begin:
   * @gesture: the object which received the signal
   * @sequence: (nullable): the `GdkEventSequence` that made the gesture
   *   to be recognized
   *
   * Emitted when the gesture is recognized.
   *
   * This means the number of touch sequences matches
   * [property@Bobgui.Gesture:n-points].
   *
   * Note: These conditions may also happen when an extra touch
   * (eg. a third touch on a 2-touches gesture) is lifted, in that
   * situation @sequence won't pertain to the current set of active
   * touches, so don't rely on this being true.
   */
  signals[BEGIN] =
    g_signal_new (I_("begin"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureClass, begin),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, GDK_TYPE_EVENT_SEQUENCE);

  /**
   * BobguiGesture::end:
   * @gesture: the object which received the signal
   * @sequence: (nullable): the `GdkEventSequence` that made gesture
   *   recognition to finish
   *
   * Emitted when @gesture either stopped recognizing the event
   * sequences as something to be handled, or the number of touch
   * sequences became higher or lower than [property@Bobgui.Gesture:n-points].
   *
   * Note: @sequence might not pertain to the group of sequences that
   * were previously triggering recognition on @gesture (ie. a just
   * pressed touch sequence that exceeds [property@Bobgui.Gesture:n-points]).
   * This situation may be detected by checking through
   * [method@Bobgui.Gesture.handles_sequence].
   */
  signals[END] =
    g_signal_new (I_("end"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureClass, end),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, GDK_TYPE_EVENT_SEQUENCE);

  /**
   * BobguiGesture::update:
   * @gesture: the object which received the signal
   * @sequence: (nullable): the `GdkEventSequence` that was updated
   *
   * Emitted whenever an event is handled while the gesture is recognized.
   *
   * @sequence is guaranteed to pertain to the set of active touches.
   */
  signals[UPDATE] =
    g_signal_new (I_("update"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureClass, update),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, GDK_TYPE_EVENT_SEQUENCE);

  /**
   * BobguiGesture::cancel:
   * @gesture: the object which received the signal
   * @sequence: (nullable): the `GdkEventSequence` that was cancelled
   *
   * Emitted whenever a sequence is cancelled.
   *
   * This usually happens on active touches when
   * [method@Bobgui.EventController.reset] is called on @gesture
   * (manually, due to grabs...), or the individual @sequence
   * was claimed by parent widgets' controllers (see
   * [method@Bobgui.Gesture.set_sequence_state]).
   *
   * @gesture must forget everything about @sequence as in
   * response to this signal.
   */
  signals[CANCEL] =
    g_signal_new (I_("cancel"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureClass, cancel),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, GDK_TYPE_EVENT_SEQUENCE);

  /**
   * BobguiGesture::sequence-state-changed:
   * @gesture: the object which received the signal
   * @sequence: (nullable): the `GdkEventSequence` that was cancelled
   * @state: the new sequence state
   *
   * Emitted whenever a sequence state changes.
   *
   * See [method@Bobgui.Gesture.set_sequence_state] to know
   * more about the expectable sequence lifetimes.
   */
  signals[SEQUENCE_STATE_CHANGED] =
    g_signal_new (I_("sequence-state-changed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureClass, sequence_state_changed),
                  NULL, NULL,
                  _bobgui_marshal_VOID__BOXED_ENUM,
                  G_TYPE_NONE, 2, GDK_TYPE_EVENT_SEQUENCE,
                  BOBGUI_TYPE_EVENT_SEQUENCE_STATE);
  g_signal_set_va_marshaller (signals[SEQUENCE_STATE_CHANGED],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__BOXED_ENUMv);
}

static void
free_point_data (gpointer data)
{
  PointData *point = data;

  if (point->event)
    gdk_event_unref (point->event);

  if (point->target)
    g_object_unref (point->target);

  g_free (point);
}

static void
bobgui_gesture_init (BobguiGesture *gesture)
{
  BobguiGesturePrivate *priv;

  priv = bobgui_gesture_get_instance_private (gesture);
  priv->points = g_hash_table_new_full (NULL, NULL, NULL,
                                        (GDestroyNotify) free_point_data);
  priv->group_link = g_list_prepend (NULL, gesture);
}

/**
 * bobgui_gesture_get_device:
 * @gesture: a `BobguiGesture`
 *
 * Returns the logical `GdkDevice` that is currently operating
 * on @gesture.
 *
 * This returns %NULL if the gesture is not being interacted.
 *
 * Returns: (nullable) (transfer none): a `GdkDevice`
 */
GdkDevice *
bobgui_gesture_get_device (BobguiGesture *gesture)
{
  BobguiGesturePrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), NULL);

  priv = bobgui_gesture_get_instance_private (gesture);

  return priv->device;
}

/**
 * bobgui_gesture_get_sequence_state:
 * @gesture: a `BobguiGesture`
 * @sequence: a `GdkEventSequence`
 *
 * Returns the @sequence state, as seen by @gesture.
 *
 * Returns: The sequence state in @gesture
 */
BobguiEventSequenceState
bobgui_gesture_get_sequence_state (BobguiGesture       *gesture,
                                GdkEventSequence *sequence)
{
  BobguiGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture),
                        BOBGUI_EVENT_SEQUENCE_NONE);

  priv = bobgui_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return BOBGUI_EVENT_SEQUENCE_NONE;

  return data->state;
}

/**
 * bobgui_gesture_set_sequence_state:
 * @gesture: a `BobguiGesture`
 * @sequence: a `GdkEventSequence`
 * @state: the sequence state
 *
 * Sets the state of @sequence in @gesture.
 *
 * Sequences start in state %BOBGUI_EVENT_SEQUENCE_NONE, and whenever
 * they change state, they can never go back to that state. Likewise,
 * sequences in state %BOBGUI_EVENT_SEQUENCE_DENIED cannot turn back to
 * a not denied state. With these rules, the lifetime of an event
 * sequence is constrained to the next four:
 *
 * * None
 * * None → Denied
 * * None → Claimed
 * * None → Claimed → Denied
 *
 * Note: Due to event handling ordering, it may be unsafe to set the
 * state on another gesture within a [signal@Bobgui.Gesture::begin] signal
 * handler, as the callback might be executed before the other gesture
 * knows about the sequence. A safe way to perform this could be:
 *
 * ```c
 * static void
 * first_gesture_begin_cb (BobguiGesture       *first_gesture,
 *                         GdkEventSequence *sequence,
 *                         gpointer          user_data)
 * {
 *   bobgui_gesture_set_sequence_state (first_gesture, sequence, BOBGUI_EVENT_SEQUENCE_CLAIMED);
 *   bobgui_gesture_set_sequence_state (second_gesture, sequence, BOBGUI_EVENT_SEQUENCE_DENIED);
 * }
 *
 * static void
 * second_gesture_begin_cb (BobguiGesture       *second_gesture,
 *                          GdkEventSequence *sequence,
 *                          gpointer          user_data)
 * {
 *   if (bobgui_gesture_get_sequence_state (first_gesture, sequence) == BOBGUI_EVENT_SEQUENCE_CLAIMED)
 *     bobgui_gesture_set_sequence_state (second_gesture, sequence, BOBGUI_EVENT_SEQUENCE_DENIED);
 * }
 * ```
 *
 * If both gestures are in the same group, just set the state on
 * the gesture emitting the event, the sequence will be already
 * be initialized to the group's global state when the second
 * gesture processes the event.
 *
 * Returns: %TRUE if @sequence is handled by @gesture,
 *   and the state is changed successfully
 *
 * Deprecated: 4.10. Use [method@Bobgui.Gesture.set_state]
 */
gboolean
bobgui_gesture_set_sequence_state (BobguiGesture            *gesture,
                                GdkEventSequence      *sequence,
                                BobguiEventSequenceState  state)
{
  BobguiGesturePrivate *priv;
  BobguiWidget *widget;
  PointData *data;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);
  g_return_val_if_fail (state >= BOBGUI_EVENT_SEQUENCE_NONE &&
                        state <= BOBGUI_EVENT_SEQUENCE_DENIED, FALSE);

  priv = bobgui_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return FALSE;

  if (data->state == state)
    return FALSE;

  /* denied sequences remain denied */
  if (data->state == BOBGUI_EVENT_SEQUENCE_DENIED)
    return FALSE;

  /* Sequences can't go from claimed/denied to none */
  if (state == BOBGUI_EVENT_SEQUENCE_NONE &&
      data->state != BOBGUI_EVENT_SEQUENCE_NONE)
    return FALSE;

  data->state = state;

  widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  bobgui_widget_propagate_event_sequence_state (widget, gesture, sequence, state);
  g_signal_emit (gesture, signals[SEQUENCE_STATE_CHANGED], 0,
                 sequence, state);

  if (state == BOBGUI_EVENT_SEQUENCE_DENIED)
    _bobgui_gesture_check_recognized (gesture, sequence);

  return TRUE;
}

/**
 * bobgui_gesture_set_state:
 * @gesture: a `BobguiGesture`
 * @state: the sequence state
 *
 * Sets the state of all sequences that @gesture is currently
 * interacting with.
 *
 * Sequences start in state %BOBGUI_EVENT_SEQUENCE_NONE, and whenever
 * they change state, they can never go back to that state. Likewise,
 * sequences in state %BOBGUI_EVENT_SEQUENCE_DENIED cannot turn back to
 * a not denied state. With these rules, the lifetime of an event
 * sequence is constrained to the next four:
 *
 * * None
 * * None → Denied
 * * None → Claimed
 * * None → Claimed → Denied
 *
 * Note: Due to event handling ordering, it may be unsafe to set the
 * state on another gesture within a [signal@Bobgui.Gesture::begin] signal
 * handler, as the callback might be executed before the other gesture
 * knows about the sequence. A safe way to perform this could be:
 *
 * ```c
 * static void
 * first_gesture_begin_cb (BobguiGesture       *first_gesture,
 *                         GdkEventSequence *sequence,
 *                         gpointer          user_data)
 * {
 *   bobgui_gesture_set_state (first_gesture, BOBGUI_EVENT_SEQUENCE_CLAIMED);
 *   bobgui_gesture_set_state (second_gesture, BOBGUI_EVENT_SEQUENCE_DENIED);
 * }
 *
 * static void
 * second_gesture_begin_cb (BobguiGesture       *second_gesture,
 *                          GdkEventSequence *sequence,
 *                          gpointer          user_data)
 * {
 *   if (bobgui_gesture_get_sequence_state (first_gesture, sequence) == BOBGUI_EVENT_SEQUENCE_CLAIMED)
 *     bobgui_gesture_set_state (second_gesture, BOBGUI_EVENT_SEQUENCE_DENIED);
 * }
 * ```
 *
 * If both gestures are in the same group, just set the state on
 * the gesture emitting the event, the sequence will be already
 * be initialized to the group's global state when the second
 * gesture processes the event.
 *
 * Returns: %TRUE if the state of at least one sequence
 *   was changed successfully
 */
gboolean
bobgui_gesture_set_state (BobguiGesture            *gesture,
                       BobguiEventSequenceState  state)
{
  gboolean handled = FALSE;
  BobguiGesturePrivate *priv;
  GList *sequences, *l;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);
  g_return_val_if_fail (state >= BOBGUI_EVENT_SEQUENCE_NONE &&
                        state <= BOBGUI_EVENT_SEQUENCE_DENIED, FALSE);

  priv = bobgui_gesture_get_instance_private (gesture);
  sequences = g_hash_table_get_keys (priv->points);

G_GNUC_BEGIN_IGNORE_DEPRECATIONS
  for (l = sequences; l; l = l->next)
    handled |= bobgui_gesture_set_sequence_state (gesture, l->data, state);
G_GNUC_END_IGNORE_DEPRECATIONS

  g_list_free (sequences);

  return handled;
}

/**
 * bobgui_gesture_get_sequences:
 * @gesture: a `BobguiGesture`
 *
 * Returns the list of `GdkEventSequences` currently being interpreted
 * by @gesture.
 *
 * Returns: (transfer container) (element-type GdkEventSequence): A list
 *   of `GdkEventSequence`, the list elements are owned by BOBGUI and must
 *   not be freed or modified, the list itself must be deleted
 *   through g_list_free()
 */
GList *
bobgui_gesture_get_sequences (BobguiGesture *gesture)
{
  GdkEventSequence *sequence;
  BobguiGesturePrivate *priv;
  GList *sequences = NULL;
  GHashTableIter iter;
  PointData *data;
  GdkEventType event_type;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), NULL);

  priv = bobgui_gesture_get_instance_private (gesture);
  g_hash_table_iter_init (&iter, priv->points);

  while (g_hash_table_iter_next (&iter, (gpointer *) &sequence, (gpointer *) &data))
    {
      if (data->state == BOBGUI_EVENT_SEQUENCE_DENIED)
        continue;

      event_type = gdk_event_get_event_type (data->event);

      if (event_type == GDK_TOUCH_END ||
          event_type == GDK_BUTTON_RELEASE)
        continue;

      sequences = g_list_prepend (sequences, sequence);
    }

  return sequences;
}

/**
 * bobgui_gesture_get_last_updated_sequence:
 * @gesture: a `BobguiGesture`
 *
 * Returns the `GdkEventSequence` that was last updated on @gesture.
 *
 * Returns: (transfer none) (nullable): The last updated sequence
 */
GdkEventSequence *
bobgui_gesture_get_last_updated_sequence (BobguiGesture *gesture)
{
  BobguiGesturePrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), NULL);

  priv = bobgui_gesture_get_instance_private (gesture);

  return priv->last_sequence;
}

/**
 * bobgui_gesture_get_last_event:
 * @gesture: a `BobguiGesture`
 * @sequence: (nullable): a `GdkEventSequence`
 *
 * Returns the last event that was processed for @sequence.
 *
 * Note that the returned pointer is only valid as long as the
 * @sequence is still interpreted by the @gesture. If in doubt,
 * you should make a copy of the event.
 *
 * Returns: (transfer none) (nullable): The last event from @sequence
 */
GdkEvent *
bobgui_gesture_get_last_event (BobguiGesture       *gesture,
                            GdkEventSequence *sequence)
{
  BobguiGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), NULL);

  priv = bobgui_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return NULL;

  return data->event;
}

/*
 * bobgui_gesture_get_last_target:
 * @gesture: a `BobguiGesture`
 * @sequence: event sequence
 *
 * Returns the widget that the last event was targeted at.
 *
 * See [method@Bobgui.Gesture.get_last_event].
 *
 * Returns: (transfer none) (nullable): The target of the last event
 */
BobguiWidget *
bobgui_gesture_get_last_target (BobguiGesture        *gesture,
                             GdkEventSequence  *sequence)
{
  BobguiGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), NULL);

  priv = bobgui_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return NULL;

  return data->target;
}

/**
 * bobgui_gesture_get_point:
 * @gesture: a `BobguiGesture`
 * @sequence: (nullable): a `GdkEventSequence`, or %NULL for pointer events
 * @x: (out) (optional): return location for X axis of the sequence coordinates
 * @y: (out) (optional): return location for Y axis of the sequence coordinates
 *
 * If @sequence is currently being interpreted by @gesture,
 * returns %TRUE and fills in @x and @y with the last coordinates
 * stored for that event sequence.
 *
 * The coordinates are always relative to the widget allocation.
 *
 * Returns: %TRUE if @sequence is currently interpreted
 */
gboolean
bobgui_gesture_get_point (BobguiGesture       *gesture,
                       GdkEventSequence *sequence,
                       double           *x,
                       double           *y)
{
  BobguiGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);

  priv = bobgui_gesture_get_instance_private (gesture);

  if (!g_hash_table_lookup_extended (priv->points, sequence,
                                     NULL, (gpointer *) &data))
    return FALSE;

  if (x)
    *x = data->widget_x;
  if (y)
    *y = data->widget_y;

  return TRUE;
}

gboolean
_bobgui_gesture_get_last_update_time (BobguiGesture       *gesture,
                                   GdkEventSequence *sequence,
                                   guint32          *evtime)
{
  BobguiGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);

  priv = bobgui_gesture_get_instance_private (gesture);

  if (!g_hash_table_lookup_extended (priv->points, sequence,
                                     NULL, (gpointer *) &data))
    return FALSE;

  if (evtime)
    *evtime = gdk_event_get_time (data->event);

  return TRUE;
};

/**
 * bobgui_gesture_get_bounding_box:
 * @gesture: a `BobguiGesture`
 * @rect: (out): bounding box containing all active touches.
 *
 * If there are touch sequences being currently handled by @gesture,
 * returns %TRUE and fills in @rect with the bounding box containing
 * all active touches.
 *
 * Otherwise, %FALSE will be returned.
 *
 * Note: This function will yield unexpected results on touchpad
 * gestures. Since there is no correlation between physical and
 * pixel distances, these will look as if constrained in an
 * infinitely small area, @rect width and height will thus be 0
 * regardless of the number of touchpoints.
 *
 * Returns: %TRUE if there are active touches, %FALSE otherwise
 */
gboolean
bobgui_gesture_get_bounding_box (BobguiGesture   *gesture,
                              GdkRectangle *rect)
{
  BobguiGesturePrivate *priv;
  double x1, y1, x2, y2;
  GHashTableIter iter;
  guint n_points = 0;
  PointData *data;
  GdkEventType event_type;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);
  g_return_val_if_fail (rect != NULL, FALSE);

  priv = bobgui_gesture_get_instance_private (gesture);
  x1 = y1 = G_MAXDOUBLE;
  x2 = y2 = -G_MAXDOUBLE;

  g_hash_table_iter_init (&iter, priv->points);

  while (g_hash_table_iter_next (&iter, NULL, (gpointer *) &data))
    {
      if (data->state == BOBGUI_EVENT_SEQUENCE_DENIED)
        continue;

      event_type = gdk_event_get_event_type (data->event);

      if (event_type == GDK_TOUCH_END ||
          event_type == GDK_BUTTON_RELEASE)
        continue;

      n_points++;
      x1 = MIN (x1, data->widget_x);
      y1 = MIN (y1, data->widget_y);
      x2 = MAX (x2, data->widget_x);
      y2 = MAX (y2, data->widget_y);
    }

  if (n_points == 0)
    return FALSE;

  rect->x = x1;
  rect->y = y1;
  rect->width = x2 - x1;
  rect->height = y2 - y1;

  return TRUE;
}


/**
 * bobgui_gesture_get_bounding_box_center:
 * @gesture: a `BobguiGesture`
 * @x: (out): X coordinate for the bounding box center
 * @y: (out): Y coordinate for the bounding box center
 *
 * If there are touch sequences being currently handled by @gesture,
 * returns %TRUE and fills in @x and @y with the center of the bounding
 * box containing all active touches.
 *
 * Otherwise, %FALSE will be returned.
 *
 * Returns: %FALSE if no active touches are present, %TRUE otherwise
 */
gboolean
bobgui_gesture_get_bounding_box_center (BobguiGesture *gesture,
                                     double     *x,
                                     double     *y)
{
  GdkEvent *last_event;
  GdkRectangle rect;
  GdkEventSequence *sequence;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);
  g_return_val_if_fail (x != NULL && y != NULL, FALSE);

  sequence = bobgui_gesture_get_last_updated_sequence (gesture);
  last_event = bobgui_gesture_get_last_event (gesture, sequence);

  if (EVENT_IS_TOUCHPAD_GESTURE (last_event))
    return bobgui_gesture_get_point (gesture, sequence, x, y);
  else if (!bobgui_gesture_get_bounding_box (gesture, &rect))
    return FALSE;

  *x = rect.x + rect.width / 2;
  *y = rect.y + rect.height / 2;
  return TRUE;
}

/**
 * bobgui_gesture_is_active:
 * @gesture: a `BobguiGesture`
 *
 * Returns %TRUE if the gesture is currently active.
 *
 * A gesture is active while there are touch sequences
 * interacting with it.
 *
 * Returns: %TRUE if gesture is active
 */
gboolean
bobgui_gesture_is_active (BobguiGesture *gesture)
{
  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);

  return _bobgui_gesture_get_n_physical_points (gesture, TRUE) != 0;
}

/**
 * bobgui_gesture_is_recognized:
 * @gesture: a `BobguiGesture`
 *
 * Returns %TRUE if the gesture is currently recognized.
 *
 * A gesture is recognized if there are as many interacting
 * touch sequences as required by @gesture.
 *
 * Returns: %TRUE if gesture is recognized
 */
gboolean
bobgui_gesture_is_recognized (BobguiGesture *gesture)
{
  BobguiGesturePrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);

  priv = bobgui_gesture_get_instance_private (gesture);

  return priv->recognized;
}

gboolean
_bobgui_gesture_check (BobguiGesture *gesture)
{
  BobguiGesturePrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);

  priv = bobgui_gesture_get_instance_private (gesture);

  return _bobgui_gesture_check_recognized (gesture, priv->last_sequence);
}

/**
 * bobgui_gesture_handles_sequence:
 * @gesture: a `BobguiGesture`
 * @sequence: (nullable): a `GdkEventSequence`
 *
 * Returns %TRUE if @gesture is currently handling events
 * corresponding to @sequence.
 *
 * Returns: %TRUE if @gesture is handling @sequence, %FALSE otherwise
 */
gboolean
bobgui_gesture_handles_sequence (BobguiGesture       *gesture,
                              GdkEventSequence *sequence)
{
  BobguiGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);

  priv = bobgui_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return FALSE;

  if (data->state == BOBGUI_EVENT_SEQUENCE_DENIED)
    return FALSE;

  return TRUE;
}

gboolean
_bobgui_gesture_cancel_sequence (BobguiGesture       *gesture,
                              GdkEventSequence *sequence)
{
  BobguiGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);

  priv = bobgui_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return FALSE;

  g_signal_emit (gesture, signals[CANCEL], 0, sequence);
  _bobgui_gesture_remove_point (gesture, data->event);
  _bobgui_gesture_check_recognized (gesture, sequence);

  return TRUE;
}

GList *
_bobgui_gesture_get_group_link (BobguiGesture *gesture)
{
  BobguiGesturePrivate *priv;

  priv = bobgui_gesture_get_instance_private (gesture);

  return priv->group_link;
}

/**
 * bobgui_gesture_group:
 * @gesture: a `BobguiGesture`
 * @group_gesture: `BobguiGesture` to group @gesture with
 *
 * Adds @gesture to the same group than @group_gesture.
 *
 * Gestures are by default isolated in their own groups.
 *
 * Both gestures must have been added to the same widget before
 * they can be grouped.
 *
 * When gestures are grouped, the state of `GdkEventSequences`
 * is kept in sync for all of those, so calling
 * [method@Bobgui.Gesture.set_sequence_state], on one will transfer
 * the same value to the others.
 *
 * Groups also perform an "implicit grabbing" of sequences, if a
 * `GdkEventSequence` state is set to %BOBGUI_EVENT_SEQUENCE_CLAIMED
 * on one group, every other gesture group attached to the same
 * `BobguiWidget` will switch the state for that sequence to
 * %BOBGUI_EVENT_SEQUENCE_DENIED.
 */
void
bobgui_gesture_group (BobguiGesture *gesture,
                   BobguiGesture *group_gesture)
{
  GList *link, *group_link, *next;

  g_return_if_fail (BOBGUI_IS_GESTURE (gesture));
  g_return_if_fail (BOBGUI_IS_GESTURE (group_gesture));
  g_return_if_fail (bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (group_gesture)) ==
                    bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture)));

  link = _bobgui_gesture_get_group_link (gesture);

  if (link->prev || link->next)
    {
      if (bobgui_gesture_is_grouped_with (gesture, group_gesture))
        return;
      bobgui_gesture_ungroup (gesture);
    }

  group_link = _bobgui_gesture_get_group_link (group_gesture);
  next = group_link->next;

  /* Rewire link so it's inserted after the group_gesture elem */
  link->prev = group_link;
  link->next = next;
  group_link->next = link;
  if (next)
    next->prev = link;
}

/**
 * bobgui_gesture_ungroup:
 * @gesture: a `BobguiGesture`
 *
 * Separates @gesture into an isolated group.
 */
void
bobgui_gesture_ungroup (BobguiGesture *gesture)
{
  GList *link, *prev, *next;

  g_return_if_fail (BOBGUI_IS_GESTURE (gesture));

  link = _bobgui_gesture_get_group_link (gesture);
  prev = link->prev;
  next = link->next;

  /* Detach link from the group chain */
  if (prev)
    prev->next = next;
  if (next)
    next->prev = prev;

  link->next = link->prev = NULL;
}

/**
 * bobgui_gesture_get_group:
 * @gesture: a `BobguiGesture`
 *
 * Returns all gestures in the group of @gesture
 *
 * Returns: (element-type BobguiGesture) (transfer container): The list
 *   of `BobguiGesture`s, free with g_list_free()
 */
GList *
bobgui_gesture_get_group (BobguiGesture *gesture)
{
  GList *link;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), NULL);

  link = _bobgui_gesture_get_group_link (gesture);

  return g_list_copy (g_list_first (link));
}

/**
 * bobgui_gesture_is_grouped_with:
 * @gesture: a `BobguiGesture`
 * @other: another `BobguiGesture`
 *
 * Returns %TRUE if both gestures pertain to the same group.
 *
 * Returns: whether the gestures are grouped
 */
gboolean
bobgui_gesture_is_grouped_with (BobguiGesture *gesture,
                             BobguiGesture *other)
{
  GList *link;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);
  g_return_val_if_fail (BOBGUI_IS_GESTURE (other), FALSE);

  link = _bobgui_gesture_get_group_link (gesture);
  link = g_list_first (link);

  return g_list_find (link, other) != NULL;
}

gboolean
_bobgui_gesture_handled_sequence_press (BobguiGesture       *gesture,
                                     GdkEventSequence *sequence)
{
  BobguiGesturePrivate *priv;
  PointData *data;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);

  priv = bobgui_gesture_get_instance_private (gesture);
  data = g_hash_table_lookup (priv->points, sequence);

  if (!data)
    return FALSE;

  return data->press_handled;
}

gboolean
_bobgui_gesture_get_pointer_emulating_sequence (BobguiGesture        *gesture,
                                             GdkEventSequence **sequence)
{
  BobguiGesturePrivate *priv;
  GdkEventSequence *seq;
  GHashTableIter iter;
  PointData *data;

  g_return_val_if_fail (BOBGUI_IS_GESTURE (gesture), FALSE);

  priv = bobgui_gesture_get_instance_private (gesture);
  g_hash_table_iter_init (&iter, priv->points);

  while (g_hash_table_iter_next (&iter, (gpointer*) &seq, (gpointer*) &data))
    {
      switch ((guint) gdk_event_get_event_type (data->event))
        {
        case GDK_TOUCH_BEGIN:
        case GDK_TOUCH_UPDATE:
        case GDK_TOUCH_END:
          if (!gdk_touch_event_get_emulating_pointer (data->event))
            continue;
          G_GNUC_FALLTHROUGH;
        case GDK_BUTTON_PRESS:
        case GDK_BUTTON_RELEASE:
        case GDK_MOTION_NOTIFY:
          *sequence = seq;
          return TRUE;
        default:
          break;
        }
    }

  return FALSE;
}
