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

/**
* BobguiShortcutAction:
*
* Encodes an action that can be triggered by a keyboard shortcut.
*
* `BobguiShortcutActions` contain functions that allow easy presentation
* to end users as well as being printed for debugging.
*
* All `BobguiShortcutActions` are immutable, you can only specify their
* properties during construction. If you want to change a action, you
* have to replace it with a new one. If you need to pass arguments to
* an action, these are specified by the higher-level `BobguiShortcut` object.
*
* To activate a `BobguiShortcutAction` manually, [method@Bobgui.ShortcutAction.activate]
* can be called.
*
* BOBGUI provides various actions:
*
*  - [class@Bobgui.MnemonicAction]: a shortcut action that calls
*    bobgui_widget_mnemonic_activate()
*  - [class@Bobgui.CallbackAction]: a shortcut action that invokes
*    a given callback
*  - [class@Bobgui.SignalAction]: a shortcut action that emits a
*    given signal
*  - [class@Bobgui.ActivateAction]: a shortcut action that calls
*    bobgui_widget_activate()
*  - [class@Bobgui.NamedAction]: a shortcut action that calls
*    bobgui_widget_activate_action()
*  - [class@Bobgui.NothingAction]: a shortcut action that does nothing
*/

#include "config.h"

#include "bobguishortcutactionprivate.h"

#include "bobguibuilder.h"
#include "bobguiwidgetprivate.h"
#include "bobguidebug.h"
#include "bobguiprivate.h"

/* {{{ BobguiShortcutAction */

struct _BobguiShortcutAction
{
GObject parent_instance;
};

struct _BobguiShortcutActionClass
{
  GObjectClass parent_class;

  gboolean      (* activate)    (BobguiShortcutAction            *action,
                                 BobguiShortcutActionFlags        flags,
                                 BobguiWidget                    *widget,
                                 GVariant                     *args);
  void          (* print)       (BobguiShortcutAction            *action,
                                 GString                      *string);
};

G_DEFINE_ABSTRACT_TYPE (BobguiShortcutAction, bobgui_shortcut_action, G_TYPE_OBJECT)

static void
bobgui_shortcut_action_class_init (BobguiShortcutActionClass *klass)
{
}

static void
bobgui_shortcut_action_init (BobguiShortcutAction *self)
{
}

/**
 * bobgui_shortcut_action_to_string:
 * @self: a `BobguiShortcutAction`
 *
 * Prints the given action into a human-readable string.
 *
 * This is a small wrapper around [method@Bobgui.ShortcutAction.print]
 * to help when debugging.
 *
 * Returns: (transfer full): a new string
 */
char *
bobgui_shortcut_action_to_string (BobguiShortcutAction *self)
{
  GString *string;

  g_return_val_if_fail (BOBGUI_IS_SHORTCUT_ACTION (self), NULL);

  string = g_string_new (NULL);
  bobgui_shortcut_action_print (self, string);

  return g_string_free (string, FALSE);
}

/**
 * bobgui_shortcut_action_print:
 * @self: a `BobguiShortcutAction`
 * @string: a `GString` to print into
 *
 * Prints the given action into a string for the developer.
 *
 * This is meant for debugging and logging.
 *
 * The form of the representation may change at any time and is
 * not guaranteed to stay identical.
 */
void
bobgui_shortcut_action_print (BobguiShortcutAction *self,
                           GString           *string)
{
  g_return_if_fail (BOBGUI_IS_SHORTCUT_ACTION (self));
  g_return_if_fail (string != NULL);

  BOBGUI_SHORTCUT_ACTION_GET_CLASS (self)->print (self, string);
}

/**
 * bobgui_shortcut_action_activate:
 * @self: a `BobguiShortcutAction`
 * @flags: flags to activate with
 * @widget: Target of the activation
 * @args: (nullable): arguments to pass
 *
 * Activates the action on the @widget with the given @args.
 *
 * Note that some actions ignore the passed in @flags, @widget or @args.
 *
 * Activation of an action can fail for various reasons. If the action
 * is not supported by the @widget, if the @args don't match the action
 * or if the activation otherwise had no effect, %FALSE will be returned.
 *
 * Returns: %TRUE if this action was activated successfully
 */
