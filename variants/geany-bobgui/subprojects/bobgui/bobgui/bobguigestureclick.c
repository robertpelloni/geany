/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2014 Red Hat, Inc.
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
 * BobguiGestureClick:
 *
 * Recognizes click gestures.
 *
 * It is able to recognize multiple clicks on a nearby zone, which
 * can be listened for through the [signal@Bobgui.GestureClick::pressed]
 * signal. Whenever time or distance between clicks exceed the BOBGUI
 * defaults, [signal@Bobgui.GestureClick::stopped] is emitted, and the
 * click counter is reset.
 */

#include "config.h"
#include "bobguigestureprivate.h"
#include "bobguigestureclick.h"
#include "bobguigestureclickprivate.h"
#include "bobguiprivate.h"
#include "bobguimarshalers.h"

typedef struct _BobguiGestureClickPrivate BobguiGestureClickPrivate;

struct _BobguiGestureClickPrivate
{
  GdkDevice *current_device;
  double initial_press_x;
  double initial_press_y;
  guint double_click_timeout_id;
  guint n_presses;
  guint n_release;
  guint current_button;
};

enum {
  PRESSED,
  RELEASED,
  STOPPED,
  UNPAIRED_RELEASE,
  LAST_SIGNAL
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (BobguiGestureClick, bobgui_gesture_click, BOBGUI_TYPE_GESTURE_SINGLE)

static void
bobgui_gesture_click_finalize (GObject *object)
{
  BobguiGestureClickPrivate *priv;
  BobguiGestureClick *gesture;

  gesture = BOBGUI_GESTURE_CLICK (object);
  priv = bobgui_gesture_click_get_instance_private (gesture);

  if (priv->double_click_timeout_id)
    {
      g_source_remove (priv->double_click_timeout_id);
      priv->double_click_timeout_id = 0;
    }

  G_OBJECT_CLASS (bobgui_gesture_click_parent_class)->finalize (object);
}

static gboolean
bobgui_gesture_click_check (BobguiGesture *gesture)
{
  BobguiGestureClick *click;
  BobguiGestureClickPrivate *priv;
  GList *sequences;
  gboolean active;

  click = BOBGUI_GESTURE_CLICK (gesture);
  priv = bobgui_gesture_click_get_instance_private (click);
  sequences = bobgui_gesture_get_sequences (gesture);

  active = g_list_length (sequences) == 1 || priv->double_click_timeout_id;
  g_list_free (sequences);

  return active;
}

static void
_bobgui_gesture_click_stop (BobguiGestureClick *gesture)
{
  BobguiGestureClickPrivate *priv;

  priv = bobgui_gesture_click_get_instance_private (gesture);

  if (priv->n_presses == 0)
    return;

  priv->current_device = NULL;
  priv->current_button = 0;
  priv->n_presses = 0;
  g_signal_emit (gesture, signals[STOPPED], 0);
  _bobgui_gesture_check (BOBGUI_GESTURE (gesture));
}

static gboolean
_double_click_timeout_cb (gpointer user_data)
{
  BobguiGestureClick *gesture = user_data;
  BobguiGestureClickPrivate *priv;

  priv = bobgui_gesture_click_get_instance_private (gesture);
  priv->double_click_timeout_id = 0;
  _bobgui_gesture_click_stop (gesture);

  return FALSE;
}

static void
_bobgui_gesture_click_update_timeout (BobguiGestureClick *gesture)
{
  BobguiGestureClickPrivate *priv;
  guint double_click_time;
  BobguiSettings *settings;
  BobguiWidget *widget;

  priv = bobgui_gesture_click_get_instance_private (gesture);

  if (priv->double_click_timeout_id)
    g_source_remove (priv->double_click_timeout_id);

  widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  settings = bobgui_widget_get_settings (widget);
  g_object_get (settings, "bobgui-double-click-time", &double_click_time, NULL);

  priv->double_click_timeout_id = g_timeout_add (double_click_time, _double_click_timeout_cb, gesture);
  gdk_source_set_static_name_by_id (priv->double_click_timeout_id, "[bobgui] _double_click_timeout_cb");
}

static gboolean
_bobgui_gesture_click_check_within_threshold (BobguiGestureClick *gesture,
                                           const char      *setting,
                                           double           x,
                                           double           y)
{
  BobguiGestureClickPrivate *priv;
  guint double_click_distance;
  BobguiSettings *settings;
  BobguiWidget *widget;

  priv = bobgui_gesture_click_get_instance_private (gesture);

  if (priv->n_presses == 0)
    return TRUE;

  widget = bobgui_event_controller_get_widget (BOBGUI_EVENT_CONTROLLER (gesture));
  settings = bobgui_widget_get_settings (widget);
  g_object_get (settings, setting, &double_click_distance, NULL);

  if (ABS (priv->initial_press_x - x) < double_click_distance &&
      ABS (priv->initial_press_y - y) < double_click_distance)
    return TRUE;

  return FALSE;
}

static void
bobgui_gesture_click_begin (BobguiGesture       *gesture,
                         GdkEventSequence *sequence)
{
  BobguiGestureClick *click;
  BobguiGestureClickPrivate *priv;
  guint n_presses, button = 1;
  GdkEventSequence *current;
  GdkEvent *event;
  GdkEventType event_type;
  GdkDevice *device;
  double x, y;

  if (!bobgui_gesture_handles_sequence (gesture, sequence))
    return;

  click = BOBGUI_GESTURE_CLICK (gesture);
  priv = bobgui_gesture_click_get_instance_private (click);
  event = bobgui_gesture_get_last_event (gesture, sequence);
  current = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  device = gdk_event_get_device (event);
  event_type = gdk_event_get_event_type (event);

  if (event_type == GDK_BUTTON_PRESS)
    button = gdk_button_event_get_button (event);
  else if (event_type == GDK_TOUCH_BEGIN)
    button = 1;
  else
    return;

  /* Reset the gesture if the button number changes mid-recognition */
  if (priv->n_presses > 0 &&
      priv->current_button != button)
    _bobgui_gesture_click_stop (click);

  /* Reset also if the device changed */
  if (priv->current_device && priv->current_device != device)
    _bobgui_gesture_click_stop (click);

  priv->current_device = device;
  priv->current_button = button;
  _bobgui_gesture_click_update_timeout (click);
  bobgui_gesture_get_point (gesture, current, &x, &y);

  if (gdk_device_get_source (priv->current_device) == GDK_SOURCE_MOUSE &&
      !_bobgui_gesture_click_check_within_threshold (click, "bobgui-double-click-distance", x, y))
    _bobgui_gesture_click_stop (click);

  /* Increment later the real counter, just if the gesture is
   * reset on the pressed handler */
  n_presses = priv->n_release = priv->n_presses + 1;

  g_signal_emit (gesture, signals[PRESSED], 0, n_presses, x, y);

  if (priv->n_presses == 0)
    {
      priv->initial_press_x = x;
      priv->initial_press_y = y;
    }

  priv->n_presses++;
}

static void
bobgui_gesture_click_update (BobguiGesture       *gesture,
                          GdkEventSequence *sequence)
{
  BobguiGestureClick *click;
  GdkEventSequence *current;
  double x, y;

  click = BOBGUI_GESTURE_CLICK (gesture);
  current = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  bobgui_gesture_get_point (gesture, current, &x, &y);

  if (!_bobgui_gesture_click_check_within_threshold (click, "bobgui-dnd-drag-threshold", x, y))
    _bobgui_gesture_click_stop (click);
}

static void
bobgui_gesture_click_end (BobguiGesture       *gesture,
                       GdkEventSequence *sequence)
{
  BobguiGestureClick *click;
  BobguiGestureClickPrivate *priv;
  GdkEventSequence *current;
  double x, y;
  gboolean interpreted;
  BobguiEventSequenceState state;

  click = BOBGUI_GESTURE_CLICK (gesture);
  priv = bobgui_gesture_click_get_instance_private (click);
  current = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));
  interpreted = bobgui_gesture_get_point (gesture, current, &x, &y);
  state = bobgui_gesture_get_sequence_state (gesture, current);

  if (current == sequence && state != BOBGUI_EVENT_SEQUENCE_DENIED && interpreted)
    g_signal_emit (gesture, signals[RELEASED], 0, priv->n_release, x, y);

  priv->n_release = 0;
}

