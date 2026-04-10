/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2020, Red Hat, Inc.
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
 * BobguiEventControllerFocus:
 *
 * Tracks keyboard focus.
 *
 * The event controller offers [signal@Bobgui.EventControllerFocus::enter]
 * and [signal@Bobgui.EventControllerFocus::leave] signals, as well as
 * [property@Bobgui.EventControllerFocus:is-focus] and
 * [property@Bobgui.EventControllerFocus:contains-focus] properties
 * which are updated to reflect focus changes inside the widget hierarchy
 * that is rooted at the controllers widget.
 */

#include "config.h"

#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguieventcontrollerprivate.h"
#include "bobguieventcontrollerfocus.h"
#include "bobguienums.h"
#include "bobguimain.h"
#include "bobguitypebuiltins.h"

#include <gdk/gdk.h>

struct _BobguiEventControllerFocus
{
  BobguiEventController parent_instance;

  guint is_focus       : 1;
  guint contains_focus : 1;
};

struct _BobguiEventControllerFocusClass
{
  BobguiEventControllerClass parent_class;
};

enum {
  ENTER,
  LEAVE,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

enum {
  PROP_IS_FOCUS = 1,
  PROP_CONTAINS_FOCUS,
  NUM_PROPERTIES
};

static GParamSpec *props[NUM_PROPERTIES] = { NULL, };

G_DEFINE_TYPE (BobguiEventControllerFocus, bobgui_event_controller_focus,
               BOBGUI_TYPE_EVENT_CONTROLLER)

static void
bobgui_event_controller_focus_finalize (GObject *object)
{
  //BobguiEventControllerFocus *focus = BOBGUI_EVENT_CONTROLLER_FOCUS (object);

  G_OBJECT_CLASS (bobgui_event_controller_focus_parent_class)->finalize (object);
}

static void
update_focus (BobguiEventController    *controller,
              const BobguiCrossingData *crossing)
{
  BobguiEventControllerFocus *focus = BOBGUI_EVENT_CONTROLLER_FOCUS (controller);
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);
  gboolean is_focus = FALSE;
  gboolean contains_focus = FALSE;
  gboolean enter = FALSE;
  gboolean leave = FALSE;

  if (crossing->direction == BOBGUI_CROSSING_IN)
    {
      if (crossing->new_descendent != NULL)
        {
          contains_focus = TRUE;
        }
      if (crossing->new_target == widget)
        {
          contains_focus = TRUE;
          is_focus = TRUE;
        }
    }
  else
    {
      if (crossing->new_descendent != NULL ||
          crossing->new_target == widget)
        contains_focus = TRUE;
      is_focus = FALSE;
    }

  if (focus->contains_focus != contains_focus)
    {
      enter = contains_focus;
      leave = !contains_focus;
    }

  if (leave)
    g_signal_emit (controller, signals[LEAVE], 0);

  g_object_freeze_notify (G_OBJECT (focus));
  if (focus->is_focus != is_focus)
    {
      focus->is_focus = is_focus;
      g_object_notify (G_OBJECT (focus), "is-focus");
    }

  if (focus->contains_focus != contains_focus)
    {
      focus->contains_focus = contains_focus;
      g_object_notify (G_OBJECT (focus), "contains-focus");
    }
  g_object_thaw_notify (G_OBJECT (focus));

  if (enter)
    g_signal_emit (controller, signals[ENTER], 0);
}

static void
bobgui_event_controller_focus_handle_crossing (BobguiEventController    *controller,
                                            const BobguiCrossingData *crossing,
                                            double                 x,
                                            double                 y)
{
  if (crossing->type == BOBGUI_CROSSING_FOCUS ||
      crossing->type == BOBGUI_CROSSING_ACTIVE)
    update_focus (controller, crossing);
}

static void
bobgui_event_controller_focus_get_property (GObject    *object,
                                         guint       prop_id,
                                         GValue     *value,
                                         GParamSpec *pspec)
{
  BobguiEventControllerFocus *controller = BOBGUI_EVENT_CONTROLLER_FOCUS (object);

  switch (prop_id)
    {
    case PROP_IS_FOCUS:
      g_value_set_boolean (value, controller->is_focus);
      break;

    case PROP_CONTAINS_FOCUS:
      g_value_set_boolean (value, controller->contains_focus);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    }
}