gboolean
bobgui_shortcut_action_activate (BobguiShortcutAction      *self,
                              BobguiShortcutActionFlags  flags,
                              BobguiWidget              *widget,
                              GVariant               *args)
{
  g_return_val_if_fail (BOBGUI_IS_SHORTCUT_ACTION (self), FALSE);
  g_return_val_if_fail (BOBGUI_IS_WIDGET (widget), FALSE);

  if (BOBGUI_DEBUG_CHECK (KEYBINDINGS))
    {
      char *act = bobgui_shortcut_action_to_string (self);
      gdk_debug_message ("Shortcut action activate on %s: %s", G_OBJECT_TYPE_NAME (widget), act);
      g_free (act);
    }

  return BOBGUI_SHORTCUT_ACTION_GET_CLASS (self)->activate (self, flags, widget, args);
}

static char *
string_is_function (const char *string,
                    const char *function_name)
{
  gsize len;

  if (!g_str_has_prefix (string, function_name))
    return NULL;
  string += strlen (function_name);

  if (string[0] != '(')
    return NULL;
  string ++;

  len = strlen (string);
  if (len == 0 || string[len - 1] != ')')
    return NULL;

  return g_strndup (string, len - 1);
}

/**
 * bobgui_shortcut_action_parse_string: (constructor)
 * @string: the string to parse
 *
 * Tries to parse the given string into an action.
 *
 * On success, the parsed action is returned. When parsing
 * failed, %NULL is returned.
 *
 * The accepted strings are:
 *
 * - `nothing`, for `BobguiNothingAction`
 * - `activate`, for `BobguiActivateAction`
 * - `mnemonic-activate`, for `BobguiMnemonicAction`
 * - `action(NAME)`, for a `BobguiNamedAction` for the action named `NAME`
 * - `signal(NAME)`, for a `BobguiSignalAction` for the signal `NAME`
 *
 * Returns: (nullable) (transfer full): a new `BobguiShortcutAction`
 */
BobguiShortcutAction *
bobgui_shortcut_action_parse_string (const char *string)
{
  BobguiShortcutAction *result;
  char *arg;

  if (g_str_equal (string, "nothing"))
    return g_object_ref (bobgui_nothing_action_get ());
  if (g_str_equal (string, "activate"))
    return g_object_ref (bobgui_activate_action_get ());
  if (g_str_equal (string, "mnemonic-activate"))
    return g_object_ref (bobgui_mnemonic_action_get ());

  if ((arg = string_is_function (string, "action")))
    {
      result = bobgui_named_action_new (arg);
      g_free (arg);
    }
  else if ((arg = string_is_function (string, "signal")))
    {
      result = bobgui_signal_action_new (arg);
      g_free (arg);
    }
  else
    return NULL;

  return result;
}

BobguiShortcutAction *
bobgui_shortcut_action_parse_builder (BobguiBuilder  *builder,
                                   const char  *string,
                                   GError     **error)
{
  BobguiShortcutAction *result;

  result = bobgui_shortcut_action_parse_string (string);

  if (!result)
    {
      g_set_error (error,
                   BOBGUI_BUILDER_ERROR, BOBGUI_BUILDER_ERROR_INVALID_VALUE,
                   "String \"%s\" does not specify a BobguiShortcutAction", string);
      return NULL;
    }

  return result;
}

/* }}} */

/* {{{ BobguiNothingAction */

struct _BobguiNothingAction
{
  BobguiShortcutAction parent_instance;
};

struct _BobguiNothingActionClass
{
  BobguiShortcutActionClass parent_class;
};

G_DEFINE_TYPE (BobguiNothingAction, bobgui_nothing_action, BOBGUI_TYPE_SHORTCUT_ACTION)

static void G_GNUC_NORETURN
bobgui_nothing_action_finalize (GObject *gobject)
{
  g_assert_not_reached ();

  G_OBJECT_CLASS (bobgui_nothing_action_parent_class)->finalize (gobject);
}

static gboolean
bobgui_nothing_action_activate (BobguiShortcutAction      *action,
                             BobguiShortcutActionFlags  flags,
                             BobguiWidget              *widget,
                             GVariant               *args)
{
  return FALSE;
}

static void
bobgui_nothing_action_print (BobguiShortcutAction *action,
                          GString           *string)
{
  g_string_append (string, "nothing");
}

static void
bobgui_nothing_action_class_init (BobguiNothingActionClass *klass)
{
  BobguiShortcutActionClass *action_class = BOBGUI_SHORTCUT_ACTION_CLASS (klass);

  G_OBJECT_CLASS (klass)->finalize = bobgui_nothing_action_finalize;

  action_class->activate = bobgui_nothing_action_activate;
  action_class->print = bobgui_nothing_action_print;
}

