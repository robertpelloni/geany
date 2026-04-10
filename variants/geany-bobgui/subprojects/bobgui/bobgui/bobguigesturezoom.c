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
 * BobguiGestureZoom:
 *
 * Recognizes 2-finger pinch/zoom gestures.
 *
 * Whenever the distance between both tracked sequences changes, the
 * [signal@Bobgui.GestureZoom::scale-changed] signal is emitted to report
 * the scale factor.
 */

#include "config.h"
#include <math.h>
#include "bobguigesturezoom.h"
#include "bobguigesturezoomprivate.h"
#include "bobguiprivate.h"

typedef struct _BobguiGestureZoomPrivate BobguiGestureZoomPrivate;

enum {
  SCALE_CHANGED,
  LAST_SIGNAL
};

struct _BobguiGestureZoomPrivate
{
  double initial_distance;
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (BobguiGestureZoom, bobgui_gesture_zoom, BOBGUI_TYPE_GESTURE)

static void
bobgui_gesture_zoom_init (BobguiGestureZoom *gesture)
{
}

static GObject *
bobgui_gesture_zoom_constructor (GType                  type,
                              guint                  n_construct_properties,
                              GObjectConstructParam *construct_properties)
{
  GObject *object;

  object = G_OBJECT_CLASS (bobgui_gesture_zoom_parent_class)->constructor (type,
                                                                        n_construct_properties,
                                                                        construct_properties);
  g_object_set (object, "n-points", 2, NULL);

  return object;
}

static gboolean
_bobgui_gesture_zoom_get_distance (BobguiGestureZoom *zoom,
                                double         *distance)
{
  GdkEvent *last_event;
  double x1, y1, x2, y2;
  BobguiGesture *gesture;
  GList *sequences = NULL;
  double dx, dy;
  GdkTouchpadGesturePhase phase;
  gboolean retval = FALSE;

  gesture = BOBGUI_GESTURE (zoom);

  if (!bobgui_gesture_is_recognized (gesture))
    goto out;

  sequences = bobgui_gesture_get_sequences (gesture);
  if (!sequences)
    goto out;

  last_event = bobgui_gesture_get_last_event (gesture, sequences->data);

  if (gdk_event_get_event_type (last_event) == GDK_TOUCHPAD_PINCH)
    {
      double scale;

      /* Touchpad pinch */
      phase = gdk_touchpad_event_get_gesture_phase (last_event);
      if (phase == GDK_TOUCHPAD_GESTURE_PHASE_CANCEL)
        goto out;

      scale = gdk_touchpad_event_get_pinch_scale (last_event);
      *distance = scale;
    }
  else
    {
      if (!sequences->next)
        goto out;

      bobgui_gesture_get_point (gesture, sequences->data, &x1, &y1);
      bobgui_gesture_get_point (gesture, sequences->next->data, &x2, &y2);

      dx = x1 - x2;
      dy = y1 - y2;;
      *distance = sqrt ((dx * dx) + (dy * dy));
    }

  retval = TRUE;

 out:
  g_list_free (sequences);
  return retval;
}

static gboolean
_bobgui_gesture_zoom_check_emit (BobguiGestureZoom *gesture)
{
  BobguiGestureZoomPrivate *priv;
  double distance, zoom;

  if (!_bobgui_gesture_zoom_get_distance (gesture, &distance))
    return FALSE;

  priv = bobgui_gesture_zoom_get_instance_private (gesture);

  if (distance == 0 || priv->initial_distance == 0)
    return FALSE;

  zoom = distance / priv->initial_distance;
  g_signal_emit (gesture, signals[SCALE_CHANGED], 0, zoom);

  return TRUE;
}

static gboolean
bobgui_gesture_zoom_filter_event (BobguiEventController *controller,
                               GdkEvent           *event)
{
  /* Let 2-finger touchpad pinch and hold events go through */
  if (gdk_event_get_event_type (event) == GDK_TOUCHPAD_PINCH)
    {
      guint n_fingers;

      n_fingers = gdk_touchpad_event_get_n_fingers (event);

      if (n_fingers == 2)
        return FALSE;
      else
        return TRUE;
    }
  else if (gdk_event_get_event_type (event) == GDK_TOUCHPAD_HOLD)
    return TRUE;

  return BOBGUI_EVENT_CONTROLLER_CLASS (bobgui_gesture_zoom_parent_class)->filter_event (controller, event);
}

static void
bobgui_gesture_zoom_begin (BobguiGesture       *gesture,
                        GdkEventSequence *sequence)
{
  BobguiGestureZoom *zoom = BOBGUI_GESTURE_ZOOM (gesture);
  BobguiGestureZoomPrivate *priv;

  priv = bobgui_gesture_zoom_get_instance_private (zoom);
  _bobgui_gesture_zoom_get_distance (zoom, &priv->initial_distance);
}

static void
bobgui_gesture_zoom_update (BobguiGesture       *gesture,
                         GdkEventSequence *sequence)
{
  _bobgui_gesture_zoom_check_emit (BOBGUI_GESTURE_ZOOM (gesture));
}

static void
bobgui_gesture_zoom_class_init (BobguiGestureZoomClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiEventControllerClass *event_controller_class = BOBGUI_EVENT_CONTROLLER_CLASS (klass);
  BobguiGestureClass *gesture_class = BOBGUI_GESTURE_CLASS (klass);

  object_class->constructor = bobgui_gesture_zoom_constructor;

  event_controller_class->filter_event = bobgui_gesture_zoom_filter_event;

  gesture_class->begin = bobgui_gesture_zoom_begin;
  gesture_class->update = bobgui_gesture_zoom_update;

  /**
   * BobguiGestureZoom::scale-changed:
   * @controller: the object on which the signal is emitted
   * @scale: Scale delta, taking the initial state as 1:1
   *
   * Emitted whenever the distance between both tracked sequences changes.
   */
  signals[SCALE_CHANGED] =
    g_signal_new (I_("scale-changed"),
                  BOBGUI_TYPE_GESTURE_ZOOM,
                  G_SIGNAL_RUN_FIRST,
                  G_STRUCT_OFFSET (BobguiGestureZoomClass, scale_changed),
                  NULL, NULL, NULL,
                  G_TYPE_NONE, 1, G_TYPE_DOUBLE);
}

/**
 * bobgui_gesture_zoom_new:
 *
 * Returns a newly created `BobguiGesture` that recognizes
 * pinch/zoom gestures.
 *
 * Returns: a newly created `BobguiGestureZoom`
 */
BobguiGesture *
bobgui_gesture_zoom_new (void)
{
  return g_object_new (BOBGUI_TYPE_GESTURE_ZOOM,
                       NULL);
}

/**
 * bobgui_gesture_zoom_get_scale_delta:
 * @gesture: a `BobguiGestureZoom`
 *
 * Gets the scale delta.
 *
 * If @gesture is active, this function returns the zooming
 * difference since the gesture was recognized (hence the
 * starting point is considered 1:1). If @gesture is not
 * active, 1 is returned.
 *
 * Returns: the scale delta
 */
double
bobgui_gesture_zoom_get_scale_delta (BobguiGestureZoom *gesture)
{
  BobguiGestureZoomPrivate *priv;
  double distance;

  g_return_val_if_fail (BOBGUI_IS_GESTURE_ZOOM (gesture), 1.0);

  if (!_bobgui_gesture_zoom_get_distance (gesture, &distance))
    return 1.0;

  priv = bobgui_gesture_zoom_get_instance_private (gesture);

  return distance / priv->initial_distance;
}
