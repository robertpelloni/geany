/*
 * Copyright © 2018 Benjamin Otte
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguishortcut.h"

#include "bobguishortcutaction.h"
#include "bobguishortcuttrigger.h"
#include "bobguiwidget.h"

/**
 * BobguiShortcut:
 *
 * Describes a keyboard shortcut.
 *
 * It contains a description of how to trigger the shortcut via a
 * [class@Bobgui.ShortcutTrigger] and a way to activate the shortcut
 * on a widget via a [class@Bobgui.ShortcutAction].
 *
 * The actual work is usually done via [class@Bobgui.ShortcutController],
 * which decides if and when to activate a shortcut. Using that controller
 * directly however is rarely necessary as various higher level
 * convenience APIs exist on `BobguiWidget`s that make it easier to use
 * shortcuts in BOBGUI.
 *
 * `BobguiShortcut` does provide functionality to make it easy for users
 * to work with shortcuts, either by providing informational strings
 * for display purposes or by allowing shortcuts to be configured.
 */

struct _BobguiShortcut
{
  GObject parent_instance;

  BobguiShortcutAction *action;
  BobguiShortcutTrigger *trigger;
  GVariant *args;
};

enum
{
  PROP_0,
  PROP_ACTION,
  PROP_ARGUMENTS,
  PROP_TRIGGER,

  N_PROPS
};

G_DEFINE_TYPE (BobguiShortcut, bobgui_shortcut, G_TYPE_OBJECT)

static GParamSpec *properties[N_PROPS] = { NULL, };

static void
bobgui_shortcut_dispose (GObject *object)
{
  BobguiShortcut *self = BOBGUI_SHORTCUT (object);

  g_clear_object (&self->action);
  g_clear_object (&self->trigger);
  g_clear_pointer (&self->args, g_variant_unref);

  G_OBJECT_CLASS (bobgui_shortcut_parent_class)->dispose (object);
}

