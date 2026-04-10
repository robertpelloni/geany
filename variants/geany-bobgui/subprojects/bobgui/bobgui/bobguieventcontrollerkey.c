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
 * Author(s): Carlos Garnacho <carlosg@gnome.org>
 */

/**
 * BobguiEventControllerKey:
 *
 * Provides access to key events.
 */

#include "config.h"

#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"
#include "bobguieventcontrollerprivate.h"
#include "bobguieventcontrollerkey.h"
#include "bobguienums.h"
#include "bobguimain.h"
#include "bobguitypebuiltins.h"

#include <gdk/gdk.h>

struct _BobguiEventControllerKey
{
  BobguiEventController parent_instance;
  BobguiIMContext *im_context;
  GHashTable *pressed_keys;

  GdkModifierType state;

  gboolean is_focus;

  GdkEvent *current_event;
};

struct _BobguiEventControllerKeyClass
{
  BobguiEventControllerClass parent_class;
};

enum {
  KEY_PRESSED,
  KEY_RELEASED,
  MODIFIERS,
  IM_UPDATE,
  N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE (BobguiEventControllerKey, bobgui_event_controller_key,
               BOBGUI_TYPE_EVENT_CONTROLLER)

static void
bobgui_event_controller_key_finalize (GObject *object)
{
  BobguiEventControllerKey *key = BOBGUI_EVENT_CONTROLLER_KEY (object);

  g_hash_table_destroy (key->pressed_keys);
  g_clear_object (&key->im_context);

  G_OBJECT_CLASS (bobgui_event_controller_key_parent_class)->finalize (object);
}

static gboolean
bobgui_event_controller_key_handle_event (BobguiEventController *controller,
                                       GdkEvent           *event,
                                       double              x,
                                       double              y)
{
  BobguiEventControllerKey *key = BOBGUI_EVENT_CONTROLLER_KEY (controller);
  GdkEventType event_type = gdk_event_get_event_type (event);
  GdkModifierType state;
  guint16 keycode;
  guint keyval;
  gboolean handled = FALSE;

  if (event_type != GDK_KEY_PRESS && event_type != GDK_KEY_RELEASE)
    return FALSE;

  if (key->im_context &&
      bobgui_im_context_filter_keypress (key->im_context, event))
    {
      g_signal_emit (controller, signals[IM_UPDATE], 0);
      return TRUE;
    }

  key->current_event = event;

  state = gdk_event_get_modifier_state (event);
  if (key->state != state)
    {
      gboolean unused;

      key->state = state;
      g_signal_emit (controller, signals[MODIFIERS], 0, state, &unused);
    }

  keycode = gdk_key_event_get_keycode (event);
  keyval = gdk_key_event_get_keyval (event);

  if (event_type == GDK_KEY_PRESS)
    {
      g_signal_emit (controller, signals[KEY_PRESSED], 0,
                     keyval, keycode, state, &handled);
      if (handled)
        g_hash_table_add (key->pressed_keys, GUINT_TO_POINTER (keyval));
    }
  else if (event_type == GDK_KEY_RELEASE)
    {
      g_signal_emit (controller, signals[KEY_RELEASED], 0,
                     keyval, keycode, state);

      handled = g_hash_table_lookup (key->pressed_keys, GUINT_TO_POINTER (keyval)) != NULL;
      g_hash_table_remove (key->pressed_keys, GUINT_TO_POINTER (keyval));
    }
  else
    handled = FALSE;

  key->current_event = NULL;

  return handled;
}

static void
bobgui_event_controller_key_handle_crossing (BobguiEventController    *controller,
                                          const BobguiCrossingData *crossing,
                                          double                 x,
                                          double                 y)
{
  BobguiEventControllerKey *key = BOBGUI_EVENT_CONTROLLER_KEY (controller);
  BobguiWidget *widget = bobgui_event_controller_get_widget (controller);
  gboolean start_crossing, end_crossing;
  gboolean is_focus;

  if (crossing->type != BOBGUI_CROSSING_FOCUS &&
      crossing->type != BOBGUI_CROSSING_ACTIVE)
    return;

  start_crossing = crossing->direction == BOBGUI_CROSSING_OUT &&
                   widget == crossing->old_target;
  end_crossing = crossing->direction == BOBGUI_CROSSING_IN &&
                 widget == crossing->new_target;

  if (!start_crossing && !end_crossing)
    return;

  is_focus = end_crossing;

  if (key->is_focus != is_focus)
    {
      key->is_focus = is_focus;

      if (key->im_context)
        {
          if (is_focus)
            bobgui_im_context_focus_in (key->im_context);
          else
            bobgui_im_context_focus_out (key->im_context);
        }
    }
}

static void
bobgui_event_controller_key_class_init (BobguiEventControllerKeyClass *klass)
{
  BobguiEventControllerClass *controller_class = BOBGUI_EVENT_CONTROLLER_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->finalize = bobgui_event_controller_key_finalize;
  controller_class->handle_event = bobgui_event_controller_key_handle_event;
  controller_class->handle_crossing = bobgui_event_controller_key_handle_crossing;

  /**
   * BobguiEventControllerKey::key-pressed:
   * @controller: the object which received the signal.
   * @keyval: the pressed key.
   * @keycode: the raw code of the pressed key.
   * @state: the bitmask, representing the state of modifier keys and pointer buttons.
   *
   * Emitted whenever a key is pressed.
   *
   * Returns: %TRUE if the key press was handled, %FALSE otherwise.
   */
  signals[KEY_PRESSED] =
    g_signal_new (I_("key-pressed"),
                  BOBGUI_TYPE_EVENT_CONTROLLER_KEY,
                  G_SIGNAL_RUN_LAST,
                  0, _bobgui_boolean_handled_accumulator, NULL,
                  _bobgui_marshal_BOOLEAN__UINT_UINT_FLAGS,
                  G_TYPE_BOOLEAN, 3, G_TYPE_UINT, G_TYPE_UINT, GDK_TYPE_MODIFIER_TYPE);
  g_signal_set_va_marshaller (signals[KEY_PRESSED],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_BOOLEAN__UINT_UINT_FLAGSv);

  /**
   * BobguiEventControllerKey::key-released:
   * @controller: the object which received the signal.
   * @keyval: the released key.
   * @keycode: the raw code of the released key.
   * @state: the bitmask, representing the state of modifier keys and pointer buttons.
   *
   * Emitted whenever a key is released.
   */
  signals[KEY_RELEASED] =
    g_signal_new (I_("key-released"),
                  BOBGUI_TYPE_EVENT_CONTROLLER_KEY,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  _bobgui_marshal_VOID__UINT_UINT_FLAGS,
                  G_TYPE_NONE, 3, G_TYPE_UINT, G_TYPE_UINT, GDK_TYPE_MODIFIER_TYPE);
  g_signal_set_va_marshaller (signals[KEY_RELEASED],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_VOID__UINT_UINT_FLAGSv);

  /**
   * BobguiEventControllerKey::modifiers:
   * @controller: the object which received the signal.
   * @state: the bitmask, representing the new state of modifier keys and
   *   pointer buttons.
   *
   * Emitted whenever the state of modifier keys and pointer buttons change.
   *
   * Returns: whether to ignore modifiers
   */
  signals[MODIFIERS] =
    g_signal_new (I_("modifiers"),
                  BOBGUI_TYPE_EVENT_CONTROLLER_KEY,
                  G_SIGNAL_RUN_LAST,
                  0, NULL,
                  NULL,
                  _bobgui_marshal_BOOLEAN__FLAGS,
                  G_TYPE_BOOLEAN, 1, GDK_TYPE_MODIFIER_TYPE);
  g_signal_set_va_marshaller (signals[MODIFIERS],
                              G_TYPE_FROM_CLASS (klass),
                              _bobgui_marshal_BOOLEAN__FLAGSv);

  /**
   * BobguiEventControllerKey::im-update:
   * @controller: the object which received the signal
   *
   * Emitted whenever the input method context filters away
   * a keypress and prevents the @controller receiving it.
   *
   * See [method@Bobgui.EventControllerKey.set_im_context] and
   * [method@Bobgui.IMContext.filter_keypress].
   */
  signals[IM_UPDATE] =
    g_signal_new (I_("im-update"),
                  BOBGUI_TYPE_EVENT_CONTROLLER_KEY,
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);
}

static void
bobgui_event_controller_key_init (BobguiEventControllerKey *controller)
{
  controller->pressed_keys = g_hash_table_new (NULL, NULL);
}

/**
 * bobgui_event_controller_key_new:
 *
 * Creates a new event controller that will handle key events.
 *
 * Returns: a new `BobguiEventControllerKey`
 **/
BobguiEventController *
bobgui_event_controller_key_new (void)
{
  return g_object_new (BOBGUI_TYPE_EVENT_CONTROLLER_KEY, NULL);
}

/**
 * bobgui_event_controller_key_set_im_context:
 * @controller: a `BobguiEventControllerKey`
 * @im_context: (nullable): a `BobguiIMContext`
 *
 * Sets the input method context of the key @controller.
 */
void
bobgui_event_controller_key_set_im_context (BobguiEventControllerKey *controller,
                                         BobguiIMContext          *im_context)
{
  g_return_if_fail (BOBGUI_IS_EVENT_CONTROLLER_KEY (controller));
  g_return_if_fail (!im_context || BOBGUI_IS_IM_CONTEXT (im_context));

  if (controller->im_context)
    bobgui_im_context_reset (controller->im_context);

  g_set_object (&controller->im_context, im_context);
}

/**
 * bobgui_event_controller_key_get_im_context:
 * @controller: a `BobguiEventControllerKey`
 *
 * Gets the input method context of the key @controller.
 *
 * Returns: (transfer none) (nullable): the `BobguiIMContext`
 **/
BobguiIMContext *
bobgui_event_controller_key_get_im_context (BobguiEventControllerKey *controller)
{
  g_return_val_if_fail (BOBGUI_IS_EVENT_CONTROLLER_KEY (controller), NULL);

  return controller->im_context;
}

/**
 * bobgui_event_controller_key_forward:
 * @controller: a `BobguiEventControllerKey`
 * @widget: a `BobguiWidget`
 *
 * Forwards the current event of this @controller to a @widget.
 *
 * This function can only be used in handlers for the
 * [signal@Bobgui.EventControllerKey::key-pressed],
 * [signal@Bobgui.EventControllerKey::key-released]
 * or [signal@Bobgui.EventControllerKey::modifiers] signals.
 *
 * Returns: whether the @widget handled the event
 */
gboolean
bobgui_event_controller_key_forward (BobguiEventControllerKey *controller,
                                  BobguiWidget             *widget)
{
  g_return_val_if_fail (BOBGUI_IS_EVENT_CONTROLLER_KEY (controller), FALSE);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (widget), FALSE);
  g_return_val_if_fail (controller->current_event != NULL, FALSE);
  g_return_val_if_fail (gdk_event_get_event_type (controller->current_event) == GDK_KEY_PRESS ||
                        gdk_event_get_event_type (controller->current_event) == GDK_KEY_RELEASE, FALSE);

  if (!bobgui_widget_get_realized (widget))
    bobgui_widget_realize (widget);

  if (bobgui_widget_run_controllers (widget, controller->current_event, widget, 0, 0,
                                  BOBGUI_PHASE_CAPTURE))
    return TRUE;
  if (bobgui_widget_run_controllers (widget, controller->current_event, widget, 0, 0,
                                  BOBGUI_PHASE_TARGET))
    return TRUE;
  if (bobgui_widget_run_controllers (widget, controller->current_event, widget, 0, 0,
                                  BOBGUI_PHASE_BUBBLE))
    return TRUE;

  return FALSE;
}

/**
 * bobgui_event_controller_key_get_group:
 * @controller: a `BobguiEventControllerKey`
 *
 * Gets the key group of the current event of this @controller.
 *
 * See [method@Gdk.KeyEvent.get_layout].
 *
 * Returns: the key group
 */
guint
bobgui_event_controller_key_get_group (BobguiEventControllerKey *controller)
{
  g_return_val_if_fail (BOBGUI_IS_EVENT_CONTROLLER_KEY (controller), FALSE);
  g_return_val_if_fail (controller->current_event != NULL, FALSE);

  return gdk_key_event_get_layout (controller->current_event);
}
