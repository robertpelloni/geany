/*
 * Copyright © 2018 Benjamin Otte
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
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
 * BobguiShortcutTrigger:
 *
 * Tracks how a `BobguiShortcut` can be activated.
 *
 * To find out if a `BobguiShortcutTrigger` triggers, you can call
 * [method@Bobgui.ShortcutTrigger.trigger] on a `GdkEvent`.
 *
 * `BobguiShortcutTriggers` contain functions that allow easy presentation
 * to end users as well as being printed for debugging.
 *
 * All `BobguiShortcutTriggers` are immutable, you can only specify their
 * properties during construction. If you want to change a trigger, you
 * have to replace it with a new one.
 */

#include "config.h"

#include "bobguishortcuttrigger.h"

#include "bobguiaccelgroupprivate.h"
#include "bobguiprivate.h"

#define BOBGUI_SHORTCUT_TRIGGER_HASH_NEVER         0u
#define BOBGUI_SHORTCUT_TRIGGER_HASH_KEYVAL        1u
#define BOBGUI_SHORTCUT_TRIGGER_HASH_MNEMONIC      2u
#define BOBGUI_SHORTCUT_TRIGGER_HASH_ALTERNATIVE   3u

struct _BobguiShortcutTrigger
{
  GObject parent_instance;
};

struct _BobguiShortcutTriggerClass
{
  GObjectClass parent_class;

  GdkKeyMatch     (* trigger)     (BobguiShortcutTrigger  *trigger,
                                   GdkEvent            *event,
                                   gboolean             enable_mnemonics);
  guint           (* hash)        (BobguiShortcutTrigger  *trigger);
  int             (* compare)     (BobguiShortcutTrigger  *trigger1,
                                   BobguiShortcutTrigger  *trigger2);
  void            (* print)       (BobguiShortcutTrigger  *trigger,
                                   GString             *string);
  gboolean        (* print_label) (BobguiShortcutTrigger  *trigger,
                                   GdkDisplay          *display,
                                   GString             *string);
};

G_DEFINE_ABSTRACT_TYPE (BobguiShortcutTrigger, bobgui_shortcut_trigger, G_TYPE_OBJECT)

static void
bobgui_shortcut_trigger_class_init (BobguiShortcutTriggerClass *klass)
{
}

static void
bobgui_shortcut_trigger_init (BobguiShortcutTrigger *self)
{
}

/**
 * bobgui_shortcut_trigger_trigger:
 * @self: a `BobguiShortcutTrigger`
 * @event: the event to check
 * @enable_mnemonics: %TRUE if mnemonics should trigger. Usually the
 *   value of this property is determined by checking that the passed
 *   in @event is a Key event and has the right modifiers set.
 *
 * Checks if the given @event triggers @self.
 *
 * Returns: Whether the event triggered the shortcut
 */
GdkKeyMatch
bobgui_shortcut_trigger_trigger (BobguiShortcutTrigger *self,
                              GdkEvent           *event,
                              gboolean            enable_mnemonics)
{
  g_return_val_if_fail (BOBGUI_IS_SHORTCUT_TRIGGER (self), GDK_KEY_MATCH_NONE);

  return BOBGUI_SHORTCUT_TRIGGER_GET_CLASS (self)->trigger (self, event, enable_mnemonics);
}

/**
 * bobgui_shortcut_trigger_parse_string: (constructor)
 * @string: the string to parse
 *
 * Tries to parse the given string into a trigger.
 *
 * On success, the parsed trigger is returned.
 * When parsing failed, %NULL is returned.
 *
 * The accepted strings are:
 *
 *   - `never`, for `BobguiNeverTrigger`
 *   - a string parsed by bobgui_accelerator_parse(), for a `BobguiKeyvalTrigger`, e.g. `<Control>C`
 *   - underscore, followed by a single character, for `BobguiMnemonicTrigger`, e.g. `_l`
 *   - two valid trigger strings, separated by a `|` character, for a
 *     `BobguiAlternativeTrigger`: `<Control>q|<Control>w`
 *
 * Note that you will have to escape the `<` and `>` characters when specifying
 * triggers in XML files, such as BobguiBuilder ui files. Use `&lt;` instead of
 * `<` and `&gt;` instead of `>`.
 *
 * Returns: (nullable) (transfer full): a new `BobguiShortcutTrigger`
 */
