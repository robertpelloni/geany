/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2017, Red Hat, Inc.
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
 * Author(s): Matthias Clasen <mclasen@redhat.com>
 */

/**
 * BobguiEventControllerMotion:
 *
 * Tracks the pointer position.
 *
 * The event controller offers [signal@Bobgui.EventControllerMotion::enter]
 * and [signal@Bobgui.EventControllerMotion::leave] signals, as well as
 * [property@Bobgui.EventControllerMotion:is-pointer] and
 * [property@Bobgui.EventControllerMotion:contains-pointer] properties
 * which are updated to reflect changes in the pointer position as it
 * moves over the widget.
 */
#include "config.h"

#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguimarshalers.h"
#include "bobguieventcontrollerprivate.h"
#include "bobguieventcontrollermotion.h"
#include "bobguitypebuiltins.h"
#include "bobguimarshalers.h"

struct _BobguiEventControllerMotion
{
  BobguiEventController parent_instance;

  guint is_pointer             : 1;
  guint contains_pointer       : 1;
};

struct _BobguiEventControllerMotionClass
{
  BobguiEventControllerClass parent_class;
};

enum {
  ENTER,
  LEAVE,
  MOTION,
  N_SIGNALS
};

enum {
  PROP_IS_POINTER = 1,
  PROP_CONTAINS_POINTER,
  NUM_PROPERTIES
};

static GParamSpec *props[NUM_PROPERTIES] = { NULL, };

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE (BobguiEventControllerMotion, bobgui_event_controller_motion, BOBGUI_TYPE_EVENT_CONTROLLER)

static gboolean
bobgui_event_controller_motion_handle_event (BobguiEventController *controller,
                                          GdkEvent           *event,
                                          double              x,
                                          double              y)
{
  BobguiEventControllerClass *parent_class;
  GdkEventType type;

  type = gdk_event_get_event_type (event);
  if (type == GDK_MOTION_NOTIFY)
    g_signal_emit (controller, signals[MOTION], 0, x, y);

  parent_class = BOBGUI_EVENT_CONTROLLER_CLASS (bobgui_event_controller_motion_parent_class);

  return parent_class->handle_event (controller, event, x, y);
}

static void
update_pointer_focus (BobguiEventController    *controller,
                      const BobguiCrossingData *crossing,
                      double                 x,
                      double                 y)
{
  BobguiEventControllerMotion *motion = BOBGUI_EVENT_CONTROLLER_MOTION (controller);
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);
  gboolean is_pointer = FALSE;
  gboolean contains_pointer = FALSE;
  gboolean enter = FALSE;
  gboolean leave = FALSE;

  if (crossing->direction == BOBGUI_CROSSING_IN)
    {
     if (crossing->new_descendent != NULL)
        {
          contains_pointer = TRUE;
        }
      if (crossing->new_target == widget)
        {
          contains_pointer = TRUE;
          is_pointer = TRUE;
        }
    }
  else
    {
      if (crossing->new_descendent != NULL ||
          crossing->new_target == widget)
        contains_pointer = TRUE;
      is_pointer = FALSE;
    }

  if (motion->contains_pointer != contains_pointer)
    {
      enter = contains_pointer;
      leave = !contains_pointer;
    }

  if (leave)
    g_signal_emit (controller, signals[LEAVE], 0);

  g_object_freeze_notify (G_OBJECT (motion));
  if (motion->is_pointer != is_pointer)
    {
      motion->is_pointer = is_pointer;
      g_object_notify_by_pspec (G_OBJECT (motion), props[PROP_IS_POINTER]);
    }
  if (motion->contains_pointer != contains_pointer)
    {
      motion->contains_pointer = contains_pointer;
      g_object_notify_by_pspec (G_OBJECT (motion), props[PROP_CONTAINS_POINTER]);
    }
  g_object_thaw_notify (G_OBJECT (motion));

  if (enter)
    g_signal_emit (controller, signals[ENTER], 0, x, y);
}

static void
bobgui_event_controller_motion_handle_crossing (BobguiEventController    *controller,
                                             const BobguiCrossingData *crossing,
                                             double                 x,
                                             double                 y)
{
  if (crossing->type == BOBGUI_CROSSING_POINTER)
    update_pointer_focus (controller, crossing, x, y);
}