static void
bobgui_event_controller_focus_class_init (BobguiEventControllerFocusClass *klass)
{
  BobguiEventControllerClass *controller_class = BOBGUI_EVENT_CONTROLLER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bobgui_event_controller_focus_finalize;
  object_class->get_property = bobgui_event_controller_focus_get_property;
  controller_class->handle_crossing = bobgui_event_controller_focus_handle_crossing;

  /**
   * BobguiEventControllerFocus:is-focus:
   *
   * %TRUE if focus is in the controllers widget itself,
   * as opposed to in a descendent widget.
   *
   * See also [property@Bobgui.EventControllerFocus:contains-focus].
   *
   * When handling focus events, this property is updated
   * before [signal@Bobgui.EventControllerFocus::enter] or
   * [signal@Bobgui.EventControllerFocus::leave] are emitted.
   */
  props[PROP_IS_FOCUS] =
      g_param_spec_boolean ("is-focus", NULL, NULL,
                            FALSE,
                            G_PARAM_READABLE);

  /**
   * BobguiEventControllerFocus:contains-focus:
   *
   * %TRUE if focus is contained in the controllers widget.
   *
   * See [property@Bobgui.EventControllerFocus:is-focus] for whether
   * the focus is in the widget itself or inside a descendent.
   *
   * When handling focus events, this property is updated
   * before [signal@Bobgui.EventControllerFocus::enter] or
   * [signal@Bobgui.EventControllerFocus::leave] are emitted.
   */
  props[PROP_CONTAINS_FOCUS] =
      g_param_spec_boolean ("contains-focus", NULL, NULL,
                            FALSE,
                            G_PARAM_READABLE);

  g_object_class_install_properties (object_class, NUM_PROPERTIES, props);

  /**
   * BobguiEventControllerFocus::enter:
   * @controller: the object which received the signal
   *
   * Emitted whenever the focus enters into the widget or one
   * of its descendents.
   *
   * Note that this means you may not get an ::enter signal
   * even though the widget becomes the focus location, in
   * certain cases (such as when the focus moves from a descendent
   * of the widget to the widget itself). If you are interested
   * in these cases, you can monitor the
   * [property@Bobgui.EventControllerFocus:is-focus]
   * property for changes.
   */
  signals[ENTER] =
    g_signal_new (I_("enter"),
                  BOBGUI_TYPE_EVENT_CONTROLLER_FOCUS,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  /**
   * BobguiEventControllerFocus::leave:
   * @controller: the object which received the signal
   *
   * Emitted whenever the focus leaves the widget hierarchy
   * that is rooted at the widget that the controller is attached to.
   *
   * Note that this means you may not get a ::leave signal
   * even though the focus moves away from the widget, in
   * certain cases (such as when the focus moves from the widget
   * to a descendent). If you are interested in these cases, you
   * can monitor the [property@Bobgui.EventControllerFocus:is-focus]
   * property for changes.
   */
  signals[LEAVE] =
    g_signal_new (I_("leave"),
                  BOBGUI_TYPE_EVENT_CONTROLLER_FOCUS,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
}

static void
bobgui_event_controller_focus_init (BobguiEventControllerFocus *controller)
{
}

/**
 * bobgui_event_controller_focus_new:
 *
 * Creates a new event controller that will handle focus events.
 *
 * Returns: a new `BobguiEventControllerFocus`
 **/
BobguiEventController *
bobgui_event_controller_focus_new (void)
{
  return g_object_new (BOBGUI_TYPE_EVENT_CONTROLLER_FOCUS, NULL);
}

/**
 * bobgui_event_controller_focus_contains_focus:
 * @self: a `BobguiEventControllerFocus`
 *
 * Returns %TRUE if focus is within @self or one of its children.
 *
 * Returns: %TRUE if focus is within @self or one of its children
 */
gboolean
bobgui_event_controller_focus_contains_focus (BobguiEventControllerFocus *self)
{
  g_return_val_if_fail (BOBGUI_IS_EVENT_CONTROLLER_FOCUS (self), FALSE);

  return self->contains_focus;
}

/**
 * bobgui_event_controller_focus_is_focus:
 * @self: a `BobguiEventControllerFocus`
 *
 * Returns %TRUE if focus is within @self, but not one of its children.
 *
 * Returns: %TRUE if focus is within @self, but not one of its children
 */
gboolean
bobgui_event_controller_focus_is_focus (BobguiEventControllerFocus *self)
{
  g_return_val_if_fail (BOBGUI_IS_EVENT_CONTROLLER_FOCUS (self), FALSE);

  return self->is_focus;
}