BobguiShortcutTrigger *
bobgui_shortcut_trigger_parse_string (const char *string)
{
  GdkModifierType modifiers;
  guint keyval;
  const char *sep;

  g_return_val_if_fail (string != NULL, NULL);

  if ((sep = strchr (string, '|')) != NULL)
    {
      char *frag_a = g_strndup (string, sep - string);
      const char *frag_b = sep + 1;
      BobguiShortcutTrigger *t1, *t2;

      /* empty first slot */
      if (*frag_a == '\0')
        {
          g_free (frag_a);
          return NULL;
        }

      /* empty second slot */
      if (*frag_b == '\0')
        {
          g_free (frag_a);
          return NULL;
        }

      t1 = bobgui_shortcut_trigger_parse_string (frag_a);
      if (t1 == NULL)
        {
          g_free (frag_a);
          return NULL;
        }

      t2 = bobgui_shortcut_trigger_parse_string (frag_b);
      if (t2 == NULL)
        {
          g_object_unref (t1);
          g_free (frag_a);
          return NULL;
        }

      g_free (frag_a);

      return bobgui_alternative_trigger_new (t1, t2);
    }

  if (g_str_equal (string, "never"))
    return g_object_ref (bobgui_never_trigger_get ());

  if (string[0] == '_')
    {
      keyval = gdk_keyval_from_name (string + 1);
      if (keyval != GDK_KEY_VoidSymbol)
        return bobgui_mnemonic_trigger_new (gdk_keyval_to_lower (keyval));
    }

  if (bobgui_accelerator_parse (string, &keyval, &modifiers))
    return bobgui_keyval_trigger_new (keyval, modifiers);

  return NULL;
}

/**
 * bobgui_shortcut_trigger_to_string:
 * @self: a `BobguiShortcutTrigger`
 *
 * Prints the given trigger into a human-readable string.
 *
 * This is a small wrapper around [method@Bobgui.ShortcutTrigger.print]
 * to help when debugging.
 *
 * Returns: (transfer full): a new string
 */
char *
bobgui_shortcut_trigger_to_string (BobguiShortcutTrigger *self)
{
  GString *string;

  g_return_val_if_fail (self != NULL, NULL);

  string = g_string_new (NULL);

  bobgui_shortcut_trigger_print (self, string);

  return g_string_free (string, FALSE);
}

/**
 * bobgui_shortcut_trigger_print:
 * @self: a `BobguiShortcutTrigger`
 * @string: a `GString` to print into
 *
 * Prints the given trigger into a string for the developer.
 * This is meant for debugging and logging.
 *
 * The form of the representation may change at any time
 * and is not guaranteed to stay identical.
 */
void
bobgui_shortcut_trigger_print (BobguiShortcutTrigger *self,
                            GString            *string)
{
  g_return_if_fail (BOBGUI_IS_SHORTCUT_TRIGGER (self));
  g_return_if_fail (string != NULL);

  BOBGUI_SHORTCUT_TRIGGER_GET_CLASS (self)->print (self, string);
}

/**
 * bobgui_shortcut_trigger_to_label:
 * @self: a `BobguiShortcutTrigger`
 * @display: `GdkDisplay` to print for
 *
 * Gets textual representation for the given trigger.
 *
 * This function is returning a translated string for
 * presentation to end users for example in menu items
 * or in help texts.
 *
 * The @display in use may influence the resulting string in
 * various forms, such as resolving hardware keycodes or by
 * causing display-specific modifier names.
 *
 * The form of the representation may change at any time and is
 * not guaranteed to stay identical.
 *
 * Returns: (transfer full): a new string
 */
char *
bobgui_shortcut_trigger_to_label (BobguiShortcutTrigger *self,
                               GdkDisplay         *display)
{
  GString *string;

  g_return_val_if_fail (self != NULL, NULL);

  string = g_string_new (NULL);
  bobgui_shortcut_trigger_print_label (self, display, string);

  return g_string_free (string, FALSE);
}

/**
 * bobgui_shortcut_trigger_print_label:
 * @self: a `BobguiShortcutTrigger`
 * @display: `GdkDisplay` to print for
 * @string: a `GString` to print into
 *
 * Prints the given trigger into a string.
 *
 * This function is returning a translated string for presentation
 * to end users for example in menu items or in help texts.
 *
 * The @display in use may influence the resulting string in
 * various forms, such as resolving hardware keycodes or by
 * causing display-specific modifier names.
 *
 * The form of the representation may change at any time and is
 * not guaranteed to stay identical.
 *
 * Returns: %TRUE if something was printed or %FALSE if the
 *   trigger did not have a textual representation suitable
 *   for end users.
 **/
gboolean
bobgui_shortcut_trigger_print_label (BobguiShortcutTrigger *self,
                                  GdkDisplay         *display,
                                  GString            *string)
{
  g_return_val_if_fail (BOBGUI_IS_SHORTCUT_TRIGGER (self), FALSE);
  g_return_val_if_fail (GDK_IS_DISPLAY (display), FALSE);
  g_return_val_if_fail (string != NULL, FALSE);

  return BOBGUI_SHORTCUT_TRIGGER_GET_CLASS (self)->print_label (self, display, string);
}

/**
 * bobgui_shortcut_trigger_hash:
 * @trigger: (type BobguiShortcutTrigger): a `BobguiShortcutTrigger`
 *
 * Generates a hash value for a `BobguiShortcutTrigger`.
 *
 * The output of this function is guaranteed to be the same for a given
 * value only per-process. It may change between different processor
 * architectures or even different versions of BOBGUI. Do not use this
 * function as a basis for building protocols or file formats.
 *
 * The types of @trigger is `gconstpointer` only to allow use of this
 * function with `GHashTable`. They must each be a `BobguiShortcutTrigger`.
 *
 * Returns: a hash value corresponding to @trigger
 */