static void
bobgui_nothing_action_init (BobguiNothingAction *self)
{
}

/**
 * bobgui_nothing_action_get:
 *
 * Gets the nothing action.
 *
 * This is an action that does nothing and where
 * activating it always fails.
 *
 * Returns: (transfer none) (type BobguiNothingAction): The nothing action
 */
BobguiShortcutAction *
bobgui_nothing_action_get (void)
{
  static BobguiShortcutAction *nothing;

  if (nothing == NULL)
    nothing = g_object_new (BOBGUI_TYPE_NOTHING_ACTION, NULL);

  return nothing;
}

/* }}} */

/* {{{ BobguiCallbackAction */

struct _BobguiCallbackAction
{
  BobguiShortcutAction parent_instance;

  BobguiShortcutFunc callback;
  gpointer user_data;
  GDestroyNotify destroy_notify;
};

struct _BobguiCallbackActionClass
{
  BobguiShortcutActionClass parent_class;
};

G_DEFINE_TYPE (BobguiCallbackAction, bobgui_callback_action, BOBGUI_TYPE_SHORTCUT_ACTION)

static void
bobgui_callback_action_finalize (GObject *gobject)
{
  BobguiCallbackAction *self = BOBGUI_CALLBACK_ACTION (gobject);

  if (self->destroy_notify != NULL)
    self->destroy_notify (self->user_data);

  G_OBJECT_CLASS (bobgui_callback_action_parent_class)->finalize (gobject);
}

static gboolean
bobgui_callback_action_activate (BobguiShortcutAction      *action,
                              BobguiShortcutActionFlags  flags,
                              BobguiWidget              *widget,
                              GVariant               *args)
{
  BobguiCallbackAction *self = BOBGUI_CALLBACK_ACTION (action);

  return self->callback (widget, args, self->user_data);
}

static void
bobgui_callback_action_print (BobguiShortcutAction *action,
                           GString           *string)
{
  BobguiCallbackAction *self = BOBGUI_CALLBACK_ACTION (action);

  g_string_append_printf (string, "callback<%p>", self->callback);
}

static void
bobgui_callback_action_class_init (BobguiCallbackActionClass *klass)
{
  BobguiShortcutActionClass *action_class = BOBGUI_SHORTCUT_ACTION_CLASS (klass);

  G_OBJECT_CLASS (klass)->finalize = bobgui_callback_action_finalize;

  action_class->activate = bobgui_callback_action_activate;
  action_class->print = bobgui_callback_action_print;
}

static void
bobgui_callback_action_init (BobguiCallbackAction *self)
{
}

/**
 * bobgui_callback_action_new:
 * @callback: (scope notified) (closure data) (destroy destroy): the callback
 *   to call when the action is activated
 * @data: the data to be passed to @callback
 * @destroy: the function to be called when the callback action is finalized
 *
 * Create a custom action that calls the given @callback when
 * activated.
 *
 * Returns: (transfer full) (type BobguiCallbackAction): A new shortcut action
 */
BobguiShortcutAction *
bobgui_callback_action_new (BobguiShortcutFunc callback,
                         gpointer        data,
                         GDestroyNotify  destroy)
{
  BobguiCallbackAction *self;

  g_return_val_if_fail (callback != NULL, NULL);

  self = g_object_new (BOBGUI_TYPE_CALLBACK_ACTION, NULL);

  self->callback = callback;
  self->user_data = data;
  self->destroy_notify = destroy;

  return BOBGUI_SHORTCUT_ACTION (self);
}

/* }}} */

/* {{{ BobguiActivateAction */

struct _BobguiActivateAction
{
  BobguiShortcutAction parent_instance;
};

struct _BobguiActivateActionClass
{
  BobguiShortcutActionClass parent_class;
};

G_DEFINE_TYPE (BobguiActivateAction, bobgui_activate_action, BOBGUI_TYPE_SHORTCUT_ACTION)

static void G_GNUC_NORETURN
bobgui_activate_action_finalize (GObject *gobject)
{
  g_assert_not_reached ();

  G_OBJECT_CLASS (bobgui_activate_action_parent_class)->finalize (gobject);
}

static gboolean
bobgui_activate_action_activate (BobguiShortcutAction      *action,
                              BobguiShortcutActionFlags  flags,
                              BobguiWidget              *widget,
                              GVariant               *args)
{
  return bobgui_widget_activate (widget);
}