static void
bobgui_gesture_click_cancel (BobguiGesture       *gesture,
                          GdkEventSequence *sequence)
{
  _bobgui_gesture_click_stop (BOBGUI_GESTURE_CLICK (gesture));
  BOBGUI_GESTURE_CLASS (bobgui_gesture_click_parent_class)->cancel (gesture, sequence);
}

static void
bobgui_gesture_click_reset (BobguiEventController *controller)
{
  _bobgui_gesture_click_stop (BOBGUI_GESTURE_CLICK (controller));
  BOBGUI_EVENT_CONTROLLER_CLASS (bobgui_gesture_click_parent_class)->reset (controller);
}

static gboolean
bobgui_gesture_click_handle_event (BobguiEventController *controller,
                                GdkEvent           *event,
                                double              x,
                                double              y)
{
  BobguiEventControllerClass *parent_controller;
  BobguiGestureClickPrivate *priv;
  GdkEventSequence *sequence;
  guint button;

  priv = bobgui_gesture_click_get_instance_private (BOBGUI_GESTURE_CLICK (controller));
  parent_controller = BOBGUI_EVENT_CONTROLLER_CLASS (bobgui_gesture_click_parent_class);
  sequence = gdk_event_get_event_sequence (event);

  if (priv->n_presses == 0 &&
      !bobgui_gesture_handles_sequence (BOBGUI_GESTURE (controller), sequence) &&
      (gdk_event_get_event_type (event) == GDK_BUTTON_RELEASE ||
       gdk_event_get_event_type (event) == GDK_TOUCH_END))
    {
      if (gdk_event_get_event_type (event) == GDK_BUTTON_RELEASE)
        button = gdk_button_event_get_button (event);
      else
        button = 0;
      g_signal_emit (controller, signals[UNPAIRED_RELEASE], 0,
                     x, y, button, sequence);
    }

  return parent_controller->handle_event (controller, event, x, y);
}

