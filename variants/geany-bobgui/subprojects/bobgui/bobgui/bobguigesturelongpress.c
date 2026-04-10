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
 * BobguiGestureLongPress:
 *
 * Recognizes long press gestures.
 *
 * This gesture is also known as “Press and Hold”.
 *
 * When the timeout is exceeded, the gesture is triggering the
 * [signal@Bobgui.GestureLongPress::pressed] signal.
 *
 * If the touchpoint is lifted before the timeout passes, or if
 * it drifts too far of the initial press point, the
 * [signal@Bobgui.GestureLongPress::cancelled] signal will be emitted.
 *
 * How long the timeout is before the ::pressed signal gets emitted is
 * determined by the [property@Bobgui.Settings:bobgui-long-press-time] setting.
 * It can be modified by the [property@Bobgui.GestureLongPress:delay-factor]
 * property.
 */

#include "config.h"
#include "bobguigesturelongpress.h"
#include "bobguigesturelongpressprivate.h"
#include "bobguigestureprivate.h"
#include "bobguimarshalers.h"
#include "bobguidragsourceprivate.h"
#include "bobguiprivate.h"
#include "bobguimarshalers.h"

typedef struct _BobguiGestureLongPressPrivate BobguiGestureLongPressPrivate;

enum {
  PRESSED,
  CANCELLED,
  N_SIGNALS
};

enum {
  PROP_DELAY_FACTOR = 1,
  LAST_PROP
};

struct _BobguiGestureLongPressPrivate
{
  double initial_x;
  double initial_y;

  double delay_factor;
  guint timeout_id;
  guint delay;
  guint cancelled : 1;
  guint triggered : 1;
};

static guint signals[N_SIGNALS] = { 0 };
static GParamSpec *props[LAST_PROP] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (BobguiGestureLongPress, bobgui_gesture_long_press, BOBGUI_TYPE_GESTURE_SINGLE)

static void
bobgui_gesture_long_press_init (BobguiGestureLongPress *gesture)
{
  BobguiGestureLongPressPrivate *priv = bobgui_gesture_long_press_get_instance_private (gesture);
  priv->delay_factor = 1.0;
}

static gboolean
bobgui_gesture_long_press_check (BobguiGesture *gesture)
{
  BobguiGestureLongPressPrivate *priv;

  priv = bobgui_gesture_long_press_get_instance_private (BOBGUI_GESTURE_LONG_PRESS (gesture));

  if (priv->cancelled)
    return FALSE;

  return BOBGUI_GESTURE_CLASS (bobgui_gesture_long_press_parent_class)->check (gesture);
}

static gboolean
_bobgui_gesture_long_press_timeout (gpointer user_data)
{
  BobguiGestureLongPress *gesture = user_data;
  BobguiGestureLongPressPrivate *priv;
  GdkEventSequence *sequence;
  double x, y;

  priv = bobgui_gesture_long_press_get_instance_private (gesture);
  sequence = bobgui_gesture_get_last_updated_sequence (BOBGUI_GESTURE (gesture));
  bobgui_gesture_get_point (BOBGUI_GESTURE (gesture), sequence, &x, &y);

  priv->timeout_id = 0;
  priv->triggered = TRUE;
  g_signal_emit (gesture, signals[PRESSED], 0, x, y);

  return G_SOURCE_REMOVE;
}