static void
bobgui_activate_action_print (BobguiShortcutAction *action,
                           GString           *string)
{
  g_string_append (string, "activate");
}

static void
bobgui_activate_action_class_init (BobguiActivateActionClass *klass)
{
  BobguiShortcutActionClass *action_class = BOBGUI_SHORTCUT_ACTION_CLASS (klass);

  G_OBJECT_CLASS (klass)->finalize = bobgui_activate_action_finalize;

  action_class->activate = bobgui_activate_action_activate;
  action_class->print = bobgui_activate_action_print;
}

static void
bobgui_activate_action_init (BobguiActivateAction *self)
{
}

/**
 * bobgui_activate_action_get:
 *
 * Gets the activate action.
 *
 * This is an action that calls bobgui_widget_activate()
 * on the given widget upon activation.
 *
 * Returns: (transfer none) (type BobguiActivateAction): The activate action
 */
BobguiShortcutAction *
bobgui_activate_action_get (void)
{
  static BobguiShortcutAction *action;

  if (action == NULL)
    action = g_object_new (BOBGUI_TYPE_ACTIVATE_ACTION, NULL);

  return action;
}

/* }}} */

/* {{{ BobguiMnemonicAction */

struct _BobguiMnemonicAction
{
  BobguiShortcutAction parent_instance;
};

struct _BobguiMnemonicActionClass
{
  BobguiShortcutActionClass parent_class;
};

G_DEFINE_TYPE (BobguiMnemonicAction, bobgui_mnemonic_action, BOBGUI_TYPE_SHORTCUT_ACTION)

static void G_GNUC_NORETURN
bobgui_mnemonic_action_finalize (GObject *gobject)
{
  g_assert_not_reached ();

  G_OBJECT_CLASS (bobgui_mnemonic_action_parent_class)->finalize (gobject);
}

static gboolean
bobgui_mnemonic_action_activate (BobguiShortcutAction      *action,
                              BobguiShortcutActionFlags  flags,
                              BobguiWidget              *widget,
                              GVariant               *args)
{
  return bobgui_widget_mnemonic_activate (widget, flags & BOBGUI_SHORTCUT_ACTION_EXCLUSIVE ? FALSE : TRUE);
}

static void
bobgui_mnemonic_action_print (BobguiShortcutAction *action,
                           GString           *string)
{
  g_string_append (string, "mnemonic-activate");
}

static void
bobgui_mnemonic_action_class_init (BobguiMnemonicActionClass *klass)
{
  BobguiShortcutActionClass *action_class = BOBGUI_SHORTCUT_ACTION_CLASS (klass);

  G_OBJECT_CLASS (klass)->finalize = bobgui_mnemonic_action_finalize;

  action_class->activate = bobgui_mnemonic_action_activate;
  action_class->print = bobgui_mnemonic_action_print;
}

static void
bobgui_mnemonic_action_init (BobguiMnemonicAction *self)
{
}

/**
 * bobgui_mnemonic_action_get:
 *
 * Gets the mnemonic action.
 *
 * This is an action that calls bobgui_widget_mnemonic_activate()
 * on the given widget upon activation.
 *
 * Returns: (transfer none) (type BobguiMnemonicAction): The mnemonic action
 */
BobguiShortcutAction *
bobgui_mnemonic_action_get (void)
{
  static BobguiShortcutAction *mnemonic;

  if (G_UNLIKELY (mnemonic == NULL))
    mnemonic = g_object_new (BOBGUI_TYPE_MNEMONIC_ACTION, NULL);

  return mnemonic;
}

/* }}} */

/* {{{ BobguiSignalAction */

struct _BobguiSignalAction
{
  BobguiShortcutAction parent_instance;

  const char *name; /* interned */
};

struct _BobguiSignalActionClass
{
  BobguiShortcutActionClass parent_class;
};

enum
{
  SIGNAL_PROP_SIGNAL_NAME = 1,
  SIGNAL_N_PROPS
};

static GParamSpec *signal_props[SIGNAL_N_PROPS];

G_DEFINE_TYPE (BobguiSignalAction, bobgui_signal_action, BOBGUI_TYPE_SHORTCUT_ACTION)