guint
bobgui_shortcut_trigger_hash (gconstpointer trigger)
{
  BobguiShortcutTrigger *t = (BobguiShortcutTrigger *) trigger;

  g_return_val_if_fail (BOBGUI_IS_SHORTCUT_TRIGGER (t), 0);

  return BOBGUI_SHORTCUT_TRIGGER_GET_CLASS (t)->hash (t);
}

/**
 * bobgui_shortcut_trigger_equal:
 * @trigger1: (type BobguiShortcutTrigger): a `BobguiShortcutTrigger`
 * @trigger2: (type BobguiShortcutTrigger): a `BobguiShortcutTrigger`
 *
 * Checks if @trigger1 and @trigger2 trigger under the same conditions.
 *
 * The types of @one and @two are `gconstpointer` only to allow use of this
 * function with `GHashTable`. They must each be a `BobguiShortcutTrigger`.
 *
 * Returns: %TRUE if @trigger1 and @trigger2 are equal
 */
gboolean
bobgui_shortcut_trigger_equal (gconstpointer trigger1,
                            gconstpointer trigger2)
{
  return bobgui_shortcut_trigger_compare (trigger1, trigger2) == 0;
}

/**
 * bobgui_shortcut_trigger_compare:
 * @trigger1: (type BobguiShortcutTrigger): a `BobguiShortcutTrigger`
 * @trigger2: (type BobguiShortcutTrigger): a `BobguiShortcutTrigger`
 *
 * The types of @trigger1 and @trigger2 are `gconstpointer` only to allow
 * use of this function as a `GCompareFunc`.
 *
 * They must each be a `BobguiShortcutTrigger`.
 *
 * Returns: An integer less than, equal to, or greater than zero if
 *   @trigger1 is found, respectively, to be less than, to match,
 *   or be greater than @trigger2.
 */
int
bobgui_shortcut_trigger_compare (gconstpointer trigger1,
                              gconstpointer trigger2)
{
  BobguiShortcutTrigger *t1 = (BobguiShortcutTrigger *) trigger1;
  BobguiShortcutTrigger *t2 = (BobguiShortcutTrigger *) trigger2;
  GType type1, type2;

  g_return_val_if_fail (BOBGUI_IS_SHORTCUT_TRIGGER (t1), -1);
  g_return_val_if_fail (BOBGUI_IS_SHORTCUT_TRIGGER (t2), 1);

  type1 = G_OBJECT_TYPE (t1);
  type2 = G_OBJECT_TYPE (t2);

  if (type1 == type2)
    {
      return BOBGUI_SHORTCUT_TRIGGER_GET_CLASS (t1)->compare (t1, t2);
    }
  else
    { /* never < keyval < mnemonic < alternative */
      if (type1 == BOBGUI_TYPE_NEVER_TRIGGER ||
          type2 == BOBGUI_TYPE_ALTERNATIVE_TRIGGER)
        return -1;
      if (type2 == BOBGUI_TYPE_NEVER_TRIGGER ||
          type1 == BOBGUI_TYPE_ALTERNATIVE_TRIGGER)
        return 1;

      if (type1 == BOBGUI_TYPE_KEYVAL_TRIGGER)
        return -1;
      else
        return 1;
    }
}

struct _BobguiNeverTrigger
{
  BobguiShortcutTrigger parent_instance;
};

struct _BobguiNeverTriggerClass
{
  BobguiShortcutTriggerClass parent_class;
};

G_DEFINE_TYPE (BobguiNeverTrigger, bobgui_never_trigger, BOBGUI_TYPE_SHORTCUT_TRIGGER)

static BobguiShortcutTrigger *never_singleton;

static void G_GNUC_NORETURN
bobgui_never_trigger_finalize (GObject *gobject)
{
  g_assert_not_reached ();

  G_OBJECT_CLASS (bobgui_never_trigger_parent_class)->finalize (gobject);
}

static GObject *
bobgui_never_trigger_constructor (GType                  type,
                               guint                  n_construct_params,
                               GObjectConstructParam *construct_params)
{
  if (G_UNLIKELY (never_singleton == NULL))
    {
      GObjectClass *parent_class = G_OBJECT_CLASS (bobgui_never_trigger_parent_class);
      never_singleton = BOBGUI_SHORTCUT_TRIGGER (parent_class->constructor (type,
                                                                         n_construct_params,
                                                                         construct_params));
    }
  return g_object_ref (G_OBJECT (never_singleton));
}

static GdkKeyMatch
bobgui_never_trigger_trigger (BobguiShortcutTrigger *trigger,
                           GdkEvent           *event,
                           gboolean            enable_mnemonics)
{
  return GDK_KEY_MATCH_NONE;
}

static guint
bobgui_never_trigger_hash (BobguiShortcutTrigger *trigger)
{
  return BOBGUI_SHORTCUT_TRIGGER_HASH_NEVER;
}

static int
bobgui_never_trigger_compare (BobguiShortcutTrigger  *trigger1,
                           BobguiShortcutTrigger  *trigger2)
{
  return 0;
}

static void
bobgui_never_trigger_print (BobguiShortcutTrigger *trigger,
                         GString            *string)
{
  g_string_append (string, "never");
}

static gboolean
bobgui_never_trigger_print_label (BobguiShortcutTrigger *trigger,
                               GdkDisplay         *display,
                               GString            *string)
{
  return FALSE;
}