static void
bobgui_gesture_long_press_begin (BobguiGesture       *gesture,
                              GdkEventSequence *sequence)
{
  BobguiGestureLongPressPrivate *priv;
  GdkEvent *event;
  GdkEventType event_type;
  BobguiWidget *widget;
  int delay;

  priv = bobgui_gesture_long_press_get_instance_private (BOBGUI_GESTURE_LONG_PRESS (gesture));
  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  event = bobgui_gesture_get_last_event (gesture, sequence);

  if (!event)
    return;

  event_type = gdk_event_get_event_type (event);

  if (event_type != GDK_BUTTON_PRESS &&
      event_type != GDK_TOUCH_BEGIN)
    return;

  widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  g_object_get (bobgui_widget_get_settings (widget),
                "bobgui-long-press-time", &delay,
                NULL);

  delay = (int)(priv->delay_factor * delay);

  bobgui_gesture_get_point (gesture, sequence,
                         &priv->initial_x, &priv->initial_y);
  priv->timeout_id = g_timeout_add (delay, _bobgui_gesture_long_press_timeout, gesture);
  gdk_source_set_static_name_by_id (priv->timeout_id, "[bobgui] _bobgui_gesture_long_press_timeout");
}

static void
bobgui_gesture_long_press_update (BobguiGesture       *gesture,
                               GdkEventSequence *sequence)
{
  BobguiGestureLongPressPrivate *priv;
  BobguiWidget *widget;
  double x, y;

  widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  priv = bobgui_gesture_long_press_get_instance_private (BOBGUI_GESTURE_LONG_PRESS (gesture));
  bobgui_gesture_get_point (gesture, sequence, &x, &y);

  if (bobgui_drag_check_threshold_double (widget, priv->initial_x, priv->initial_y, x, y))
    {
      if (priv->timeout_id)
        {
          g_source_remove (priv->timeout_id);
          priv->timeout_id = 0;
          g_signal_emit (gesture, signals[CANCELLED], 0);
        }

      priv->cancelled = TRUE;
      _bobgui_gesture_check (gesture);
    }
}

static void
bobgui_gesture_long_press_end (BobguiGesture       *gesture,
                            GdkEventSequence *sequence)
{
  BobguiGestureLongPressPrivate *priv;

  priv = bobgui_gesture_long_press_get_instance_private (BOBGUI_GESTURE_LONG_PRESS (gesture));

  if (priv->timeout_id)
    {
      g_source_remove (priv->timeout_id);
      priv->timeout_id = 0;
      g_signal_emit (gesture, signals[CANCELLED], 0);
    }

  priv->cancelled = priv->triggered = FALSE;
}

static void
bobgui_gesture_long_press_cancel (BobguiGesture       *gesture,
                               GdkEventSequence *sequence)
{
  bobgui_gesture_long_press_end (gesture, sequence);
  BOBGUI_GESTURE_CLASS (bobgui_gesture_long_press_parent_class)->cancel (gesture, sequence);
}

static void
bobgui_gesture_long_press_sequence_state_changed (BobguiGesture            *gesture,
                                               GdkEventSequence      *sequence,
                                               BobguiEventSequenceState  state)
{
  if (state == BOBGUI_EVENT_SEQUENCE_DENIED)
    bobgui_gesture_long_press_end (gesture, sequence);
}

static void
bobgui_gesture_long_press_finalize (GObject *object)
{
  BobguiGestureLongPressPrivate *priv;

  priv = bobgui_gesture_long_press_get_instance_private (BOBGUI_GESTURE_LONG_PRESS (object));

  if (priv->timeout_id)
    g_source_remove (priv->timeout_id);

  G_OBJECT_CLASS (bobgui_gesture_long_press_parent_class)->finalize (object);
}