static void
bobgui_signal_action_finalize (GObject *gobject)
{
  //BobguiSignalAction *self = BOBGUI_SIGNAL_ACTION (gobject);

  G_OBJECT_CLASS (bobgui_signal_action_parent_class)->finalize (gobject);
}

static gboolean
binding_compose_params (BobguiWidget     *widget,
                        GVariantIter  *args,
                        GSignalQuery  *query,
                        GValue       **params_p)
{
  GValue *params;
  const GType *types;
  guint i;
  gboolean valid;

  params = g_new0 (GValue, query->n_params + 1);
  *params_p = params;

  /* The instance we emit on is the first object in the array
   */
  g_value_init (params, G_TYPE_OBJECT);
  g_value_set_object (params, G_OBJECT (widget));
  params++;

  types = query->param_types;
  valid = TRUE;
  for (i = 1; i < query->n_params + 1 && valid; i++)
    {
      GValue tmp_value = G_VALUE_INIT;
      GVariant *tmp_variant;

      g_value_init (params, *types);
      tmp_variant = g_variant_iter_next_value (args);

      switch ((guint) g_variant_classify (tmp_variant))
        {
        case G_VARIANT_CLASS_BOOLEAN:
          g_value_init (&tmp_value, G_TYPE_BOOLEAN);
          g_value_set_boolean (&tmp_value, g_variant_get_boolean (tmp_variant));
          break;
        case G_VARIANT_CLASS_DOUBLE:
          g_value_init (&tmp_value, G_TYPE_DOUBLE);
          g_value_set_double (&tmp_value, g_variant_get_double (tmp_variant));
          break;
        case G_VARIANT_CLASS_INT32:
          g_value_init (&tmp_value, G_TYPE_LONG);
          g_value_set_long (&tmp_value, g_variant_get_int32 (tmp_variant));
          break;
        case G_VARIANT_CLASS_UINT32:
          g_value_init (&tmp_value, G_TYPE_LONG);
          g_value_set_long (&tmp_value, g_variant_get_uint32 (tmp_variant));
          break;
        case G_VARIANT_CLASS_INT64:
          g_value_init (&tmp_value, G_TYPE_LONG);
          g_value_set_long (&tmp_value, g_variant_get_int64 (tmp_variant));
          break;
        case G_VARIANT_CLASS_STRING:
          /* bobgui_rc_parse_flags/enum() has fancier parsing for this; we can't call
           * that since we don't have a GParamSpec, so just do something simple
           */
          if (G_TYPE_FUNDAMENTAL (*types) == G_TYPE_ENUM)
            {
              GEnumClass *class = G_ENUM_CLASS (g_type_class_ref (*types));
              GEnumValue *enum_value;
              const char *s = g_variant_get_string (tmp_variant, NULL);

              valid = FALSE;

              enum_value = g_enum_get_value_by_name (class, s);
              if (!enum_value)
                enum_value = g_enum_get_value_by_nick (class, s);

              if (enum_value)
                {
                  g_value_init (&tmp_value, *types);
                  g_value_set_enum (&tmp_value, enum_value->value);
                  valid = TRUE;
                }

              g_type_class_unref (class);
            }
          /* This is just a hack for compatibility with BOBGUI 1.2 where a string
           * could be used for a single flag value / without the support for multiple
           * values in bobgui_rc_parse_flags(), this isn't very useful.
           */
          else if (G_TYPE_FUNDAMENTAL (*types) == G_TYPE_FLAGS)
            {
              GFlagsClass *class = G_FLAGS_CLASS (g_type_class_ref (*types));
              GFlagsValue *flags_value;
              const char *s = g_variant_get_string (tmp_variant, NULL);

              valid = FALSE;

              flags_value = g_flags_get_value_by_name (class, s);
              if (!flags_value)
                flags_value = g_flags_get_value_by_nick (class, s);
              if (flags_value)
                {
                  g_value_init (&tmp_value, *types);
                  g_value_set_flags (&tmp_value, flags_value->value);
                  valid = TRUE;
                }

              g_type_class_unref (class);
            }
          else
            {
              g_value_init (&tmp_value, G_TYPE_STRING);
              g_value_set_static_string (&tmp_value, g_variant_get_string (tmp_variant, NULL));
            }
          break;
        default:
          valid = FALSE;
          break;
        }

      if (valid)
        {
          if (!g_value_transform (&tmp_value, params))
            valid = FALSE;

          g_value_unset (&tmp_value);
        }

      g_variant_unref (tmp_variant);
      types++;
      params++;
    }

  if (!valid)
    {
      guint j;

      for (j = 0; j < i; j++)
        g_value_unset (&(*params_p)[j]);

      g_free (*params_p);
      *params_p = NULL;
    }

  return valid;
}