static void
bobgui_shortcut_get_property (GObject    *object,
                           guint       property_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  BobguiShortcut *self = BOBGUI_SHORTCUT (object);

  switch (property_id)
    {
    case PROP_ACTION:
      g_value_set_object (value, self->action);
      break;

    case PROP_ARGUMENTS:
      g_value_set_variant (value, self->args);
      break;

    case PROP_TRIGGER:
      g_value_set_object (value, self->trigger);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_shortcut_set_property (GObject      *object,
                           guint         property_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  BobguiShortcut *self = BOBGUI_SHORTCUT (object);

  switch (property_id)
    {
    case PROP_ACTION:
      bobgui_shortcut_set_action (self, g_value_dup_object (value));
      break;

    case PROP_ARGUMENTS:
      bobgui_shortcut_set_arguments (self, g_value_get_variant (value));
      break;

    case PROP_TRIGGER:
      bobgui_shortcut_set_trigger (self, g_value_dup_object (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_shortcut_class_init (BobguiShortcutClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bobgui_shortcut_dispose;
  gobject_class->get_property = bobgui_shortcut_get_property;
  gobject_class->set_property = bobgui_shortcut_set_property;

  /**
   * BobguiShortcut:action:
   *
   * The action that gets activated by this shortcut.
   */
  properties[PROP_ACTION] =
    g_param_spec_object ("action", NULL, NULL,
                         BOBGUI_TYPE_SHORTCUT_ACTION,
                         G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS);

  /**
   * BobguiShortcut:arguments:
   *
   * Arguments passed to activation.
   */
  properties[PROP_ARGUMENTS] =
    g_param_spec_variant ("arguments", NULL, NULL,
                          G_VARIANT_TYPE_ANY,
                          NULL,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiShortcut:trigger:
   *
   * The trigger that triggers this shortcut.
   */
  properties[PROP_TRIGGER] =
    g_param_spec_object ("trigger", NULL, NULL,
                         BOBGUI_TYPE_SHORTCUT_TRIGGER,
                         G_PARAM_READWRITE |
                         G_PARAM_EXPLICIT_NOTIFY |
                         G_PARAM_STATIC_STRINGS);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);
}

static void
bobgui_shortcut_init (BobguiShortcut *self)
{
  self->action = g_object_ref (bobgui_nothing_action_get ());
  self->trigger = g_object_ref (bobgui_never_trigger_get ());
}

/**
 * bobgui_shortcut_new:
 * @trigger: (transfer full) (nullable): The trigger that will trigger the shortcut
 * @action: (transfer full) (nullable): The action that will be activated upon
 *    triggering
 *
 * Creates a new `BobguiShortcut` that is triggered by
 * @trigger and then activates @action.
 *
 * Returns: a new `BobguiShortcut`
 */
BobguiShortcut *
bobgui_shortcut_new (BobguiShortcutTrigger *trigger,
                  BobguiShortcutAction  *action)
{
  BobguiShortcut *shortcut;

  shortcut = g_object_new (BOBGUI_TYPE_SHORTCUT,
                           "action", action,
                           "trigger", trigger,
                           NULL);

  if (trigger)
    g_object_unref (trigger);
  if (action)
    g_object_unref (action);

  return shortcut;
}

/**
 * bobgui_shortcut_new_with_arguments: (skip)
 * @trigger: (transfer full) (nullable): The trigger that will trigger the shortcut
 * @action: (transfer full) (nullable): The action that will be activated upon
 *   triggering
 * @format_string: (nullable): GVariant format string for arguments or %NULL for
 *   no arguments
 * @...: arguments, as given by format string.
 *
 * Creates a new `BobguiShortcut` that is triggered by @trigger and then activates
 * @action with arguments given by @format_string.
 *
 * Returns: a new `BobguiShortcut`
 */
BobguiShortcut *
bobgui_shortcut_new_with_arguments (BobguiShortcutTrigger *trigger,
                                 BobguiShortcutAction  *action,
                                 const char         *format_string,
                                 ...)
{
  BobguiShortcut *shortcut;
  GVariant *args;

  if (format_string)
    {
      va_list valist;
      va_start (valist, format_string);
      args = g_variant_new_va (format_string, NULL, &valist);
      va_end (valist);
    }
  else
    {
      args = NULL;
    }

  shortcut = g_object_new (BOBGUI_TYPE_SHORTCUT,
                           "action", action,
                           "arguments", args,
                           "trigger", trigger,
                           NULL);

  if (trigger)
    g_object_unref (trigger);
  if (action)
    g_object_unref (action);

  return shortcut;
}

/**
 * bobgui_shortcut_get_action:
 * @self: a `BobguiShortcut`
 *
 * Gets the action that is activated by this shortcut.
 *
 * Returns: (transfer none) (nullable): the action
 */
BobguiShortcutAction *
bobgui_shortcut_get_action (BobguiShortcut *self)
{
  g_return_val_if_fail (BOBGUI_IS_SHORTCUT (self), NULL);

  return self->action;
}

/**
 * bobgui_shortcut_set_action:
 * @self: a `BobguiShortcut`
 * @action: (transfer full) (nullable): The new action.
 *   If the @action is %NULL, the nothing action will be used.
 *
 * Sets the new action for @self to be @action.
 */
void
bobgui_shortcut_set_action (BobguiShortcut *self,
                         BobguiShortcutAction *action)
{
  g_return_if_fail (BOBGUI_IS_SHORTCUT (self));

  if (action == NULL)
    action = g_object_ref (bobgui_nothing_action_get ());

  if (g_set_object (&self->action, action))
    {
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ACTION]);
      g_object_unref (action);
    }
}

/**
 * bobgui_shortcut_get_trigger:
 * @self: a `BobguiShortcut`
 *
 * Gets the trigger used to trigger @self.
 *
 * Returns: (transfer none) (nullable): the trigger used
 */
BobguiShortcutTrigger *
bobgui_shortcut_get_trigger (BobguiShortcut *self)
{
  g_return_val_if_fail (BOBGUI_IS_SHORTCUT (self), NULL);

  return self->trigger;
}

/**
 * bobgui_shortcut_set_trigger:
 * @self: a `BobguiShortcut`
 * @trigger: (transfer full) (nullable): The new trigger.
 *   If the @trigger is %NULL, the never trigger will be used.
 *
 * Sets the new trigger for @self to be @trigger.
 */
void
bobgui_shortcut_set_trigger (BobguiShortcut *self,
                          BobguiShortcutTrigger *trigger)
{
  g_return_if_fail (BOBGUI_IS_SHORTCUT (self));

  if (trigger == NULL)
    trigger = g_object_ref (bobgui_never_trigger_get ());

  if (g_set_object (&self->trigger, trigger))
    {
      g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TRIGGER]);
      g_object_unref (trigger);
    }
}

/**
 * bobgui_shortcut_get_arguments:
 * @self: a `BobguiShortcut`
 *
 * Gets the arguments that are passed when activating the shortcut.
 *
 * Returns: (transfer none) (nullable): the arguments
 */
GVariant *
bobgui_shortcut_get_arguments (BobguiShortcut *self)
{
  g_return_val_if_fail (BOBGUI_IS_SHORTCUT (self), NULL);

  return self->args;
}

/**
 * bobgui_shortcut_set_arguments:
 * @self: a `BobguiShortcut`
 * @args: (nullable): arguments to pass when activating @self
 *
 * Sets the arguments to pass when activating the shortcut.
 */
void
bobgui_shortcut_set_arguments (BobguiShortcut *self,
                            GVariant    *args)
{
  g_return_if_fail (BOBGUI_IS_SHORTCUT (self));

  if (self->args == args)
    return;
  
  g_clear_pointer (&self->args, g_variant_unref);
  if (args)
    self->args = g_variant_ref_sink (args);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ARGUMENTS]);
}