static void
bobgui_never_trigger_class_init (BobguiNeverTriggerClass *klass)
{
  BobguiShortcutTriggerClass *trigger_class = BOBGUI_SHORTCUT_TRIGGER_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructor = bobgui_never_trigger_constructor;
  gobject_class->finalize = bobgui_never_trigger_finalize;

  trigger_class->trigger = bobgui_never_trigger_trigger;
  trigger_class->hash = bobgui_never_trigger_hash;
  trigger_class->compare = bobgui_never_trigger_compare;
  trigger_class->print = bobgui_never_trigger_print;
  trigger_class->print_label = bobgui_never_trigger_print_label;
}

static void
bobgui_never_trigger_init (BobguiNeverTrigger *self)
{
  /* We are the singleton being constructed */
  g_assert (!never_singleton);
}

/**
 * bobgui_never_trigger_get:
 *
 * Gets the never trigger.
 *
 * This is a singleton for a trigger that never triggers.
 * Use this trigger instead of %NULL because it implements
 * all virtual functions.
 *
 * Returns: (type BobguiNeverTrigger) (transfer none): The never trigger
 */
BobguiShortcutTrigger *
bobgui_never_trigger_get (void)
{
  if (G_UNLIKELY (never_singleton == NULL))
    {
      BobguiShortcutTrigger *never = g_object_new (BOBGUI_TYPE_NEVER_TRIGGER, NULL);
      g_assert (never == never_singleton);
      g_object_unref (never);
    }

  return never_singleton;
}

struct _BobguiKeyvalTrigger
{
  BobguiShortcutTrigger parent_instance;

  guint keyval;
  GdkModifierType modifiers;
};

struct _BobguiKeyvalTriggerClass
{
  BobguiShortcutTriggerClass parent_class;
};

G_DEFINE_TYPE (BobguiKeyvalTrigger, bobgui_keyval_trigger, BOBGUI_TYPE_SHORTCUT_TRIGGER)

enum
{
  KEYVAL_PROP_KEYVAL = 1,
  KEYVAL_PROP_MODIFIERS,
  KEYVAL_N_PROPS
};

static GParamSpec *keyval_props[KEYVAL_N_PROPS];

static GdkKeyMatch
bobgui_keyval_trigger_trigger (BobguiShortcutTrigger *trigger,
                            GdkEvent           *event,
                            gboolean            enable_mnemonics)
{
  BobguiKeyvalTrigger *self = BOBGUI_KEYVAL_TRIGGER (trigger);

  if (gdk_event_get_event_type (event) != GDK_KEY_PRESS)
    return GDK_KEY_MATCH_NONE;

  return gdk_key_event_matches (event, self->keyval, self->modifiers);
}

static guint
bobgui_keyval_trigger_hash (BobguiShortcutTrigger *trigger)
{
  BobguiKeyvalTrigger *self = BOBGUI_KEYVAL_TRIGGER (trigger);

  return (self->modifiers << 24)
       | (self->modifiers >> 8)
       | (self->keyval << 16)
       | BOBGUI_SHORTCUT_TRIGGER_HASH_KEYVAL;
}

static int
bobgui_keyval_trigger_compare (BobguiShortcutTrigger  *trigger1,
                            BobguiShortcutTrigger  *trigger2)
{
  BobguiKeyvalTrigger *self1 = BOBGUI_KEYVAL_TRIGGER (trigger1);
  BobguiKeyvalTrigger *self2 = BOBGUI_KEYVAL_TRIGGER (trigger2);

  if (self1->modifiers != self2->modifiers)
    return self2->modifiers - self1->modifiers;

  return self1->keyval - self2->keyval;
}

static void
bobgui_keyval_trigger_print (BobguiShortcutTrigger *trigger,
                          GString            *string)
{
  BobguiKeyvalTrigger *self = BOBGUI_KEYVAL_TRIGGER (trigger);
  char *accelerator_name;

  accelerator_name = bobgui_accelerator_name (self->keyval, self->modifiers);
  g_string_append (string, accelerator_name);
  g_free (accelerator_name);
}

static gboolean
bobgui_keyval_trigger_print_label (BobguiShortcutTrigger *trigger,
                                GdkDisplay         *display,
                                GString            *string)
{
  BobguiKeyvalTrigger *self = BOBGUI_KEYVAL_TRIGGER (trigger);

  bobgui_accelerator_print_label (string, self->keyval, self->modifiers);

  return TRUE;
}

