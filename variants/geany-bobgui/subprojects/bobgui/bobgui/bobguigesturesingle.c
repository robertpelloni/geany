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
 * BobguiGestureSingle:
 *
 * A `BobguiGesture` subclass optimized for singe-touch and mouse gestures.
 *
 * Under interaction, these gestures stick to the first interacting sequence,
 * which is accessible through [method@Bobgui.GestureSingle.get_current_sequence]
 * while the gesture is being interacted with.
 *
 * By default gestures react to both %GDK_BUTTON_PRIMARY and touch events.
 * [method@Bobgui.GestureSingle.set_touch_only] can be used to change the
 * touch behavior. Callers may also specify a different mouse button number
 * to interact with through [method@Bobgui.GestureSingle.set_button], or react
 * to any mouse button by setting it to 0. While the gesture is active, the
 * button being currently pressed can be known through
 * [method@Bobgui.GestureSingle.get_current_button].
 */

#include "config.h"
#include "bobguigesturesingle.h"
#include "bobguigesturesingleprivate.h"
#include "bobguiprivate.h"
#include "bobguidebug.h"

typedef struct _BobguiGestureSinglePrivate BobguiGestureSinglePrivate;

struct _BobguiGestureSinglePrivate
{
  GdkEventSequence *current_sequence;
  guint button;
  guint current_button;
  guint touch_only : 1;
  guint exclusive  : 1;
};

enum {
  PROP_TOUCH_ONLY = 1,
  PROP_EXCLUSIVE,
  PROP_BUTTON,
  LAST_PROP
};

static GParamSpec *properties[LAST_PROP] = { NULL, };

G_DEFINE_TYPE_WITH_PRIVATE (BobguiGestureSingle, bobgui_gesture_single, BOBGUI_TYPE_GESTURE)