static void
bobgui_event_controller_motion_get_property (GObject    *object,
                                          guint       prop_id,
                                          GValue     *value,
                                          GParamSpec *pspec)
{
  BobguiEventControllerMotion *controller = BOBGUI_EVENT_CONTROLLER_MOTION (object);

  switch (prop_id)
    {
    case PROP_IS_POINTER:
      g_value_set_boolean (value, controller->is_pointer);
      break;

    case PROP_CONTAINS_POINTER:
      g_value_set_boolean (value, controller->contains_pointer);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_event_controller_motion_class_init (BobguiEventControllerMotionClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  BobguiEventControllerClass *controller_class = BOBGUI_EVENT_CONTROLLER_CLASS (klass);

  object_class->get_property = bobgui_event_controller_motion_get_property;

  controller_class->handle_event = bobgui_event_controller_motion_handle_event;
  controller_class->handle_crossing = bobgui_event_controller_motion_handle_crossing;

  /**
   * BobguiEventControllerMotion:is-pointer:
   *
   * Whether the pointer is in the controllers widget itself,
   * as opposed to in a descendent widget.
   *
   * See also [property@Bobgui.EventControllerMotion:contains-pointer].
   *
   * When handling crossing events, this property is updated
   * before [signal@Bobgui.EventControllerMotion::enter], but after
   * [signal@Bobgui.EventControllerMotion::leave] is emitted.
   */
  props[PROP_IS_POINTER] =
      g_param_spec_boolean ("is-pointer", NULL, NULL,
                            FALSE,
                            G_PARAM_READABLE);

  /**
   * BobguiEventControllerMotion:contains-pointer:
   *
   * Whether the pointer is in the controllers widget or a descendant.
   *
   * See also [property@Bobgui.EventControllerMotion:is-pointer].
   *
   * When handling crossing events, this property is updated
   * before [signal@Bobgui.EventControllerMotion::enter], but after
   * [signal@Bobgui.EventControllerMotion::leave] is emitted.
   */
  props[PROP_CONTAINS_POINTER] =
      g_param_spec_boolean ("contains-pointer", NULL, NULL,
                            FALSE,
                            G_PARAM_READABLE);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, props);

  /**
   * BobguiEventControllerMotion::enter:
   * @controller: the object which received the signal
   * @x: coordinates of pointer location
   * @y: coordinates of pointer location
   *
   * Signals that the pointer has entered the widget.
   */
  signals[ENTER] =
    g_signal_new (I_("enter"),
                  BOBGUI_TYPE_EVENT_CONTROLLER_MOTION,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _bobgui_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2,
                  G_TYPE_DOUBLE,
                  G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[ENTER],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__DOUBLE_DOUBLEv);

  /**
   * BobguiEventControllerMotion::leave:
   * @controller: the object which received the signal
   *
   * Signals that the pointer has left the widget.
   */
  signals[LEAVE] =
    g_signal_new (I_("leave"),
                  BOBGUI_TYPE_EVENT_CONTROLLER_MOTION,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiEventControllerMotion::motion:
   * @controller: The object that received the signal
   * @x: the x coordinate
   * @y: the y coordinate
   *
   * Emitted when the pointer moves inside the widget.
   */
  signals[MOTION] =
    g_signal_new (I_("motion"),
                  BOBGUI_TYPE_EVENT_CONTROLLER_MOTION,
                  G_SIGNAL_RUN_FIRST,
                  0, NULL, NULL,
                  _bobgui_marshal_VOID__DOUBLE_DOUBLE,
                  G_TYPE_NONE, 2, G_TYPE_DOUBLE, G_TYPE_DOUBLE);
  g_signal_set_va_marshaller (signals[MOTION],
                              BOBGUI_TYPE_EVENT_CONTROLLER_MOTION,
                              _bobgui_marshal_VOID__DOUBLE_DOUBLEv);
}

static void
bobgui_event_controller_motion_init (BobguiEventControllerMotion *motion)
{
}

/**
 * bobgui_event_controller_motion_new:
 *
 * Creates a new event controller that will handle motion events.
 *
 * Returns: a new `BobguiEventControllerMotion`
 **/
BobguiEventController *
bobgui_event_controller_motion_new (void)
{
  return g_object_new (BOBGUI_TYPE_EVENT_CONTROLLER_MOTION,
                       NULL);
}

/**
 * bobgui_event_controller_motion_contains_pointer:
 * @self: a `BobguiEventControllerMotion`
 *
 * Returns if a pointer is within @self or one of its children.
 *
 * Returns: %TRUE if a pointer is within @self or one of its children
 */
gboolean
bobgui_event_controller_motion_contains_pointer (BobguiEventControllerMotion *self)
{
  g_return_val_if_fail (BOBGUI_IS_EVENT_CONTROLLER_MOTION (self), FALSE);

  return self->contains_pointer;
}

/**
 * bobgui_event_controller_motion_is_pointer:
 * @self: a `BobguiEventControllerMotion`
 *
 * Returns if a pointer is within @self, but not one of its children.
 *
 * Returns: %TRUE if a pointer is within @self but not one of its children
 */
gboolean
bobgui_event_controller_motion_is_pointer (BobguiEventControllerMotion *self)
{
  g_return_val_if_fail (BOBGUI_IS_EVENT_CONTROLLER_MOTION (self), FALSE);

  return self->is_pointer;
}