static void
bobgui_gesture_click_class_init (BobguiGestureClickClass *klass)
{
  BobguiEventControllerClass *controller_class = BOBGUI_EVENT_CONTROLLER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiGestureClass *gesture_class = BOBGUI_GESTURE_CLASS (klass);

  object_class->finalize = bobgui_gesture_click_finalize;

  gesture_class->check = bobgui_gesture_click_check;
  gesture_class->begin = bobgui_gesture_click_begin;
  gesture_class->update = bobgui_gesture_click_update;
  gesture_class->end = bobgui_gesture_click_end;
  gesture_class->cancel = bobgui_gesture_click_cancel;

  controller_class->reset = bobgui_gesture_click_reset;
  controller_class->handle_event = bobgui_gesture_click_handle_event;

  /**
   * BobguiGestureClick::pressed:
   * @gesture: the object which received the signal
   * @n_press: how many touch/button presses happened with this one
   * @x: The X coordinate, in widget allocation coordinates
   * @y: The Y coordinate, in widget allocation coordinates
   *
   * Emitted whenever a button or touch press happens.
   */
  signals[PRESSED] =
    g_signal_new (I_("pressed"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureClickClass, pressed),
                  NULL, NULL,
                  _bobgui_marshal_VOID__INT_DOUBLE_DOUBLE,
                  G_TYPE_NONE, 3, G_TYPE_INT,
                  G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[PRESSED],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__INT_DOUBLE_DOUBLEv);

  /**
   * BobguiGestureClick::released:
   * @gesture: the object which received the signal
   * @n_press: number of press that is paired with this release
   * @x: The X coordinate, in widget allocation coordinates
   * @y: The Y coordinate, in widget allocation coordinates
   *
   * Emitted when a button or touch is released.
   *
   * @n_press will report the number of press that is paired to
   * this event, note that [signal@Bobgui.GestureClick::stopped] may
   * have been emitted between the press and its release, @n_press
   * will only start over at the next press.
   */
  signals[RELEASED] =
    g_signal_new (I_("released"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureClickClass, released),
                  NULL, NULL,
                  _bobgui_marshal_VOID__INT_DOUBLE_DOUBLE,
                  G_TYPE_NONE, 3, G_TYPE_INT,
                  G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[RELEASED],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__INT_DOUBLE_DOUBLEv);

  /**
   * BobguiGestureClick::stopped:
   * @gesture: the object which received the signal
   *
   * Emitted whenever any time/distance threshold has been exceeded.
   */
  signals[STOPPED] =
    g_signal_new (I_("stopped"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureClickClass, stopped),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiGestureClick::unpaired-release
   * @gesture: the object which received the signal
   * @x: X coordinate of the event
   * @y: Y coordinate of the event
   * @button: Button being released
   * @sequence: (nullable): Sequence being released
   *
   * Emitted whenever the gesture receives a release
   * event that had no previous corresponding press.
   *
   * Due to implicit grabs, this can only happen on situations
   * where input is grabbed elsewhere mid-press or the pressed
   * widget voluntarily relinquishes its implicit grab.
   */
  signals[UNPAIRED_RELEASE] =
    g_signal_new (I_("unpaired-release"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _bobgui_marshal_VOID__DOUBLE_DOUBLE_UINT_BOXED,
                  G_TYPE_NONE, 4,
                  G_TYPE_DOUBLE, G_TYPE_DOUBLE,
                  G_TYPE_UINT, GDK_TYPE_EVENT_SEQUENCE);
  g_signal_set_va_marshaller (signals[UNPAIRED_RELEASE],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__DOUBLE_DOUBLE_UINT_BOXEDv);
}

static void
bobgui_gesture_click_init (BobguiGestureClick *gesture)
{
}

/**
 * bobgui_gesture_click_new:
 *
 * Returns a newly created `BobguiGesture` that recognizes
 * single and multiple presses.
 *
 * Returns: a newly created `BobguiGestureClick`
 **/
BobguiGesture *
bobgui_gesture_click_new (void)
{
  return g_object_new (BOBGUI_TYPE_GESTURE_CLICK, NULL);
}