static gboolean
bobgui_signal_action_emit_signal (BobguiWidget   *widget,
                               const char  *signal,
                               GVariant    *args,
                               gboolean    *handled,
                               GError     **error)
{
  GSignalQuery query;
  guint signal_id;
  GValue *params = NULL;
  GValue return_val = G_VALUE_INIT;
  GVariantIter args_iter;
  gsize n_args;
  guint i;

  *handled = FALSE;

  signal_id = g_signal_lookup (signal, G_OBJECT_TYPE (widget));
  if (!signal_id)
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "Could not find signal \"%s\" in the '%s' class ancestry",
                   signal,
                   g_type_name (G_OBJECT_TYPE (widget)));
      return FALSE;
    }

  g_signal_query (signal_id, &query);
  if (args == NULL)
    n_args = 0;
  else if (g_variant_is_of_type (args, G_VARIANT_TYPE_TUPLE))
    n_args = g_variant_iter_init (&args_iter, args);
  else
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "argument GVariant is not a tuple");
      return FALSE;
    }
  if (query.n_params != n_args ||
      (query.return_type != G_TYPE_NONE && query.return_type != G_TYPE_BOOLEAN) ||
      !binding_compose_params (widget, &args_iter, &query, &params))
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "signature mismatch for signal \"%s\" in the '%s' class ancestry",
                   signal,
                   g_type_name (G_OBJECT_TYPE (widget)));
      return FALSE;
    }
  else if (!(query.signal_flags & G_SIGNAL_ACTION))
    {
      g_set_error (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                   "signal \"%s\" in the '%s' class ancestry cannot be used for action emissions",
                   signal,
                   g_type_name (G_OBJECT_TYPE (widget)));
      return FALSE;
    }

  if (query.return_type == G_TYPE_BOOLEAN)
    g_value_init (&return_val, G_TYPE_BOOLEAN);

  g_signal_emitv (params, signal_id, 0, &return_val);

  if (query.return_type == G_TYPE_BOOLEAN)
    {
      if (g_value_get_boolean (&return_val))
        *handled = TRUE;
      g_value_unset (&return_val);
    }
  else
    *handled = TRUE;

  if (params != NULL)
    {
      for (i = 0; i < query.n_params + 1; i++)
        g_value_unset (&params[i]);

      g_free (params);
    }

  return TRUE;
}

static gboolean
bobgui_signal_action_activate (BobguiShortcutAction      *action,
                            BobguiShortcutActionFlags  flags,
                            BobguiWidget              *widget,
                            GVariant               *args)
{
  BobguiSignalAction *self = BOBGUI_SIGNAL_ACTION (action);
  GError *error = NULL;
  gboolean handled;

  if (!bobgui_signal_action_emit_signal (widget,
                                      self->name,
                                      args,
                                      &handled,
                                      &error))
    {
      g_warning ("bobgui_signal_action_activate(): %s",
                 error->message);
      g_clear_error (&error);
      return FALSE;
    }

  return handled;
}

static void
bobgui_signal_action_print (BobguiShortcutAction *action,
                         GString           *string)
{
  BobguiSignalAction *self = BOBGUI_SIGNAL_ACTION (action);

  g_string_append_printf (string, "signal(%s)", self->name);
}

static void
bobgui_signal_action_constructed (GObject *gobject)
{
  BobguiSignalAction *self G_GNUC_UNUSED = BOBGUI_SIGNAL_ACTION (gobject);

  g_assert (self->name != NULL && self->name[0] != '\0');

  G_OBJECT_CLASS (bobgui_signal_action_parent_class)->constructed (gobject);
}

