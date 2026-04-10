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

/**
 * BobguiGestureDrag:
 *
 * Recognizes drag gestures.
 *
 * The drag operation itself can be tracked throughout the
 * [signal@Bobgui.GestureDrag::drag-begin],
 * [signal@Bobgui.GestureDrag::drag-update] and
 * [signal@Bobgui.GestureDrag::drag-end] signals, and the relevant
 * coordinates can be extracted through
 * [method@Bobgui.GestureDrag.get_offset] and
 * [method@Bobgui.GestureDrag.get_start_point].
 */
#include "config.h"
#include "bobguigesturedrag.h"
#include "bobguigesturedragprivate.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"

typedef struct _BobguiGestureDragPrivate BobguiGestureDragPrivate;
typedef struct _EventData EventData;

struct _BobguiGestureDragPrivate
{
  double start_x;
  double start_y;
  double last_x;
  double last_y;
};

enum {
  DRAG_BEGIN,
  DRAG_UPDATE,
  DRAG_END,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (BobguiGestureDrag, bobgui_gesture_drag, BOBGUI_TYPE_GESTURE_SINGLE)

static gboolean
bobgui_gesture_drag_filter_event (BobguiEventController *controller,
                               GdkEvent           *event)
{
  /* Let touchpad swipe events go through, only if they match n-points  */
  if (gdk_event_get_event_type (event) == GDK_TOUCHPAD_SWIPE)
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

  return BOBGUI_EVENT_CONTROLLER_CLASS (bobgui_gesture_drag_parent_class)->filter_event (controller, event);
}

static void
bobgui_gesture_drag_begin (BobguiGesture       *gesture,
                        GdkEventSequence *sequence)
{
  BobguiGestureDragPrivate *priv;
  GdkEventSequence *current;

  current = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  priv = bobgui_gesture_drag_get_instance_private (BOBGUI_GESTURE_DRAG (gesture));
  bobgui_gesture_get_point (gesture, current, &priv->start_x, &priv->start_y);
  priv->last_x = priv->start_x;
  priv->last_y = priv->start_y;

  g_signal_emit (gesture, signals[DRAG_BEGIN], 0, priv->start_x, priv->start_y);
}

static void
bobgui_gesture_drag_update (BobguiGesture       *gesture,
                         GdkEventSequence *sequence)
{
  BobguiGestureDragPrivate *priv;
  double x, y;

  priv = bobgui_gesture_drag_get_instance_private (BOBGUI_GESTURE_DRAG (gesture));
  bobgui_gesture_get_point (gesture, sequence, &priv->last_x, &priv->last_y);
  x = priv->last_x - priv->start_x;
  y = priv->last_y - priv->start_y;

  g_signal_emit (gesture, signals[DRAG_UPDATE], 0, x, y);
}

static void
bobgui_gesture_drag_end (BobguiGesture       *gesture,
                      GdkEventSequence *sequence)
{
  BobguiGestureDragPrivate *priv;
  GdkEventSequence *current;
  double x, y;

  current = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  priv = bobgui_gesture_drag_get_instance_private (BOBGUI_GESTURE_DRAG (gesture));
  bobgui_gesture_get_point (gesture, current, &priv->last_x, &priv->last_y);
  x = priv->last_x - priv->start_x;
  y = priv->last_y - priv->start_y;

  g_signal_emit (gesture, signals[DRAG_END], 0, x, y);
}

static void
bobgui_gesture_drag_class_init (BobguiGestureDragClass *klass)
{
  BobguiGestureClass *gesture_class = BOBGUI_GESTURE_CLASS (klass);
  BobguiEventControllerClass *event_controller_class = BOBGUI_EVENT_CONTROLLER_CLASS (klass);

  event_controller_class->filter_event = bobgui_gesture_drag_filter_event;

  gesture_class->begin = bobgui_gesture_drag_begin;
  gesture_class->update = bobgui_gesture_drag_update;
  gesture_class->end = bobgui_gesture_drag_end;

  /**
   * BobguiGestureDrag::drag-begin:
   * @gesture: the object which received the signal
   * @start_x: X coordinate, relative to the widget allocation
   * @start_y: Y coordinate, relative to the widget allocation
   *
   * Emitted whenever dragging starts.
   */
  signals[DRAG_BEGIN] =
    g_signal_new (I_("drag-begin"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureDragClass, drag_begin),
                  NULL, NULL,
                  _bobgui_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[DRAG_BEGIN],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__DOUBLE_DOUBLEv);
  /**
   * BobguiGestureDrag::drag-update:
   * @gesture: the object which received the signal
   * @offset_x: X offset, relative to the start point
   * @offset_y: Y offset, relative to the start point
   *
   * Emitted whenever the dragging point moves.
   */
  signals[DRAG_UPDATE] =
    g_signal_new (I_("drag-update"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureDragClass, drag_update),
                  NULL, NULL,
                  _bobgui_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[DRAG_UPDATE],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__DOUBLE_DOUBLEv);
  /**
   * BobguiGestureDrag::drag-end:
   * @gesture: the object which received the signal
   * @offset_x: X offset, relative to the start point
   * @offset_y: Y offset, relative to the start point
   *
   * Emitted whenever the dragging is finished.
   */
  signals[DRAG_END] =
    g_signal_new (I_("drag-end"),
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_LAST,
                  G_STRUCT_OFFSET (BobguiGestureDragClass, drag_end),
                  NULL, NULL,
                  _bobgui_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[DRAG_END],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__DOUBLE_DOUBLEv);
}

static void
bobgui_gesture_drag_init (BobguiGestureDrag *gesture)
{
}

/**
 * bobgui_gesture_drag_new:
 *
 * Returns a newly created `BobguiGesture` that recognizes drags.
 *
 * Returns: a newly created `BobguiGestureDrag`
 **/
BobguiGesture *
bobgui_gesture_drag_new (void)
{
  return g_object_new (BOBGUI_TYPE_GESTURE_DRAG,
                       NULL);
}

/**
 * bobgui_gesture_drag_get_start_point:
 * @gesture: a `BobguiGesture`
 * @x: (out) (optional): X coordinate for the drag start point
 * @y: (out) (optional): Y coordinate for the drag start point
 *
 * Gets the point where the drag started.
 *
 * If the @gesture is active, this function returns %TRUE
 * and fills in @x and @y with the drag start coordinates,
 * in widget-relative coordinates.
 *
 * Returns: %TRUE if the gesture is active
 */
gboolean
bobgui_gesture_drag_get_start_point (BobguiGestureDrag *gesture,
                                  double         *x,
                                  double         *y)
{
  BobguiGestureDragPrivate *priv;
  GdkEventSequence *sequence;

  g_return_val_if_fail (BOBGUI_IS_GESTURE_DRAG (gesture), FALSE);

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  if (!bobgui_gesture_handles_sequence (BOBGUI_GESTURE (gesture), sequence))
    return FALSE;

  priv = bobgui_gesture_drag_get_instance_private (gesture);

  if (x)
    *x = priv->start_x;

  if (y)
    *y = priv->start_y;

  return TRUE;
}

/**
 * bobgui_gesture_drag_get_offset:
 * @gesture: a `BobguiGesture`
 * @x: (out) (optional): X offset for the current point
 * @y: (out) (optional): Y offset for the current point
 *
 * Gets the offset from the start point.
 *
 * If the @gesture is active, this function returns %TRUE and
 * fills in @x and @y with the coordinates of the current point,
 * as an offset to the starting drag point.
 *
 * Returns: %TRUE if the gesture is active
 */
gboolean
bobgui_gesture_drag_get_offset (BobguiGestureDrag *gesture,
                             double         *x,
                             double         *y)
{
  BobguiGestureDragPrivate *priv;
  GdkEventSequence *sequence;

  g_return_val_if_fail (BOBGUI_IS_GESTURE_DRAG (gesture), FALSE);

  sequence = bobgui_gesture_single_get_current_sequence (BOBGUI_GESTURE_SINGLE (gesture));

  if (!bobgui_gesture_handles_sequence (BOBGUI_GESTURE (gesture), sequence))
    return FALSE;

  priv = bobgui_gesture_drag_get_instance_private (gesture);

  if (x)
    *x = priv->last_x - priv->start_x;

  if (y)
    *y = priv->last_y - priv->start_y;

  return TRUE;
}