static void
bobgui_gesture_single_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BobguiGestureSinglePrivate *priv;

  priv = bobgui_gesture_single_get_instance_private (BOBGUI_GESTURE_SINGLE (object));

  switch (prop_id)
    {
    case PROP_TOUCH_ONLY:
      g_value_set_boolean (value, priv->touch_only);
      break;
    case PROP_EXCLUSIVE:
      g_value_set_boolean (value, priv->exclusive);
      break;
    case PROP_BUTTON:
      g_value_set_uint (value, priv->button);
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_gesture_single_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  switch (prop_id)
    {
    case PROP_TOUCH_ONLY:
      bobgui_gesture_single_set_touch_only (BOBGUI_GESTURE_SINGLE (object),
                                         g_value_get_boolean (value));
      break;
    case PROP_EXCLUSIVE:
      bobgui_gesture_single_set_exclusive (BOBGUI_GESTURE_SINGLE (object),
                                        g_value_get_boolean (value));
      break;
    case PROP_BUTTON:
      bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (object),
                                     g_value_get_uint (value));
      break;
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_gesture_single_cancel (BobguiGesture       *gesture,
                           GdkEventSequence *sequence)
{
  BobguiGestureSinglePrivate *priv;

  priv = bobgui_gesture_single_get_instance_private (BOBGUI_GESTURE_SINGLE (gesture));

  if (sequence == priv->current_sequence)
    priv->current_button = 0;
}

static gboolean
bobgui_gesture_single_handle_event (BobguiEventController *controller,
                                 GdkEvent           *event,
                                 double              x,
                                 double              y)
{
  GdkEventSequence *sequence = NULL;
  BobguiGestureSinglePrivate *priv;
  GdkDevice *source_device;
  GdkInputSource source;
  guint button = 0, state, i;
  gboolean retval;
  GdkEventType event_type;

  source_device = gdk_event_get_device (event);

  if (!source_device)
    return FALSE;

  priv = bobgui_gesture_single_get_instance_private (BOBGUI_GESTURE_SINGLE (controller));
  source = gdk_device_get_source (source_device);

  event_type = gdk_event_get_event_type (event);

  switch ((guint) event_type)
    {
    case GDK_TOUCH_BEGIN:
    case GDK_TOUCH_END:
    case GDK_TOUCH_UPDATE:
      if (priv->exclusive && !gdk_touch_event_get_emulating_pointer (event))
        return FALSE;

      sequence = gdk_event_get_event_sequence (event);
      button = 1;
      break;
    case GDK_BUTTON_PRESS:
    case GDK_BUTTON_RELEASE:
      if (priv->touch_only && source != GDK_SOURCE_TOUCHSCREEN)
        return FALSE;

      button = gdk_button_event_get_button (event);
      break;
    case GDK_MOTION_NOTIFY:
      if (!bobgui_gesture_handles_sequence (BOBGUI_GESTURE (controller), sequence))
        return FALSE;
      if (priv->touch_only && source != GDK_SOURCE_TOUCHSCREEN)
        return FALSE;
      state = gdk_event_get_modifier_state (event);

      if (priv->current_button > 0 && priv->current_button <= 5 &&
          (state & (GDK_BUTTON1_MASK << (priv->current_button - 1))))
        button = priv->current_button;
      else if (priv->current_button == 0)
        {
          /* No current button, find out from the mask */
          for (i = 0; i < 3; i++)
            {
              if ((state & (GDK_BUTTON1_MASK << i)) == 0)
                continue;
              button = i + 1;
              break;
            }
        }

      break;
    case GDK_TOUCHPAD_HOLD:
      if (gdk_touchpad_event_get_n_fingers (event) == 1)
        return FALSE;
      G_GNUC_FALLTHROUGH;
    case GDK_TOUCH_CANCEL:
    case GDK_GRAB_BROKEN:
    case GDK_TOUCHPAD_SWIPE:
      return BOBGUI_EVENT_CONTROLLER_CLASS (bobgui_gesture_single_parent_class)->handle_event (controller, event, x, y);
      break;
    default:
      return FALSE;
    }

  if (button == 0 ||
      (priv->button != 0 && priv->button != button) ||
      (priv->current_button != 0 && priv->current_button != button))
    {
      if (bobgui_gesture_is_active (BOBGUI_GESTURE (controller)))
        bobgui_event_controller_reset (controller);
      return FALSE;
    }

  if (event_type == GDK_BUTTON_PRESS || event_type == GDK_TOUCH_BEGIN ||
      event_type == GDK_MOTION_NOTIFY || event_type == GDK_TOUCH_UPDATE)
    {
      if (!bobgui_gesture_is_active (BOBGUI_GESTURE (controller)))
        priv->current_sequence = sequence;

      priv->current_button = button;
    }

  retval = BOBGUI_EVENT_CONTROLLER_CLASS (bobgui_gesture_single_parent_class)->handle_event (controller, event, x, y);

  if (sequence == priv->current_sequence &&
      (event_type == GDK_BUTTON_RELEASE || event_type == GDK_TOUCH_END))
    priv->current_button = 0;
  else if (priv->current_sequence == sequence &&
           !bobgui_gesture_handles_sequence (BOBGUI_GESTURE (controller), sequence))
    {
      if (button == priv->current_button && event_type == GDK_BUTTON_PRESS)
        priv->current_button = 0;
      else if (sequence == priv->current_sequence && event_type == GDK_TOUCH_BEGIN)
        priv->current_sequence = NULL;
    }

  return retval;
}

static void
bobgui_gesture_single_class_init (BobguiGestureSingleClass *klass)
{
  BobguiEventControllerClass *controller_class = BOBGUI_EVENT_CONTROLLER_CLASS (klass);
  BobguiGestureClass *gesture_class = BOBGUI_GESTURE_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->get_property = bobgui_gesture_single_get_property;
  object_class->set_property = bobgui_gesture_single_set_property;

  controller_class->handle_event = bobgui_gesture_single_handle_event;

  gesture_class->cancel = bobgui_gesture_single_cancel;

  /**
   * BobguiGestureSingle:touch-only:
   *
   * Whether the gesture handles only touch events.
   */
  properties[PROP_TOUCH_ONLY] =
      g_param_spec_boolean ("touch-only", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGestureSingle:exclusive:
   *
   * Whether the gesture is exclusive.
   *
   * Exclusive gestures only listen to pointer and pointer emulated events.
   */
  properties[PROP_EXCLUSIVE] =
      g_param_spec_boolean ("exclusive", NULL, NULL,
                            FALSE,
                            BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGestureSingle:button:
   *
   * Mouse button number to listen to, or 0 to listen for any button.
   */
  properties[PROP_BUTTON] =
      g_param_spec_uint ("button", NULL, NULL,
                         0, G_MAXUINT,
                         GDK_BUTTON_PRIMARY,
                         BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (object_class, LAST_PROP, properties);
}

static void
bobgui_gesture_single_init (BobguiGestureSingle *gesture)
{
  BobguiGestureSinglePrivate *priv;

  priv = bobgui_gesture_single_get_instance_private (gesture);
  priv->touch_only = FALSE;
  priv->button = GDK_BUTTON_PRIMARY;
}

/**
 * bobgui_gesture_single_get_touch_only:
 * @gesture: a `BobguiGestureSingle`
 *
 * Returns %TRUE if the gesture is only triggered by touch events.
 *
 * Returns: %TRUE if the gesture only handles touch events
 */
gboolean
bobgui_gesture_single_get_touch_only (BobguiGestureSingle *gesture)
{
  BobguiGestureSinglePrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_GESTURE_SINGLE (gesture), FALSE);

  priv = bobgui_gesture_single_get_instance_private (gesture);

  return priv->touch_only;
}

/**
 * bobgui_gesture_single_set_touch_only:
 * @gesture: a `BobguiGestureSingle`
 * @touch_only: whether @gesture handles only touch events
 *
 * Sets whether to handle only touch events.
 *
 * If @touch_only is %TRUE, @gesture will only handle events of type
 * %GDK_TOUCH_BEGIN, %GDK_TOUCH_UPDATE or %GDK_TOUCH_END. If %FALSE,
 * mouse events will be handled too.
 */
void
bobgui_gesture_single_set_touch_only (BobguiGestureSingle *gesture,
                                   gboolean          touch_only)
{
  BobguiGestureSinglePrivate *priv;

  g_return_if_fail (BOBGUI_IS_GESTURE_SINGLE (gesture));

  touch_only = touch_only != FALSE;
  priv = bobgui_gesture_single_get_instance_private (gesture);

  if (priv->touch_only == touch_only)
    return;

  priv->touch_only = touch_only;
  g_object_notify_by_pspec (G_OBJECT (gesture), properties[PROP_TOUCH_ONLY]);
}

/**
 * bobgui_gesture_single_get_exclusive:
 * @gesture: a `BobguiGestureSingle`
 *
 * Gets whether a gesture is exclusive.
 *
 * For more information, see [method@Bobgui.GestureSingle.set_exclusive].
 *
 * Returns: Whether the gesture is exclusive
 */
gboolean
bobgui_gesture_single_get_exclusive (BobguiGestureSingle *gesture)
{
  BobguiGestureSinglePrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_GESTURE_SINGLE (gesture), FALSE);

  priv = bobgui_gesture_single_get_instance_private (gesture);

  return priv->exclusive;
}

/**
 * bobgui_gesture_single_set_exclusive:
 * @gesture: a `BobguiGestureSingle`
 * @exclusive: %TRUE to make @gesture exclusive
 *
 * Sets whether @gesture is exclusive.
 *
 * An exclusive gesture will only handle pointer and "pointer emulated"
 * touch events, so at any given time, there is only one sequence able
 * to interact with those.
 */
void
bobgui_gesture_single_set_exclusive (BobguiGestureSingle *gesture,
                                  gboolean          exclusive)
{
  BobguiGestureSinglePrivate *priv;

  g_return_if_fail (BOBGUI_IS_GESTURE_SINGLE (gesture));

  exclusive = exclusive != FALSE;
  priv = bobgui_gesture_single_get_instance_private (gesture);

  if (priv->exclusive == exclusive)
    return;

  priv->exclusive = exclusive;
  g_object_notify_by_pspec (G_OBJECT (gesture), properties[PROP_EXCLUSIVE]);
}

/**
 * bobgui_gesture_single_get_button:
 * @gesture: a `BobguiGestureSingle`
 *
 * Returns the button number @gesture listens for.
 *
 * If this is 0, the gesture reacts to any button press.
 *
 * Returns: The button number, or 0 for any button
 */
guint
bobgui_gesture_single_get_button (BobguiGestureSingle *gesture)
{
  BobguiGestureSinglePrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_GESTURE_SINGLE (gesture), 0);

  priv = bobgui_gesture_single_get_instance_private (gesture);

  return priv->button;
}

/**
 * bobgui_gesture_single_set_button:
 * @gesture: a `BobguiGestureSingle`
 * @button: button number to listen to, or 0 for any button
 *
 * Sets the button number @gesture listens to.
 *
 * If non-0, every button press from a different button
 * number will be ignored. Touch events implicitly match
 * with button 1.
 */
void
bobgui_gesture_single_set_button (BobguiGestureSingle *gesture,
                               guint             button)
{
  BobguiGestureSinglePrivate *priv;

  g_return_if_fail (BOBGUI_IS_GESTURE_SINGLE (gesture));

  priv = bobgui_gesture_single_get_instance_private (gesture);

  if (priv->button == button)
    return;

  priv->button = button;
  g_object_notify_by_pspec (G_OBJECT (gesture), properties[PROP_BUTTON]);
}

/**
 * bobgui_gesture_single_get_current_button:
 * @gesture: a `BobguiGestureSingle`
 *
 * Returns the button number currently interacting
 * with @gesture, or 0 if there is none.
 *
 * Returns: The current button number
 */
guint
bobgui_gesture_single_get_current_button (BobguiGestureSingle *gesture)
{
  BobguiGestureSinglePrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_GESTURE_SINGLE (gesture), 0);

  priv = bobgui_gesture_single_get_instance_private (gesture);

  return priv->current_button;
}

/**
 * bobgui_gesture_single_get_current_sequence:
 * @gesture: a `BobguiGestureSingle`
 *
 * Returns the event sequence currently interacting with @gesture.
 *
 * This is only meaningful if [method@Bobgui.Gesture.is_active]
 * returns %TRUE.
 *
 * Returns: (nullable): the current sequence
 */
GdkEventSequence *
bobgui_gesture_single_get_current_sequence (BobguiGestureSingle *gesture)
{
  BobguiGestureSinglePrivate *priv;

  g_return_val_if_fail (BOBGUI_IS_GESTURE_SINGLE (gesture), NULL);

  priv = bobgui_gesture_single_get_instance_private (gesture);

  return priv->current_sequence;
}