static void
bobgui_signal_action_set_property (GObject      *gobject,
                                guint         prop_id,
                                const GValue *value,
                                GParamSpec   *pspec)
{
  BobguiSignalAction *self = BOBGUI_SIGNAL_ACTION (gobject);

  switch (prop_id)
    {
    case SIGNAL_PROP_SIGNAL_NAME:
      self->name = g_intern_string (g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_signal_action_get_property (GObject    *gobject,
                                guint       prop_id,
                                GValue     *value,
                                GParamSpec *pspec)
{
  BobguiSignalAction *self = BOBGUI_SIGNAL_ACTION (gobject);

  switch (prop_id)
    {
    case SIGNAL_PROP_SIGNAL_NAME:
      g_value_set_string (value, self->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_signal_action_class_init (BobguiSignalActionClass *klass)
{
  BobguiShortcutActionClass *action_class = BOBGUI_SHORTCUT_ACTION_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = bobgui_signal_action_constructed;
  gobject_class->set_property = bobgui_signal_action_set_property;
  gobject_class->get_property = bobgui_signal_action_get_property;
  gobject_class->finalize = bobgui_signal_action_finalize;

  action_class->activate = bobgui_signal_action_activate;
  action_class->print = bobgui_signal_action_print;

  /**
   * BobguiSignalAction:signal-name:
   *
   * The name of the signal to emit.
   */
  signal_props[SIGNAL_PROP_SIGNAL_NAME] =
    g_param_spec_string (I_("signal-name"), NULL, NULL,
                         NULL,
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (gobject_class, SIGNAL_N_PROPS, signal_props);
}

static void
bobgui_signal_action_init (BobguiSignalAction *self)
{
}

/**
 * bobgui_signal_action_new:
 * @signal_name: name of the signal to emit
 *
 * Creates an action that when activated, emits the given action signal
 * on the provided widget.
 *
 * It will also unpack the args into arguments passed to the signal.
 *
 * Returns: (transfer full) (type BobguiSignalAction): a new `BobguiShortcutAction`
 */
BobguiShortcutAction *
bobgui_signal_action_new (const char *signal_name)
{
  BobguiShortcutAction *action;
  const char *name = "signal-name";
  GValue value = G_VALUE_INIT;

  g_return_val_if_fail (signal_name != NULL, NULL);

  g_value_init (&value, G_TYPE_STRING);
  g_value_set_static_string (&value, signal_name);

  action = BOBGUI_SHORTCUT_ACTION (g_object_new_with_properties (BOBGUI_TYPE_SIGNAL_ACTION,
                                                              1, &name, &value));

  g_value_unset (&value);

  return action;
}

/**
 * bobgui_signal_action_get_signal_name:
 * @self: a signal action
 *
 * Returns the name of the signal that will be emitted.
 *
 * Returns: (transfer none): the name of the signal to emit
 */
const char *
bobgui_signal_action_get_signal_name (BobguiSignalAction *self)
{
  g_return_val_if_fail (BOBGUI_IS_SIGNAL_ACTION (self), NULL);

  return self->name;
}

/* }}} */

/* {{{ BobguiNamedAction */

struct _BobguiNamedAction
{
  BobguiShortcutAction parent_instance;

  char *name;
};

struct _BobguiNamedActionClass
{
  BobguiShortcutActionClass parent_class;
};

enum
{
  NAMED_PROP_ACTION_NAME = 1,
  NAMED_N_PROPS
};

static GParamSpec *named_props[NAMED_N_PROPS];

G_DEFINE_TYPE (BobguiNamedAction, bobgui_named_action, BOBGUI_TYPE_SHORTCUT_ACTION)

static void
bobgui_named_action_finalize (GObject *gobject)
{
  BobguiNamedAction *self = BOBGUI_NAMED_ACTION (gobject);

  g_free (self->name);

  G_OBJECT_CLASS (bobgui_named_action_parent_class)->finalize (gobject);
}

static gboolean
check_parameter_type (GVariant           *args,
                      const GVariantType *parameter_type)
{
  if (args)
    {
      if (parameter_type == NULL)
        {
          g_warning ("Trying to invoke action with arguments, but action has no parameter");
          return FALSE;
        }

      if (!g_variant_is_of_type (args, parameter_type))
        {
          char *typestr = g_variant_type_dup_string (parameter_type);
          char *targetstr = g_variant_print (args, TRUE);
          g_warning ("Trying to invoke action with target '%s',"
                     " but action expects parameter with type '%s'", targetstr, typestr);
          g_free (targetstr);
          g_free (typestr);
          return FALSE;
        }
    }
  else
    {
      if (parameter_type != NULL)
        {
          char *typestr = g_variant_type_dup_string (parameter_type);
          g_warning ("Trying to invoke action without arguments,"
                     " but action expects parameter with type '%s'", typestr);
          g_free (typestr);
          return FALSE;
        }
    }

  return TRUE;
}

static gboolean
bobgui_named_action_activate (BobguiShortcutAction      *action,
                           BobguiShortcutActionFlags  flags,
                           BobguiWidget              *widget,
                           GVariant               *args)
{
  BobguiNamedAction *self = BOBGUI_NAMED_ACTION (action);
  const GVariantType *parameter_type;
  BobguiActionMuxer *muxer;
  gboolean enabled;

  muxer = _bobgui_widget_get_action_muxer (widget, FALSE);
  if (muxer == NULL)
    return FALSE;

  if (!bobgui_action_muxer_query_action (muxer, self->name,
                                      &enabled, &parameter_type,
                                      NULL, NULL, NULL))
    return FALSE;

  if (!enabled)
    return FALSE;

  /* We found an action with the correct name and it's enabled.
   * This is the action that we are going to try to invoke.
   *
   * There is still the possibility that the args don't
   * match the expected parameter type.  In that case, we will print
   * a warning.
   */
  if (!check_parameter_type (args, parameter_type))
    return FALSE;

  bobgui_action_muxer_activate_action (muxer, self->name, args);

  return TRUE;
}

static void
bobgui_named_action_print (BobguiShortcutAction *action,
                        GString           *string)
{
  BobguiNamedAction *self = BOBGUI_NAMED_ACTION (action);

  g_string_append_printf (string, "action(%s)", self->name);
}

static void
bobgui_named_action_set_property (GObject      *gobject,
                               guint         prop_id,
                               const GValue *value,
                               GParamSpec   *pspec)
{
  BobguiNamedAction *self = BOBGUI_NAMED_ACTION (gobject);

  switch (prop_id)
    {
    case NAMED_PROP_ACTION_NAME:
      self->name = g_value_dup_string (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_named_action_get_property (GObject    *gobject,
                               guint       prop_id,
                               GValue     *value,
                               GParamSpec *pspec)
{
  BobguiNamedAction *self = BOBGUI_NAMED_ACTION (gobject);

  switch (prop_id)
    {
    case NAMED_PROP_ACTION_NAME:
      g_value_set_string (value, self->name);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_named_action_constructed (GObject *gobject)
{
  BobguiNamedAction *self G_GNUC_UNUSED = BOBGUI_NAMED_ACTION (gobject);

  g_assert (self->name != NULL && self->name[0] != '\0');

  G_OBJECT_CLASS (bobgui_named_action_parent_class)->constructed (gobject);
}

static void
bobgui_named_action_class_init (BobguiNamedActionClass *klass)
{
  BobguiShortcutActionClass *action_class = BOBGUI_SHORTCUT_ACTION_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = bobgui_named_action_constructed;
  gobject_class->set_property = bobgui_named_action_set_property;
  gobject_class->get_property = bobgui_named_action_get_property;
  gobject_class->finalize = bobgui_named_action_finalize;

  action_class->activate = bobgui_named_action_activate;
  action_class->print = bobgui_named_action_print;

  /**
   * BobguiNamedAction:action-name:
   *
   * The name of the action to activate.
   */
  named_props[NAMED_PROP_ACTION_NAME] =
    g_param_spec_string (I_("action-name"), NULL, NULL,
                         NULL,
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_READWRITE |
                         G_PARAM_CONSTRUCT_ONLY);

  g_object_class_install_properties (gobject_class, NAMED_N_PROPS, named_props);
}

static void
bobgui_named_action_init (BobguiNamedAction *self)
{
}

/**
 * bobgui_named_action_new:
 * @name: the detailed name of the action
 *
 * Creates an action that when activated, activates
 * the named action on the widget.
 *
 * It also passes the given arguments to it.
 *
 * See [method@Bobgui.Widget.insert_action_group] for
 * how to add actions to widgets.
 *
 * Returns: (transfer full) (type BobguiNamedAction): a new `BobguiShortcutAction`
 */
BobguiShortcutAction *
bobgui_named_action_new (const char *name)
{
  g_return_val_if_fail (name != NULL, NULL);

  return g_object_new (BOBGUI_TYPE_NAMED_ACTION,
                       "action-name", name,
                       NULL);
}

/**
 * bobgui_named_action_get_action_name:
 * @self: a named action
 *
 * Returns the name of the action that will be activated.
 *
 * Returns: the name of the action to activate
 */
const char *
bobgui_named_action_get_action_name (BobguiNamedAction *self)
{
  g_return_val_if_fail (BOBGUI_IS_NAMED_ACTION (self), NULL);

  return self->name;
}