static void
bobgui_keyval_trigger_set_property (GObject      *gobject,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  BobguiKeyvalTrigger *self = BOBGUI_KEYVAL_TRIGGER (gobject);

  switch (prop_id)
    {
    case KEYVAL_PROP_KEYVAL:
      {
        guint keyval = g_value_get_uint (value);

        /* We store keyvals as lower key */
        if (keyval == GDK_KEY_ISO_Left_Tab)
          self->keyval = GDK_KEY_Tab;
        else
          self->keyval = gdk_keyval_to_lower (keyval);
      }
      break;

    case KEYVAL_PROP_MODIFIERS:
      self->modifiers = g_value_get_flags (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_keyval_trigger_get_property (GObject    *gobject,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  BobguiKeyvalTrigger *self = BOBGUI_KEYVAL_TRIGGER (gobject);

  switch (prop_id)
    {
    case KEYVAL_PROP_KEYVAL:
      g_value_set_uint (value, self->keyval);
      break;

    case KEYVAL_PROP_MODIFIERS:
      g_value_set_flags (value, self->modifiers);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_keyval_trigger_class_init (BobguiKeyvalTriggerClass *klass)
{
  BobguiShortcutTriggerClass *trigger_class = BOBGUI_SHORTCUT_TRIGGER_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bobgui_keyval_trigger_set_property;
  gobject_class->get_property = bobgui_keyval_trigger_get_property;

  trigger_class->trigger = bobgui_keyval_trigger_trigger;
  trigger_class->hash = bobgui_keyval_trigger_hash;
  trigger_class->compare = bobgui_keyval_trigger_compare;
  trigger_class->print = bobgui_keyval_trigger_print;
  trigger_class->print_label = bobgui_keyval_trigger_print_label;

  /**
   * BobguiKeyvalTrigger:keyval:
   *
   * The key value for the trigger.
   */
  keyval_props[KEYVAL_PROP_KEYVAL] =
    g_param_spec_uint (I_("keyval"), NULL, NULL,
                       0, G_MAXINT,
                       0,
                       G_PARAM_STATIC_STRINGS |
                       G_PARAM_CONSTRUCT_ONLY |
                       G_PARAM_READWRITE);

  /**
   * BobguiKeyvalTrigger:modifiers:
   *
   * The key modifiers for the trigger.
   */
  keyval_props[KEYVAL_PROP_MODIFIERS] =
    g_param_spec_flags (I_("modifiers"), NULL, NULL,
                        GDK_TYPE_MODIFIER_TYPE,
                        GDK_NO_MODIFIER_MASK,
                        G_PARAM_STATIC_STRINGS |
                        G_PARAM_CONSTRUCT_ONLY |
                        G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, KEYVAL_N_PROPS, keyval_props);
}

static void
bobgui_keyval_trigger_init (BobguiKeyvalTrigger *self)
{
}

/**
 * bobgui_keyval_trigger_new:
 * @keyval: The keyval to trigger for
 * @modifiers: the modifiers that need to be present
 *
 * Creates a `BobguiShortcutTrigger` that will trigger whenever
 * the key with the given @keyval and @modifiers is pressed.
 *
 * Returns: A new `BobguiShortcutTrigger`
 */
BobguiShortcutTrigger *
bobgui_keyval_trigger_new (guint           keyval,
                        GdkModifierType modifiers)
{
  return g_object_new (BOBGUI_TYPE_KEYVAL_TRIGGER,
                       "keyval", keyval,
                       "modifiers", modifiers,
                       NULL);
}

/**
 * bobgui_keyval_trigger_get_modifiers:
 * @self: a keyval `BobguiShortcutTrigger`
 *
 * Gets the modifiers that must be present to succeed
 * triggering @self.
 *
 * Returns: the modifiers
 */
GdkModifierType
bobgui_keyval_trigger_get_modifiers (BobguiKeyvalTrigger *self)
{
  g_return_val_if_fail (BOBGUI_IS_KEYVAL_TRIGGER (self), 0);

  return self->modifiers;
}

/**
 * bobgui_keyval_trigger_get_keyval:
 * @self: a keyval `BobguiShortcutTrigger`
 *
 * Gets the keyval that must be pressed to succeed
 * triggering @self.
 *
 * Returns: the keyval
 */
guint
bobgui_keyval_trigger_get_keyval (BobguiKeyvalTrigger *self)
{
  g_return_val_if_fail (BOBGUI_IS_KEYVAL_TRIGGER (self), 0);

  return self->keyval;
}

/*** BOBGUI_MNEMONIC_TRIGGER ***/

struct _BobguiMnemonicTrigger
{
  BobguiShortcutTrigger parent_instance;

  guint keyval;
};

struct _BobguiMnemonicTriggerClass
{
  BobguiShortcutTriggerClass parent_class;
};

G_DEFINE_TYPE (BobguiMnemonicTrigger, bobgui_mnemonic_trigger, BOBGUI_TYPE_SHORTCUT_TRIGGER)

enum
{
  MNEMONIC_PROP_KEYVAL = 1,
  MNEMONIC_N_PROPS
};

static GParamSpec *mnemonic_props[MNEMONIC_N_PROPS];

static GdkKeyMatch
bobgui_mnemonic_trigger_trigger (BobguiShortcutTrigger *trigger,
                              GdkEvent           *event,
                              gboolean            enable_mnemonics)
{
  BobguiMnemonicTrigger *self = BOBGUI_MNEMONIC_TRIGGER (trigger);
  guint keyval;

  if (!enable_mnemonics)
    return GDK_KEY_MATCH_NONE;

  if (gdk_event_get_event_type (event) != GDK_KEY_PRESS)
    return GDK_KEY_MATCH_NONE;

  /* XXX: This needs to deal with groups */
  keyval = gdk_key_event_get_keyval (event);

  if (keyval == GDK_KEY_ISO_Left_Tab)
    keyval = GDK_KEY_Tab;
  else
    keyval = gdk_keyval_to_lower (keyval);

  if (keyval != self->keyval)
    return GDK_KEY_MATCH_NONE;

  return GDK_KEY_MATCH_EXACT;
}

static guint
bobgui_mnemonic_trigger_hash (BobguiShortcutTrigger *trigger)
{
  BobguiMnemonicTrigger *self = BOBGUI_MNEMONIC_TRIGGER (trigger);

  return (self->keyval << 8)
       | BOBGUI_SHORTCUT_TRIGGER_HASH_MNEMONIC;
}

static int
bobgui_mnemonic_trigger_compare (BobguiShortcutTrigger  *trigger1,
                              BobguiShortcutTrigger  *trigger2)
{
  BobguiMnemonicTrigger *self1 = BOBGUI_MNEMONIC_TRIGGER (trigger1);
  BobguiMnemonicTrigger *self2 = BOBGUI_MNEMONIC_TRIGGER (trigger2);

  return self1->keyval - self2->keyval;
}

static void
bobgui_mnemonic_trigger_print (BobguiShortcutTrigger *trigger,
                            GString            *string)
{
  BobguiMnemonicTrigger *self = BOBGUI_MNEMONIC_TRIGGER (trigger);
  const char *keyval_str;

  keyval_str = gdk_keyval_name (self->keyval);
  if (keyval_str == NULL)
    keyval_str = "???";

  g_string_append (string, "<Mnemonic>");
  g_string_append (string, keyval_str);
}

static gboolean
bobgui_mnemonic_trigger_print_label (BobguiShortcutTrigger *trigger,
                                  GdkDisplay         *display,
                                  GString            *string)
{
  BobguiMnemonicTrigger *self = BOBGUI_MNEMONIC_TRIGGER (trigger);
  const char *keyval_str;

  keyval_str = gdk_keyval_name (self->keyval);
  if (keyval_str == NULL)
    return FALSE;

  g_string_append (string, keyval_str);

  return TRUE;
}

static void
bobgui_mnemonic_trigger_set_property (GObject      *gobject,
                                   guint         prop_id,
                                   const GValue *value,
                                   GParamSpec   *pspec)
{
  BobguiMnemonicTrigger *self = BOBGUI_MNEMONIC_TRIGGER (gobject);

  switch (prop_id)
    {
    case MNEMONIC_PROP_KEYVAL:
      {
        guint keyval = g_value_get_uint (value);

        /* We store keyvals as lower key */
        if (keyval == GDK_KEY_ISO_Left_Tab)
          self->keyval = GDK_KEY_Tab;
        else
          self->keyval = gdk_keyval_to_lower (keyval);
      }
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_mnemonic_trigger_get_property (GObject    *gobject,
                                   guint       prop_id,
                                   GValue     *value,
                                   GParamSpec *pspec)
{
  BobguiMnemonicTrigger *self = BOBGUI_MNEMONIC_TRIGGER (gobject);

  switch (prop_id)
    {
    case MNEMONIC_PROP_KEYVAL:
      g_value_set_uint (value, self->keyval);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_mnemonic_trigger_class_init (BobguiMnemonicTriggerClass *klass)
{
  BobguiShortcutTriggerClass *trigger_class = BOBGUI_SHORTCUT_TRIGGER_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->set_property = bobgui_mnemonic_trigger_set_property;
  gobject_class->get_property = bobgui_mnemonic_trigger_get_property;

  trigger_class->trigger = bobgui_mnemonic_trigger_trigger;
  trigger_class->hash = bobgui_mnemonic_trigger_hash;
  trigger_class->compare = bobgui_mnemonic_trigger_compare;
  trigger_class->print = bobgui_mnemonic_trigger_print;
  trigger_class->print_label = bobgui_mnemonic_trigger_print_label;

  /**
   * BobguiMnemonicTrigger:keyval:
   *
   * The key value for the trigger.
   */
  mnemonic_props[KEYVAL_PROP_KEYVAL] =
    g_param_spec_uint (I_("keyval"), NULL, NULL,
                       0, G_MAXINT,
                       0,
                       G_PARAM_STATIC_STRINGS |
                       G_PARAM_CONSTRUCT_ONLY |
                       G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, MNEMONIC_N_PROPS, mnemonic_props);
}

static void
bobgui_mnemonic_trigger_init (BobguiMnemonicTrigger *self)
{
}

/**
 * bobgui_mnemonic_trigger_new:
 * @keyval: The keyval to trigger for
 *
 * Creates a `BobguiShortcutTrigger` that will trigger whenever the key with
 * the given @keyval is pressed and mnemonics have been activated.
 *
 * Mnemonics are activated by calling code when a key event with the right
 * modifiers is detected.
 *
 * Returns: (transfer full) (type BobguiMnemonicTrigger): A new `BobguiShortcutTrigger`
 */
BobguiShortcutTrigger *
bobgui_mnemonic_trigger_new (guint keyval)
{
  return g_object_new (BOBGUI_TYPE_MNEMONIC_TRIGGER,
                       "keyval", keyval,
                       NULL);
}

/**
 * bobgui_mnemonic_trigger_get_keyval:
 * @self: a mnemonic `BobguiShortcutTrigger`
 *
 * Gets the keyval that must be pressed to succeed triggering @self.
 *
 * Returns: the keyval
 */
guint
bobgui_mnemonic_trigger_get_keyval (BobguiMnemonicTrigger *self)
{
  g_return_val_if_fail (BOBGUI_IS_MNEMONIC_TRIGGER (self), 0);

  return self->keyval;
}

/*** BOBGUI_ALTERNATIVE_TRIGGER ***/

struct _BobguiAlternativeTrigger
{
  BobguiShortcutTrigger parent_instance;

  BobguiShortcutTrigger *first;
  BobguiShortcutTrigger *second;
};

struct _BobguiAlternativeTriggerClass
{
  BobguiShortcutTriggerClass parent_class;
};

G_DEFINE_TYPE (BobguiAlternativeTrigger, bobgui_alternative_trigger, BOBGUI_TYPE_SHORTCUT_TRIGGER)

enum
{
  ALTERNATIVE_PROP_FIRST = 1,
  ALTERNATIVE_PROP_SECOND,
  ALTERNATIVE_N_PROPS
};

static GParamSpec *alternative_props[ALTERNATIVE_N_PROPS];

static void
bobgui_alternative_trigger_dispose (GObject *gobject)
{
  BobguiAlternativeTrigger *self = BOBGUI_ALTERNATIVE_TRIGGER (gobject);

  g_clear_object (&self->first);
  g_clear_object (&self->second);

  G_OBJECT_CLASS (bobgui_alternative_trigger_parent_class)->dispose (gobject);
}

static GdkKeyMatch
bobgui_alternative_trigger_trigger (BobguiShortcutTrigger *trigger,
                                 GdkEvent           *event,
                                 gboolean            enable_mnemonics)
{
  BobguiAlternativeTrigger *self = BOBGUI_ALTERNATIVE_TRIGGER (trigger);

  return MAX (bobgui_shortcut_trigger_trigger (self->first, event, enable_mnemonics),
              bobgui_shortcut_trigger_trigger (self->second, event, enable_mnemonics));
}

static guint
bobgui_alternative_trigger_hash (BobguiShortcutTrigger *trigger)
{
  BobguiAlternativeTrigger *self = BOBGUI_ALTERNATIVE_TRIGGER (trigger);
  guint result;

  result = bobgui_shortcut_trigger_hash (self->first);
  result <<= 5;

  result |= bobgui_shortcut_trigger_hash (self->second);
  result <<= 5;

  return result | BOBGUI_SHORTCUT_TRIGGER_HASH_ALTERNATIVE;
}

static int
bobgui_alternative_trigger_compare (BobguiShortcutTrigger  *trigger1,
                                 BobguiShortcutTrigger  *trigger2)
{
  BobguiAlternativeTrigger *self1 = BOBGUI_ALTERNATIVE_TRIGGER (trigger1);
  BobguiAlternativeTrigger *self2 = BOBGUI_ALTERNATIVE_TRIGGER (trigger2);
  int cmp;

  cmp = bobgui_shortcut_trigger_compare (self1->first, self2->first);
  if (cmp != 0)
    return cmp;

  return bobgui_shortcut_trigger_compare (self1->second, self2->second);
}

static void
bobgui_alternative_trigger_print (BobguiShortcutTrigger *trigger,
                               GString            *string)
{
  BobguiAlternativeTrigger *self = BOBGUI_ALTERNATIVE_TRIGGER (trigger);

  bobgui_shortcut_trigger_print (self->first, string);
  g_string_append (string, "|");
  bobgui_shortcut_trigger_print (self->second, string);
}

static gboolean
bobgui_alternative_trigger_print_label (BobguiShortcutTrigger *trigger,
                                     GdkDisplay         *display,
                                     GString            *string)
{
  BobguiAlternativeTrigger *self = BOBGUI_ALTERNATIVE_TRIGGER (trigger);

  if (bobgui_shortcut_trigger_print_label (self->first, display, string))
    {
      g_string_append (string, ", ");
      if (!bobgui_shortcut_trigger_print_label (self->second, display, string))
        g_string_truncate (string, string->len - 2);
      return TRUE;
    }
  else
    {
      return bobgui_shortcut_trigger_print_label (self->second, display, string);
    }
}

static void
bobgui_alternative_trigger_set_property (GObject      *gobject,
                                      guint         prop_id,
                                      const GValue *value,
                                      GParamSpec   *pspec)
{
  BobguiAlternativeTrigger *self = BOBGUI_ALTERNATIVE_TRIGGER (gobject);

  switch (prop_id)
    {
    case ALTERNATIVE_PROP_FIRST:
      self->first = g_value_dup_object (value);
      break;

    case ALTERNATIVE_PROP_SECOND:
      self->second = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_alternative_trigger_get_property (GObject    *gobject,
                                      guint       prop_id,
                                      GValue     *value,
                                      GParamSpec *pspec)
{
  BobguiAlternativeTrigger *self = BOBGUI_ALTERNATIVE_TRIGGER (gobject);

  switch (prop_id)
    {
    case ALTERNATIVE_PROP_FIRST:
      g_value_set_object (value, self->first);
      break;

    case ALTERNATIVE_PROP_SECOND:
      g_value_set_object (value, self->second);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (gobject, prop_id, pspec);
    }
}

static void
bobgui_alternative_trigger_constructed (GObject *gobject)
{
  BobguiAlternativeTrigger *self = BOBGUI_ALTERNATIVE_TRIGGER (gobject);

  if (self->first == NULL || self->second == NULL)
    {
      g_critical ("Invalid alternative trigger, disabling");
      self->first = g_object_ref (bobgui_never_trigger_get ());
      self->second = g_object_ref (bobgui_never_trigger_get ());
    }

  G_OBJECT_CLASS (bobgui_alternative_trigger_parent_class)->constructed (gobject);
}

static void
bobgui_alternative_trigger_class_init (BobguiAlternativeTriggerClass *klass)
{
  BobguiShortcutTriggerClass *trigger_class = BOBGUI_SHORTCUT_TRIGGER_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->constructed = bobgui_alternative_trigger_constructed;
  gobject_class->set_property = bobgui_alternative_trigger_set_property;
  gobject_class->get_property = bobgui_alternative_trigger_get_property;
  gobject_class->dispose = bobgui_alternative_trigger_dispose;

  trigger_class->trigger = bobgui_alternative_trigger_trigger;
  trigger_class->hash = bobgui_alternative_trigger_hash;
  trigger_class->compare = bobgui_alternative_trigger_compare;
  trigger_class->print = bobgui_alternative_trigger_print;
  trigger_class->print_label = bobgui_alternative_trigger_print_label;

  /**
   * BobguiAlternativeTrigger:first:
   *
   * The first `BobguiShortcutTrigger` to check.
   */
  alternative_props[ALTERNATIVE_PROP_FIRST] =
    g_param_spec_object (I_("first"), NULL, NULL,
                         BOBGUI_TYPE_SHORTCUT_TRIGGER,
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_READWRITE);

  /**
   * BobguiAlternativeTrigger:second:
   *
   * The second `BobguiShortcutTrigger` to check.
   */
  alternative_props[ALTERNATIVE_PROP_SECOND] =
    g_param_spec_object (I_("second"), NULL, NULL,
                         BOBGUI_TYPE_SHORTCUT_TRIGGER,
                         G_PARAM_STATIC_STRINGS |
                         G_PARAM_CONSTRUCT_ONLY |
                         G_PARAM_READWRITE);

  g_object_class_install_properties (gobject_class, ALTERNATIVE_N_PROPS, alternative_props);
}

static void
bobgui_alternative_trigger_init (BobguiAlternativeTrigger *self)
{
}

/**
 * bobgui_alternative_trigger_new:
 * @first: (transfer full): The first trigger that may trigger
 * @second: (transfer full): The second trigger that may trigger
 *
 * Creates a `BobguiShortcutTrigger` that will trigger whenever
 * either of the two given triggers gets triggered.
 *
 * Note that nesting is allowed, so if you want more than two
 * alternative, create a new alternative trigger for each option.
 *
 * Returns: a new `BobguiShortcutTrigger`
 */
BobguiShortcutTrigger *
bobgui_alternative_trigger_new (BobguiShortcutTrigger *first,
                             BobguiShortcutTrigger *second)
{
  BobguiShortcutTrigger *res;

  g_return_val_if_fail (BOBGUI_IS_SHORTCUT_TRIGGER (first), NULL);
  g_return_val_if_fail (BOBGUI_IS_SHORTCUT_TRIGGER (second), NULL);

  res = g_object_new (BOBGUI_TYPE_ALTERNATIVE_TRIGGER,
                      "first", first,
                      "second", second,
                      NULL);

  g_object_unref (first);
  g_object_unref (second);

  return res;
}

/**
 * bobgui_alternative_trigger_get_first:
 * @self: an alternative `BobguiShortcutTrigger`
 *
 * Gets the first of the two alternative triggers that may
 * trigger @self.
 *
 * [method@Bobgui.AlternativeTrigger.get_second] will return
 * the other one.
 *
 * Returns: (transfer none): the first alternative trigger
 */
BobguiShortcutTrigger *
bobgui_alternative_trigger_get_first (BobguiAlternativeTrigger *self)
{
  g_return_val_if_fail (BOBGUI_IS_ALTERNATIVE_TRIGGER (self), NULL);

  return self->first;
}

/**
 * bobgui_alternative_trigger_get_second:
 * @self: an alternative `BobguiShortcutTrigger`
 *
 * Gets the second of the two alternative triggers that may
 * trigger @self.
 *
 * [method@Bobgui.AlternativeTrigger.get_first] will return
 * the other one.
 *
 * Returns: (transfer none): the second alternative trigger
 */
BobguiShortcutTrigger *
bobgui_alternative_trigger_get_second (BobguiAlternativeTrigger *self)
{
  g_return_val_if_fail (BOBGUI_IS_ALTERNATIVE_TRIGGER (self), NULL);

  return self->second;
}