static void
bobgui_gesture_long_press_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  switch (property_id)
    {
    case PROP_DELAY_FACTOR:
      g_value_set_double (value, bobgui_gesture_long_press_get_delay_factor (BOBGUI_GESTURE_LONG_PRESS (object)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_gesture_long_press_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  switch (property_id)
    {
    case PROP_DELAY_FACTOR:
      bobgui_gesture_long_press_set_delay_factor (BOBGUI_GESTURE_LONG_PRESS (object),
                                               g_value_get_double (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_gesture_long_press_class_init (BobguiGestureLongPressClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiGestureClass *gesture_class = BOBGUI_GESTURE_CLASS (klass);

  object_class->finalize = bobgui_gesture_long_press_finalize;
  object_class->get_property = bobgui_gesture_long_press_get_property;
  object_class->set_property = bobgui_gesture_long_press_set_property;

  gesture_class->check = bobgui_gesture_long_press_check;
  gesture_class->begin = bobgui_gesture_long_press_begin;
  gesture_class->update = bobgui_gesture_long_press_update;
  gesture_class->end = bobgui_gesture_long_press_end;
  gesture_class->cancel = bobgui_gesture_long_press_cancel;
  gesture_class->sequence_state_changed = bobgui_gesture_long_press_sequence_state_changed;

  /**
   * BobguiGestureLongPress:delay-factor:
   *
   * Factor by which to modify the default timeout.
   */
  props[PROP_DELAY_FACTOR] =
    g_param_spec_double ("delay-factor", NULL, NULL,
                         0.5, 2.0, 1.0,
                         G_PARAM_READWRITE | G_PARAM_STATIC_STRINGS | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, props);

  /**
   * BobguiGestureLongPress::pressed:
   * @gesture: the object which received the signal
   * @x: the X coordinate where the press happened, relative to the widget allocation
   * @y: the Y coordinate where the press happened, relative to the widget allocation
   *
   * Emitted whenever a press goes unmoved/unreleased longer than
   * what the BOBGUI defaults tell.
   */
  signals[PRESSED] =
    g_signal_new (I_("pressed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureLongPressClass, pressed),
                  NULL, NULL,
                  _bobgui_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[PRESSED],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__DOUBLE_DOUBLEv);

  /**
   * BobguiGestureLongPress::cancelled:
   * @gesture: the object which received the signal
   *
   * Emitted whenever a press moved too far, or was released
   * before [signal@Bobgui.GestureLongPress::pressed] happened.
   */
  signals[CANCELLED] =
    g_signal_new (I_("cancelled"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureLongPressClass, cancelled),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);
}

/**
 * bobgui_gesture_long_press_new:
 *
 * Returns a newly created `BobguiGesture` that recognizes long presses.
 *
 * Returns: a newly created `BobguiGestureLongPress`.
 */
BobguiGesture *
bobgui_gesture_long_press_new (void)
{
  return g_object_new (BOBGUI_TYPE_GESTURE_LONG_PRESS,
                       NULL);
}

/**
 * bobgui_gesture_long_press_set_delay_factor:
 * @gesture: A `BobguiGestureLongPress`
 * @delay_factor: The delay factor to apply
 *
 * Applies the given delay factor.
 *
 * The default long press time will be multiplied by this value.
 * Valid values are in the range [0.5..2.0].
 */
void
bobgui_gesture_long_press_set_delay_factor (BobguiGestureLongPress *gesture,
                                         double               delay_factor)
{
  BobguiGestureLongPressPrivate *priv = bobgui_gesture_long_press_get_instance_private (gesture);

  g_return_if_fail (BOBGUI_IS_GESTURE_LONG_PRESS (gesture));
  g_return_if_fail (delay_factor >= 0.5);
  g_return_if_fail (delay_factor <= 2.0);

  if (delay_factor == priv->delay_factor)
    return;

  priv->delay_factor = delay_factor;

  g_object_notify_by_pspec (G_OBJECT (gesture), props[PROP_DELAY_FACTOR]);
}

/**
 * bobgui_gesture_long_press_get_delay_factor:
 * @gesture: A `BobguiGestureLongPress`
 *
 * Returns the delay factor.
 *
 * Returns: the delay factor
 */
double
bobgui_gesture_long_press_get_delay_factor (BobguiGestureLongPress *gesture)
{
  BobguiGestureLongPressPrivate *priv = bobgui_gesture_long_press_get_instance_private (gesture);

  g_return_val_if_fail (BOBGUI_IS_GESTURE_LONG_PRESS (gesture), 0);

  return priv->delay_factor;
}
