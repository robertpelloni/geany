/* bobguiaccessiblevalue.c: Accessible value
 *
 * Copyright 2020  GNOME Foundation
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
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

/*< private >
 * BobguiAccessibleValue:
 *
 * BobguiAccessibleValue is a reference counted, generic container for values used
 * to represent the state and properties of a `BobguiAccessible` implementation.
 *
 * There are two kinds of accessible value types:
 *
 *  - hard coded, static values; BOBGUI owns them, and their contents, and are
 *    guaranteed to exist for the duration of the application's life time
 *  - dynamic values; the accessible state owns the value and their contents,
 *    and they can be allocated and freed
 *
 * Typically, the former type of values is used for boolean, tristate, and
 * token value; the latter is used for numbers, strings, and token lists.
 *
 * For more information on the types of values, see the [WAI-ARIA](https://www.w3.org/WAI/PF/aria/states_and_properties#propcharacteristic_value)
 * reference.
 */

#include "config.h"

#include "bobguiaccessiblevalueprivate.h"

#include "bobguiaccessible.h"
#include "bobguibuilderprivate.h"
#include "bobguienums.h"
#include "bobguitypebuiltins.h"

#include <math.h>
#include <float.h>
#include <errno.h>

G_DEFINE_QUARK (bobgui-accessible-value-error-quark, bobgui_accessible_value_error)

G_DEFINE_BOXED_TYPE (BobguiAccessibleValue, bobgui_accessible_value,
                     bobgui_accessible_value_ref,
                     bobgui_accessible_value_unref)

/*< private >
 * bobgui_accessible_value_alloc:
 * @value_class: a `BobguiAccessibleValueClass` structure
 *
 * Allocates a new `BobguiAccessibleValue` subclass using @value_class as the
 * type definition.
 *
 * Returns: (transfer full): the newly allocated `BobguiAccessibleValue`
 */
BobguiAccessibleValue *
bobgui_accessible_value_alloc (const BobguiAccessibleValueClass *value_class)
{
  g_return_val_if_fail (value_class != NULL, NULL);
  g_return_val_if_fail (value_class->instance_size >= sizeof (BobguiAccessibleValue), NULL);

  BobguiAccessibleValue *res = g_malloc0 (value_class->instance_size);

  /* We do not use grefcount, here, because we want to have statically
   * allocated BobguiAccessibleValue subclasses, and those cannot be initialized
   * with g_ref_count_init()
   */
  res->ref_count = 1;
  res->value_class = value_class;

  if (res->value_class->init != NULL)
    res->value_class->init (res);

  return res;
}

/*< private >
 * bobgui_accessible_value_ref:
 * @self: a `BobguiAccessibleValue`
 *
 * Acquires a reference on the given `BobguiAccessibleValue`.
 *
 * Returns: (transfer full): the value, with an additional reference
 */
BobguiAccessibleValue *
bobgui_accessible_value_ref (BobguiAccessibleValue *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  self->ref_count += 1;

  return self;
}

/*< private >
 * bobgui_accessible_value_unref:
 * @self: (transfer full): a `BobguiAccessibleValue`
 *
 * Releases a reference on the given `BobguiAccessibleValue`.
 */
void
bobgui_accessible_value_unref (BobguiAccessibleValue *self)
{
  g_return_if_fail (self != NULL);

  self->ref_count -= 1;
  if (self->ref_count == 0)
    {
      if (self->value_class->finalize != NULL)
        self->value_class->finalize (self);

      g_free (self);
    }
}

/*< private >
 * bobgui_accessible_value_print:
 * @self: a `BobguiAccessibleValue`
 * @buffer: a `GString`
 *
 * Prints the contents of a `BobguiAccessibleValue` into the given @buffer.
 */
void
bobgui_accessible_value_print (const BobguiAccessibleValue *self,
                            GString                  *buffer)
{
  g_return_if_fail (self != NULL);
  g_return_if_fail (buffer != NULL);

  if (self->value_class->print != NULL)
    self->value_class->print (self, buffer);
}

/*< private >
 * bobgui_accessible_value_to_string:
 * @self: a `BobguiAccessibleValue`
 *
 * Fills a string with the contents of the given `BobguiAccessibleValue`.
 *
 * Returns: (transfer full): a string with the contents of the value
 */
char *
bobgui_accessible_value_to_string (const BobguiAccessibleValue *self)
{
  g_return_val_if_fail (self != NULL, NULL);

  GString *buffer = g_string_new (NULL);

  bobgui_accessible_value_print (self, buffer);

  return g_string_free (buffer, FALSE);
}

/*< private >
 * bobgui_accessible_value_equal:
 * @value_a: (nullable): the first `BobguiAccessibleValue`
 * @value_b: (nullable): the second `BobguiAccessibleValue`
 *
 * Checks whether @value_a and @value_b are equal.
 *
 * This function is %NULL-safe.
 *
 * Returns: %TRUE if the given `BobguiAccessibleValue` instances are equal,
 *   and %FALSE otherwise
 */
gboolean
bobgui_accessible_value_equal (const BobguiAccessibleValue *value_a,
                            const BobguiAccessibleValue *value_b)
{
  if (value_a == value_b)
    return TRUE;

  if (value_a == NULL || value_b == NULL)
    return FALSE;

  if (value_a->value_class != value_b->value_class)
    return FALSE;

  if (value_a->value_class->equal == NULL)
    return FALSE;

  return value_a->value_class->equal (value_a, value_b);
}

/* {{{ Basic allocated types */

typedef struct {
  BobguiAccessibleValue parent;

  int value;
} BobguiIntAccessibleValue;

static gboolean
bobgui_int_accessible_value_equal (const BobguiAccessibleValue *value_a,
                                const BobguiAccessibleValue *value_b)
{
  const BobguiIntAccessibleValue *self_a = (BobguiIntAccessibleValue *) value_a;
  const BobguiIntAccessibleValue *self_b = (BobguiIntAccessibleValue *) value_b;

  return self_a->value == self_b->value;
}

static void
bobgui_int_accessible_value_print (const BobguiAccessibleValue *value,
                                GString                  *buffer)
{
  const BobguiIntAccessibleValue *self = (BobguiIntAccessibleValue *) value;

  g_string_append_printf (buffer, "%d", self->value);
}

static const BobguiAccessibleValueClass BOBGUI_INT_ACCESSIBLE_VALUE = {
  .type = BOBGUI_ACCESSIBLE_VALUE_TYPE_INTEGER,
  .type_name = "BobguiIntAccessibleValue",
  .instance_size = sizeof (BobguiIntAccessibleValue),
  .equal = bobgui_int_accessible_value_equal,
  .print = bobgui_int_accessible_value_print,
};

BobguiAccessibleValue *
bobgui_int_accessible_value_new (int value)
{
  BobguiAccessibleValue *res = bobgui_accessible_value_alloc (&BOBGUI_INT_ACCESSIBLE_VALUE);

  /* XXX: Possible optimization: statically allocate the first N values
   *      and hand out references to them, instead of dynamically
   *      allocating a new BobguiAccessibleValue instance. Needs some profiling
   *      to figure out the common integer values used by large applications
   */
  BobguiIntAccessibleValue *self = (BobguiIntAccessibleValue *) res;
  self->value = value;

  return res;
}

int
bobgui_int_accessible_value_get (const BobguiAccessibleValue *value)
{
  BobguiIntAccessibleValue *self = (BobguiIntAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, 0);
  g_return_val_if_fail (value->value_class == &BOBGUI_INT_ACCESSIBLE_VALUE, 0);

  return self->value;
}

typedef struct {
  BobguiAccessibleValue parent;

  double value;
} BobguiNumberAccessibleValue;

static gboolean
bobgui_number_accessible_value_equal (const BobguiAccessibleValue *value_a,
                                   const BobguiAccessibleValue *value_b)
{
  const BobguiNumberAccessibleValue *self_a = (BobguiNumberAccessibleValue *) value_a;
  const BobguiNumberAccessibleValue *self_b = (BobguiNumberAccessibleValue *) value_b;

  return G_APPROX_VALUE (self_a->value, self_b->value, 0.001);
}

static void
bobgui_number_accessible_value_print (const BobguiAccessibleValue *value,
                                   GString                  *buffer)
{
  const BobguiNumberAccessibleValue *self = (BobguiNumberAccessibleValue *) value;

  g_string_append_printf (buffer, "%g", self->value);
}

static const BobguiAccessibleValueClass BOBGUI_NUMBER_ACCESSIBLE_VALUE = {
  .type = BOBGUI_ACCESSIBLE_VALUE_TYPE_NUMBER,
  .type_name = "BobguiNumberAccessibleValue",
  .instance_size = sizeof (BobguiNumberAccessibleValue),
  .equal = bobgui_number_accessible_value_equal,
  .print = bobgui_number_accessible_value_print,
};

BobguiAccessibleValue *
bobgui_number_accessible_value_new (double value)
{
  BobguiAccessibleValue *res = bobgui_accessible_value_alloc (&BOBGUI_NUMBER_ACCESSIBLE_VALUE);

  BobguiNumberAccessibleValue *self = (BobguiNumberAccessibleValue *) res;
  self->value = value;

  return res;
}

double
bobgui_number_accessible_value_get (const BobguiAccessibleValue *value)
{
  BobguiNumberAccessibleValue *self = (BobguiNumberAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, 0);
  g_return_val_if_fail (value->value_class == &BOBGUI_NUMBER_ACCESSIBLE_VALUE, 0);

  return self->value;
}

typedef struct {
  BobguiAccessibleValue parent;

  char *value;
  gsize length;
} BobguiStringAccessibleValue;

static void
bobgui_string_accessible_value_finalize (BobguiAccessibleValue *value)
{
  BobguiStringAccessibleValue *self = (BobguiStringAccessibleValue *) value;

  g_free (self->value);
}

static gboolean
bobgui_string_accessible_value_equal (const BobguiAccessibleValue *value_a,
                                   const BobguiAccessibleValue *value_b)
{
  const BobguiStringAccessibleValue *self_a = (BobguiStringAccessibleValue *) value_a;
  const BobguiStringAccessibleValue *self_b = (BobguiStringAccessibleValue *) value_b;

  if (self_a->length != self_b->length)
    return FALSE;

  return g_strcmp0 (self_a->value, self_b->value) == 0;
}

static void
bobgui_string_accessible_value_print (const BobguiAccessibleValue *value,
                                   GString                  *buffer)
{
  const BobguiStringAccessibleValue *self = (BobguiStringAccessibleValue *) value;

  g_string_append (buffer, self->value);
}

static const BobguiAccessibleValueClass BOBGUI_STRING_ACCESSIBLE_VALUE = {
  .type = BOBGUI_ACCESSIBLE_VALUE_TYPE_STRING,
  .type_name = "BobguiStringAccessibleValue",
  .instance_size = sizeof (BobguiStringAccessibleValue),
  .finalize = bobgui_string_accessible_value_finalize,
  .equal = bobgui_string_accessible_value_equal,
  .print = bobgui_string_accessible_value_print,
};

BobguiAccessibleValue *
bobgui_string_accessible_value_new (const char *str)
{
  BobguiAccessibleValue *res = bobgui_accessible_value_alloc (&BOBGUI_STRING_ACCESSIBLE_VALUE);

  BobguiStringAccessibleValue *self = (BobguiStringAccessibleValue *) res;

  self->value = g_strdup (str);
  self->length = strlen (str);

  return res;
}

const char *
bobgui_string_accessible_value_get (const BobguiAccessibleValue *value)
{
  BobguiStringAccessibleValue *self = (BobguiStringAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, 0);
  g_return_val_if_fail (value->value_class == &BOBGUI_STRING_ACCESSIBLE_VALUE, 0);

  return self->value;
}

typedef struct {
  BobguiAccessibleValue parent;

  BobguiAccessible *ref;
} BobguiReferenceAccessibleValue;

static void
remove_weak_ref (gpointer  data,
                 GObject  *old_reference)
{
  BobguiReferenceAccessibleValue *self = data;

  self->ref = NULL;
}

static void
bobgui_reference_accessible_value_finalize (BobguiAccessibleValue *value)
{
  BobguiReferenceAccessibleValue *self = (BobguiReferenceAccessibleValue *) value;

  if (self->ref != NULL)
    g_object_weak_unref (G_OBJECT (self->ref), remove_weak_ref, self);
}

static gboolean
bobgui_reference_accessible_value_equal (const BobguiAccessibleValue *value_a,
                                      const BobguiAccessibleValue *value_b)
{
  const BobguiReferenceAccessibleValue *self_a = (BobguiReferenceAccessibleValue *) value_a;
  const BobguiReferenceAccessibleValue *self_b = (BobguiReferenceAccessibleValue *) value_b;

  return self_a->ref == self_b->ref;
}

static void
bobgui_reference_accessible_value_print (const BobguiAccessibleValue *value,
                                      GString                  *buffer)
{
  const BobguiReferenceAccessibleValue *self = (BobguiReferenceAccessibleValue *) value;

  if (self->ref != NULL)
    {
      g_string_append_printf (buffer, "%s<%p>",
                              G_OBJECT_TYPE_NAME (self->ref),
                              self->ref);
    }
  else
    {
      g_string_append (buffer, "<null>");
    }
}

static const BobguiAccessibleValueClass BOBGUI_REFERENCE_ACCESSIBLE_VALUE = {
  .type = BOBGUI_ACCESSIBLE_VALUE_TYPE_REFERENCE,
  .type_name = "BobguiReferenceAccessibleValue",
  .instance_size = sizeof (BobguiReferenceAccessibleValue),
  .finalize = bobgui_reference_accessible_value_finalize,
  .equal = bobgui_reference_accessible_value_equal,
  .print = bobgui_reference_accessible_value_print,
};

BobguiAccessibleValue *
bobgui_reference_accessible_value_new (BobguiAccessible *ref)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (ref), NULL);

  BobguiAccessibleValue *res = bobgui_accessible_value_alloc (&BOBGUI_REFERENCE_ACCESSIBLE_VALUE);

  BobguiReferenceAccessibleValue *self = (BobguiReferenceAccessibleValue *) res;

  self->ref = ref;
  g_object_weak_ref (G_OBJECT (self->ref), remove_weak_ref, self);

  return res;
}

BobguiAccessible *
bobgui_reference_accessible_value_get (const BobguiAccessibleValue *value)
{
  BobguiReferenceAccessibleValue *self = (BobguiReferenceAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, 0);
  g_return_val_if_fail (value->value_class == &BOBGUI_REFERENCE_ACCESSIBLE_VALUE, 0);

  return self->ref;
}

typedef struct {
  BobguiAccessibleValue parent;

  GList *refs;
} BobguiReferenceListAccessibleValue;

static void
remove_weak_ref_from_list (gpointer  data,
                           GObject  *old_reference)
{
  BobguiReferenceListAccessibleValue *self = data;

  GList *item = g_list_find (self->refs, old_reference);

  if (item != NULL)
    {
      self->refs = g_list_remove_link (self->refs, item);
      g_list_free (item);
    }
}

static void
bobgui_reference_list_accessible_value_finalize (BobguiAccessibleValue *value)
{
  BobguiReferenceListAccessibleValue *self = (BobguiReferenceListAccessibleValue *) value;

  for (GList *l = self->refs; l != NULL; l = l->next)
    {
      if (l->data != NULL)
        g_object_weak_unref (G_OBJECT (l->data), remove_weak_ref_from_list, self);
    }

  g_list_free (self->refs);
}

static gboolean
bobgui_reference_list_accessible_value_equal (const BobguiAccessibleValue *value_a,
                                           const BobguiAccessibleValue *value_b)
{
  const BobguiReferenceListAccessibleValue *self_a = (BobguiReferenceListAccessibleValue *) value_a;
  const BobguiReferenceListAccessibleValue *self_b = (BobguiReferenceListAccessibleValue *) value_b;

  if (g_list_length (self_a->refs) != g_list_length (self_b->refs))
    return FALSE;

  for (GList *l = self_a->refs; l != NULL; l = l->next)
    {
      if (g_list_find (self_b->refs, l->data) == NULL)
        return FALSE;
    }

  return TRUE;
}

static void
bobgui_reference_list_accessible_value_print (const BobguiAccessibleValue *value,
                                           GString                  *buffer)
{
  const BobguiReferenceListAccessibleValue *self = (BobguiReferenceListAccessibleValue *) value;

  if (self->refs == NULL)
    {
      g_string_append (buffer, "<null>");
      return;
    }

  for (GList *l = self->refs; l != NULL; l = l->next)
    {
      g_string_append_printf (buffer, "%s<%p>",
                              G_OBJECT_TYPE_NAME (l->data),
                              l->data);
    }
}

static const BobguiAccessibleValueClass BOBGUI_REFERENCE_LIST_ACCESSIBLE_VALUE = {
  .type = BOBGUI_ACCESSIBLE_VALUE_TYPE_REFERENCE_LIST,
  .type_name = "BobguiReferenceListAccessibleValue",
  .instance_size = sizeof (BobguiReferenceListAccessibleValue),
  .finalize = bobgui_reference_list_accessible_value_finalize,
  .equal = bobgui_reference_list_accessible_value_equal,
  .print = bobgui_reference_list_accessible_value_print,
};

/*< private >
 * bobgui_reference_list_accessible_value_new:
 * @value: (element-type BobguiAccessible) (transfer full): a list of accessible objects
 *
 * Creates a new `BobguiAccessible` that stores a list of references
 * to `BobguiAccessible` objects.
 *
 * Returns: (transfer full): the newly created `BobguiAccessible`
 */
BobguiAccessibleValue *
bobgui_reference_list_accessible_value_new (GList *value)
{
  BobguiAccessibleValue *res = bobgui_accessible_value_alloc (&BOBGUI_REFERENCE_LIST_ACCESSIBLE_VALUE);

  BobguiReferenceListAccessibleValue *self = (BobguiReferenceListAccessibleValue *) res;

  self->refs = value;
  if (self->refs != NULL)
    {
      for (GList *l = self->refs; l != NULL; l = l->next)
        g_object_weak_ref (l->data, remove_weak_ref_from_list, self);
    }

  return res;
}

GList *
bobgui_reference_list_accessible_value_get (const BobguiAccessibleValue *value)
{
  BobguiReferenceListAccessibleValue *self = (BobguiReferenceListAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, 0);
  g_return_val_if_fail (value->value_class == &BOBGUI_REFERENCE_LIST_ACCESSIBLE_VALUE, 0);

  return self->refs;
}

void
bobgui_reference_list_accessible_value_append (BobguiAccessibleValue *value,
                                            BobguiAccessible      *ref)
{
  BobguiReferenceListAccessibleValue *self = (BobguiReferenceListAccessibleValue *) value;

  g_return_if_fail (value != NULL);
  g_return_if_fail (value->value_class == &BOBGUI_REFERENCE_LIST_ACCESSIBLE_VALUE);

  if (g_list_find (self->refs, ref) != NULL)
    return;

  self->refs = g_list_append (self->refs, ref);

  g_object_weak_ref (G_OBJECT (ref), remove_weak_ref_from_list, self);
}

void
bobgui_reference_list_accessible_value_remove (BobguiAccessibleValue *value,
                                            BobguiAccessible      *ref)
{
  BobguiReferenceListAccessibleValue *self = (BobguiReferenceListAccessibleValue *) value;

  g_return_if_fail (value != NULL);
  g_return_if_fail (value->value_class == &BOBGUI_REFERENCE_LIST_ACCESSIBLE_VALUE);

  if (g_list_find (self->refs, ref) == NULL)
    {
      g_warning ("Trying to remove accessible '%s', but it cannot be found in "
                 "the reference list %p",
                 G_OBJECT_TYPE_NAME (ref),
                 value);
      return;
    }

  self->refs = g_list_remove (self->refs, ref);

  g_object_weak_unref (G_OBJECT (ref), remove_weak_ref_from_list, self);
}

/* }}} */

/* {{{ Collection API */

typedef enum {
  BOBGUI_ACCESSIBLE_COLLECT_INVALID = -1,

  /* true/false */
  BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN = 0,

  /* true/false/mixed/undefined */
  BOBGUI_ACCESSIBLE_COLLECT_TRISTATE,

  /* one token */
  BOBGUI_ACCESSIBLE_COLLECT_TOKEN,

  /* integer number */
  BOBGUI_ACCESSIBLE_COLLECT_INTEGER,

  /* real number */
  BOBGUI_ACCESSIBLE_COLLECT_NUMBER,

  /* string */
  BOBGUI_ACCESSIBLE_COLLECT_STRING,

  /* reference */
  BOBGUI_ACCESSIBLE_COLLECT_REFERENCE,

  /* references list */
  BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,

  /* allows collecting BOBGUI_ACCESSIBLE_VALUE_UNDEFINED; implied
   * by BOBGUI_ACCESSIBLE_COLLECT_TRISTATE
   */
  BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED = 1 << 16
} BobguiAccessibleCollectType;

typedef struct {
  int value;
  BobguiAccessibleCollectType ctype;
  const char *name;

  /* The constructor and getter will be derived by the
   * @ctype field and by the collected value, except for the
   * BOBGUI_ACCESSIBLE_COLLECT_TOKEN collection type. You can
   * override the default ones by filling out these two
   * pointers
   */
  GCallback ctor;
  GCallback getter;
  GCallback parser;
  GCallback init_value;
} BobguiAccessibleCollect;

static const BobguiAccessibleCollect collect_states[] = {
  [BOBGUI_ACCESSIBLE_STATE_BUSY] = {
    .value = BOBGUI_ACCESSIBLE_STATE_BUSY,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN,
    .name = "busy"
  },
  [BOBGUI_ACCESSIBLE_STATE_CHECKED] = {
    .value = BOBGUI_ACCESSIBLE_STATE_CHECKED,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_TRISTATE,
    .name = "checked"
  },
  [BOBGUI_ACCESSIBLE_STATE_DISABLED] = {
    .value = BOBGUI_ACCESSIBLE_STATE_DISABLED,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN,
    .name = "disabled"
  },
  [BOBGUI_ACCESSIBLE_STATE_EXPANDED] = {
    .value = BOBGUI_ACCESSIBLE_STATE_EXPANDED,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN | BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED,
    .name = "expanded"
  },
  [BOBGUI_ACCESSIBLE_STATE_HIDDEN] = {
    .value = BOBGUI_ACCESSIBLE_STATE_HIDDEN,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN,
    .name = "hidden"
  },
  [BOBGUI_ACCESSIBLE_STATE_INVALID] = {
    .value = BOBGUI_ACCESSIBLE_STATE_INVALID,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_TOKEN,
    .name = "invalid",
    .ctor = (GCallback) bobgui_invalid_accessible_value_new,
    .getter = (GCallback) bobgui_invalid_accessible_value_get,
    .parser = (GCallback) bobgui_invalid_accessible_value_parse,
    .init_value = (GCallback) bobgui_invalid_accessible_value_init_value,
  },
  [BOBGUI_ACCESSIBLE_STATE_PRESSED] = {
    .value = BOBGUI_ACCESSIBLE_STATE_PRESSED,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_TRISTATE,
    .name = "pressed"
  },
  [BOBGUI_ACCESSIBLE_STATE_SELECTED] = {
    .value = BOBGUI_ACCESSIBLE_STATE_SELECTED,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN | BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED,
    .name = "selected"
  },
  [BOBGUI_ACCESSIBLE_STATE_VISITED] = {
    .value = BOBGUI_ACCESSIBLE_STATE_VISITED,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN|BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED,
    .name = "visited"
  }
};

/* § 6.6.1 Widget attributes */
static const BobguiAccessibleCollect collect_props[] = {
  [BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_TOKEN,
    .name = "autocomplete",
    .ctor = (GCallback) bobgui_autocomplete_accessible_value_new,
    .getter = (GCallback) bobgui_autocomplete_accessible_value_get,
    .parser = (GCallback) bobgui_autocomplete_accessible_value_parse,
    .init_value = (GCallback) bobgui_autocomplete_accessible_value_init_value,
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_STRING,
    .name = "description"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN,
    .name = "haspopup"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_STRING,
    .name = "keyshortcuts"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_LABEL] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_LABEL,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_STRING,
    .name = "label"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_LEVEL] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_LEVEL,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_INTEGER,
    .name = "level"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_MODAL] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_MODAL,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN,
    .name = "modal"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_MULTI_LINE] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_MULTI_LINE,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN,
    .name = "multiline"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN,
    .name = "multiselectable"
  },
  /* "orientation" is a bit special; it maps to BobguiOrientation, but it
   * can also be "undefined". This means we need to override constructor
   * and getter, in order to properly handle BOBGUI_ACCESSIBLE_VALUE_UNDEFINED
   */
  [BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_TOKEN | BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED,
    .name = "orientation",
    .ctor = (GCallback) bobgui_orientation_accessible_value_new,
    .getter = (GCallback) bobgui_orientation_accessible_value_get,
    .parser = (GCallback) bobgui_orientation_accessible_value_parse,
    .init_value = (GCallback) bobgui_orientation_accessible_value_init_value,
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_STRING,
    .name = "placeholder"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN,
    .name = "readonly"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_REQUIRED] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_REQUIRED,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN,
    .name = "required"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_ROLE_DESCRIPTION] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_ROLE_DESCRIPTION,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_STRING,
    .name = "roledescription"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_SORT] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_SORT,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_TOKEN,
    .name = "sort",
    .ctor = (GCallback) bobgui_sort_accessible_value_new,
    .getter = (GCallback) bobgui_sort_accessible_value_get,
    .parser = (GCallback) bobgui_sort_accessible_value_parse,
    .init_value = (GCallback) bobgui_sort_accessible_value_init_value,
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_NUMBER,
    .name = "valuemax"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_NUMBER,
    .name = "valuemin"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_NUMBER,
    .name = "valuenow"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_STRING,
    .name = "valuetext"
  },
  [BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT] = {
    .value = BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_STRING,
    .name = "helptext"
  },
};

/* § 6.6.4 Relationship Attributes */
static const BobguiAccessibleCollect collect_rels[] = {
  [BOBGUI_ACCESSIBLE_RELATION_ACTIVE_DESCENDANT] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_ACTIVE_DESCENDANT,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE,
    .name = "activedescendant"
  },
  [BOBGUI_ACCESSIBLE_RELATION_COL_COUNT] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_COL_COUNT,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_INTEGER,
    .name = "colcount"
  },
  [BOBGUI_ACCESSIBLE_RELATION_COL_INDEX] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_COL_INDEX,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_INTEGER,
    .name = "colindex"
  },
  [BOBGUI_ACCESSIBLE_RELATION_COL_INDEX_TEXT] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_COL_INDEX_TEXT,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_STRING,
    .name = "colindextext"
  },
  [BOBGUI_ACCESSIBLE_RELATION_COL_SPAN] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_COL_SPAN,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_INTEGER,
    .name = "colspan"
  },
  [BOBGUI_ACCESSIBLE_RELATION_CONTROLS] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_CONTROLS,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "controls"
  },
  [BOBGUI_ACCESSIBLE_RELATION_DESCRIBED_BY] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_DESCRIBED_BY,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "describedby"
  },
  [BOBGUI_ACCESSIBLE_RELATION_DETAILS] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_DETAILS,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "details"
  },
  [BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "errormessage"
  },
  [BOBGUI_ACCESSIBLE_RELATION_FLOW_TO] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_FLOW_TO,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "flowto"
  },
  [BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "labelledby"
  },
  [BOBGUI_ACCESSIBLE_RELATION_OWNS] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_OWNS,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "owns"
  },
  [BOBGUI_ACCESSIBLE_RELATION_POS_IN_SET] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_POS_IN_SET,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_INTEGER,
    .name = "posinset"
  },
  [BOBGUI_ACCESSIBLE_RELATION_ROW_COUNT] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_ROW_COUNT,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_INTEGER,
    .name = "rowcount"
  },
  [BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_INTEGER,
    .name = "rowindex"
  },
  [BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX_TEXT] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX_TEXT,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_STRING,
    .name = "rowindextext"
  },
  [BOBGUI_ACCESSIBLE_RELATION_ROW_SPAN] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_ROW_SPAN,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_INTEGER,
    .name = "rowspan"
  },
  [BOBGUI_ACCESSIBLE_RELATION_SET_SIZE] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_SET_SIZE,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_INTEGER,
    .name = "setsize"
  },
  [BOBGUI_ACCESSIBLE_RELATION_LABEL_FOR] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_LABEL_FOR,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "labelfor"
  },
  [BOBGUI_ACCESSIBLE_RELATION_DESCRIPTION_FOR] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_DESCRIPTION_FOR,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "descriptionfor"
  },
  [BOBGUI_ACCESSIBLE_RELATION_CONTROLLED_BY] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_CONTROLLED_BY,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "controlledby"
  },
  [BOBGUI_ACCESSIBLE_RELATION_DETAILS_FOR] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_DETAILS_FOR,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "detailsfor"
  },
  [BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE_FOR] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE_FOR,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "errormessagefor"
  },
  [BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM] = {
    .value = BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM,
    .ctype = BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST,
    .name = "flowfrom"
  },
};

typedef BobguiAccessibleValue * (* BobguiAccessibleValueBooleanCtor)  (gboolean value);
typedef BobguiAccessibleValue * (* BobguiAccessibleValueIntCtor)      (int value);
typedef BobguiAccessibleValue * (* BobguiAccessibleValueTristateCtor) (int value);
typedef BobguiAccessibleValue * (* BobguiAccessibleValueEnumCtor)     (int value);
typedef BobguiAccessibleValue * (* BobguiAccessibleValueNumberCtor)   (double value);
typedef BobguiAccessibleValue * (* BobguiAccessibleValueStringCtor)   (const char *value);
typedef BobguiAccessibleValue * (* BobguiAccessibleValueRefCtor)      (BobguiAccessible *value);
typedef BobguiAccessibleValue * (* BobguiAccessibleValueRefListCtor)  (GList *value);

typedef BobguiAccessibleValue * (* BobguiAccessibleValueEnumParser)   (const char  *str,
                                                                 gsize        len,
                                                                 GError     **error);
typedef void                 (* BobguiAccessibleValueEnumInit)     (GValue      *value);

/*< private >
 * bobgui_accessible_value_get_default_for_state:
 * @state: a `BobguiAccessibleState`
 *
 * Retrieves the `BobguiAccessibleValue` that contains the default for the
 * given @state.
 *
 * Returns: (transfer full): the `BobguiAccessibleValue`
 */
BobguiAccessibleValue *
bobgui_accessible_value_get_default_for_state (BobguiAccessibleState state)
{
  const BobguiAccessibleCollect *cstate = &collect_states[state];

  g_return_val_if_fail (state <= BOBGUI_ACCESSIBLE_STATE_VISITED, NULL);

  switch (cstate->value)
    {
    case BOBGUI_ACCESSIBLE_STATE_BUSY:
    case BOBGUI_ACCESSIBLE_STATE_DISABLED:
    case BOBGUI_ACCESSIBLE_STATE_HIDDEN:
      return bobgui_boolean_accessible_value_new (FALSE);

    case BOBGUI_ACCESSIBLE_STATE_CHECKED:
    case BOBGUI_ACCESSIBLE_STATE_EXPANDED:
    case BOBGUI_ACCESSIBLE_STATE_PRESSED:
    case BOBGUI_ACCESSIBLE_STATE_SELECTED:
    case BOBGUI_ACCESSIBLE_STATE_VISITED:
      return bobgui_undefined_accessible_value_new ();

    case BOBGUI_ACCESSIBLE_STATE_INVALID:
      return bobgui_invalid_accessible_value_new (BOBGUI_ACCESSIBLE_INVALID_FALSE);

    default:
      g_critical ("Unknown value for accessible state “%s”", cstate->name);
      break;
    }

  return NULL;
}

static BobguiAccessibleValue *
bobgui_accessible_value_collect_valist (const BobguiAccessibleCollect  *cstate,
                                     GError                     **error,
                                     va_list                     *args)
{
  BobguiAccessibleValue *res = NULL;
  BobguiAccessibleCollectType ctype = cstate->ctype;
  gboolean collects_undef = (ctype & BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED) != 0;

  ctype &= (BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED - 1);

  /* Tristate values include "undefined" by definition */
  if (ctype == BOBGUI_ACCESSIBLE_COLLECT_TRISTATE)
    collects_undef = TRUE;

  switch (ctype)
    {
    case BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN:
      {
        if (collects_undef)
          {
            int value = va_arg (*args, int);

            if (value == BOBGUI_ACCESSIBLE_VALUE_UNDEFINED)
              res = bobgui_undefined_accessible_value_new ();
            else
              res = bobgui_boolean_accessible_value_new (value == 0 ? FALSE : TRUE);
          }
        else
          {
            gboolean value = va_arg (*args, gboolean);

            res = bobgui_boolean_accessible_value_new (value);
          }
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_TRISTATE:
      {
        int value = va_arg (*args, int);

        if (collects_undef && value == BOBGUI_ACCESSIBLE_VALUE_UNDEFINED)
          res = bobgui_undefined_accessible_value_new ();
        else
          res = bobgui_tristate_accessible_value_new (value);
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_TOKEN:
      {
        BobguiAccessibleValueEnumCtor ctor =
          (BobguiAccessibleValueEnumCtor) cstate->ctor;

        int value = va_arg (*args, int);

        if (collects_undef && value == BOBGUI_ACCESSIBLE_VALUE_UNDEFINED)
          {
            res = bobgui_undefined_accessible_value_new ();
          }
        else
          {
            /* Token collection requires a constructor */
            g_assert (ctor != NULL);

            res = (* ctor) (value);
          }

        if (res == NULL)
          {
            g_set_error (error, BOBGUI_ACCESSIBLE_VALUE_ERROR,
                         BOBGUI_ACCESSIBLE_VALUE_ERROR_INVALID_TOKEN,
                         "Invalid value for token attribute: %d",
                         value);
          }
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_INTEGER:
      {
        BobguiAccessibleValueEnumCtor ctor =
          (BobguiAccessibleValueEnumCtor) cstate->ctor;

        int value = va_arg (*args, int);

        if (ctor == NULL)
          res = bobgui_int_accessible_value_new (value);
        else
          res = (* ctor) (value);
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_NUMBER:
      {
        BobguiAccessibleValueNumberCtor ctor =
          (BobguiAccessibleValueNumberCtor) cstate->ctor;

        double value = va_arg (*args, double);

        if (isnan (value) || isinf (value))
          {
            g_set_error_literal (error, BOBGUI_ACCESSIBLE_VALUE_ERROR,
                                 BOBGUI_ACCESSIBLE_VALUE_ERROR_INVALID_VALUE,
                                 "Invalid numeric value");
            return NULL;
          }

        if (ctor == NULL)
          res = bobgui_number_accessible_value_new (value);
        else
          res = (* ctor) (value);
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_STRING:
      {
        BobguiAccessibleValueStringCtor ctor =
          (BobguiAccessibleValueStringCtor) cstate->ctor;

        const char *value = va_arg (*args, char*);

        if (ctor == NULL)
          {
            if (value != NULL)
              res = bobgui_string_accessible_value_new (value);
          }
        else
          {
            res = (* ctor) (value);
          }
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_REFERENCE:
      {
        BobguiAccessibleValueRefCtor ctor =
          (BobguiAccessibleValueRefCtor) cstate->ctor;

        gpointer value = va_arg (*args, gpointer);

        if (value != NULL && !BOBGUI_IS_ACCESSIBLE (value))
          {
            g_set_error_literal (error, BOBGUI_ACCESSIBLE_VALUE_ERROR,
                                 BOBGUI_ACCESSIBLE_VALUE_ERROR_INVALID_VALUE,
                                 "Reference does not implement BobguiAccessible");
            return NULL;
          }

        if (ctor == NULL)
          {
            if (value != NULL)
              res = bobgui_reference_accessible_value_new (value);
            else
              res = bobgui_undefined_accessible_value_new ();
          }
        else
          {
            res = (* ctor) (value);
          }
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST:
      {
        BobguiAccessibleValueRefListCtor ctor =
          (BobguiAccessibleValueRefListCtor) cstate->ctor;

        BobguiAccessible *ref = va_arg (*args, gpointer);
        GList *value = NULL;

        while (ref != NULL)
          {
            if (!BOBGUI_IS_ACCESSIBLE (ref))
              {
                g_set_error (error, BOBGUI_ACCESSIBLE_VALUE_ERROR,
                             BOBGUI_ACCESSIBLE_VALUE_ERROR_INVALID_VALUE,
                             "Reference of type “%s” [%p] does not implement BobguiAccessible",
                             G_OBJECT_TYPE_NAME (ref), ref);
                return NULL;
              }

            value = g_list_prepend (value, ref);

            ref = va_arg (*args, gpointer);
          }

        if (value == NULL)
          res = bobgui_undefined_accessible_value_new ();
        else
          {
            value = g_list_reverse (value);

            if (ctor == NULL)
              res = bobgui_reference_list_accessible_value_new (value);
            else
              res = (* ctor) (value);
          }
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED:
    case BOBGUI_ACCESSIBLE_COLLECT_INVALID:
    default:
      g_assert_not_reached ();
      break;
    }

  return res;
}

static BobguiAccessibleValue *
bobgui_accessible_value_collect_value (const BobguiAccessibleCollect  *cstate,
                                    const GValue                *value_,
                                    GError                     **error)
{
  BobguiAccessibleValue *res = NULL;
  BobguiAccessibleCollectType ctype = cstate->ctype;
  gboolean collects_undef = (ctype & BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED) != 0;

  ctype &= (BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED - 1);

  /* Tristate values include "undefined" by definition */
  if (ctype == BOBGUI_ACCESSIBLE_COLLECT_TRISTATE)
    collects_undef = TRUE;

  switch (ctype)
    {
    case BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN:
      {
        if (collects_undef)
          {
            int value = g_value_get_int (value_);

            if (value == BOBGUI_ACCESSIBLE_VALUE_UNDEFINED)
              res = bobgui_undefined_accessible_value_new ();
            else
              res = bobgui_boolean_accessible_value_new (value == 0 ? FALSE : TRUE);
          }
        else
          {
            gboolean value = g_value_get_boolean (value_);

            res = bobgui_boolean_accessible_value_new (value);
          }
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_TRISTATE:
      {
        int value = g_value_get_int (value_);

        if (collects_undef && value == BOBGUI_ACCESSIBLE_VALUE_UNDEFINED)
          res = bobgui_undefined_accessible_value_new ();
        else
          res = bobgui_tristate_accessible_value_new (value);
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_TOKEN:
      {
        BobguiAccessibleValueEnumCtor ctor =
          (BobguiAccessibleValueEnumCtor) cstate->ctor;

        int value = g_value_get_int (value_);

        if (collects_undef && value == BOBGUI_ACCESSIBLE_VALUE_UNDEFINED)
          {
            res = bobgui_undefined_accessible_value_new ();
          }
        else
          {
            /* Token collection requires a constructor */
            g_assert (ctor != NULL);

            res = (* ctor) (value);
          }

        if (res == NULL)
          {
            g_set_error (error, BOBGUI_ACCESSIBLE_VALUE_ERROR,
                         BOBGUI_ACCESSIBLE_VALUE_ERROR_INVALID_TOKEN,
                         "Invalid value for token attribute: %d",
                         value);
          }
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_INTEGER:
      {
        BobguiAccessibleValueEnumCtor ctor =
          (BobguiAccessibleValueEnumCtor) cstate->ctor;

        int value = g_value_get_int (value_);

        if (ctor == NULL)
          res = bobgui_int_accessible_value_new (value);
        else
          res = (* ctor) (value);
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_NUMBER:
      {
        BobguiAccessibleValueNumberCtor ctor =
          (BobguiAccessibleValueNumberCtor) cstate->ctor;

        double value = g_value_get_double (value_);

        if (isnan (value) || isinf (value))
          {
            g_set_error_literal (error, BOBGUI_ACCESSIBLE_VALUE_ERROR,
                                 BOBGUI_ACCESSIBLE_VALUE_ERROR_INVALID_VALUE,
                                 "Invalid numeric value");
            return NULL;
          }

        if (ctor == NULL)
          res = bobgui_number_accessible_value_new (value);
        else
          res = (* ctor) (value);
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_STRING:
      {
        BobguiAccessibleValueStringCtor ctor =
          (BobguiAccessibleValueStringCtor) cstate->ctor;

        const char *value = g_value_get_string (value_);

        if (ctor == NULL)
          {
            if (value != NULL)
              res = bobgui_string_accessible_value_new (value);
          }
        else
          {
            res = (* ctor) (value);
          }
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_REFERENCE:
      {
        BobguiAccessibleValueRefCtor ctor =
          (BobguiAccessibleValueRefCtor) cstate->ctor;

        gpointer value = g_value_get_object (value_);

        if (value != NULL && !BOBGUI_IS_ACCESSIBLE (value))
          {
            g_set_error_literal (error, BOBGUI_ACCESSIBLE_VALUE_ERROR,
                                 BOBGUI_ACCESSIBLE_VALUE_ERROR_INVALID_VALUE,
                                 "Reference does not implement BobguiAccessible");
            return NULL;
          }

        if (ctor == NULL)
          {
            if (value != NULL)
              res = bobgui_reference_accessible_value_new (value);
          }
        else
          {
            res = (* ctor) (value);
          }
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST:
      {
        BobguiAccessibleValueRefListCtor ctor =
          (BobguiAccessibleValueRefListCtor) cstate->ctor;

        GList *value;

        if (g_type_is_a (G_VALUE_TYPE(value_), BOBGUI_ACCESSIBLE_LIST))
          {
            BobguiAccessibleList *boxed = g_value_get_boxed (value_);

            value = bobgui_accessible_list_get_objects (boxed);
          }
        else
          {
            value = g_list_copy (g_value_get_pointer (value_));
          }

        if (ctor == NULL)
          {
            if (value != NULL)
              res = bobgui_reference_list_accessible_value_new (value);
          }
        else
          {
            res = (* ctor) (value);
          }
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED:
    case BOBGUI_ACCESSIBLE_COLLECT_INVALID:
    default:
      g_assert_not_reached ();
      break;
    }

  return res;
}

static BobguiAccessibleValue *
bobgui_accessible_value_parse (const BobguiAccessibleCollect  *cstate,
                            const char                  *str,
                            gsize                        len,
                            GError                     **error)
{
  BobguiAccessibleValue *res = NULL;
  BobguiAccessibleCollectType ctype = cstate->ctype;
  gboolean collects_undef = (ctype & BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED) != 0;

  ctype &= (BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED - 1);

  /* Tristate values include "undefined" by definition */
  if (ctype == BOBGUI_ACCESSIBLE_COLLECT_TRISTATE)
    collects_undef = TRUE;

  switch (ctype)
    {
    case BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN:
      {
        gboolean b;

        if (collects_undef && strncmp (str, "undefined", 9) == 0)
          res = bobgui_undefined_accessible_value_new ();
        else if (_bobgui_builder_boolean_from_string (str, &b, error))
          res = bobgui_boolean_accessible_value_new (b);
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_TRISTATE:
      {
        int value;

        if (collects_undef && strncmp (str, "undefined", 9) == 0)
          res = bobgui_undefined_accessible_value_new ();
        else if (_bobgui_builder_enum_from_string (BOBGUI_TYPE_ACCESSIBLE_TRISTATE, str, &value, error))
          res = bobgui_boolean_accessible_value_new (value);
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_TOKEN:
      {
        BobguiAccessibleValueEnumParser parser =
          (BobguiAccessibleValueEnumParser) cstate->parser;

        if (collects_undef && strncmp (str, "undefined", 9) == 0)
          {
            res = bobgui_undefined_accessible_value_new ();
          }
        else
          {
            /* Token collection requires a constructor */
            g_assert (parser != NULL);

            res = (* parser) (str, len, error);
          }
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_INTEGER:
      {
        char *end = NULL;
        gint64 value = g_ascii_strtoll (str, &end, 10);

        if (str == end)
          {
            int saved_errno = errno;
            g_set_error (error, BOBGUI_ACCESSIBLE_VALUE_ERROR,
                         BOBGUI_ACCESSIBLE_VALUE_ERROR_INVALID_VALUE,
                         "Invalid integer value “%s”: %s",
                         str, g_strerror (saved_errno));

            return NULL;
          }
        else
          res = bobgui_int_accessible_value_new ((int) value);
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_NUMBER:
      {
        char *end = NULL;
        double value = g_ascii_strtod (str, &end);

        if (str == end || isnan (value) || isinf (value))
          {
            g_set_error (error, BOBGUI_ACCESSIBLE_VALUE_ERROR,
                         BOBGUI_ACCESSIBLE_VALUE_ERROR_INVALID_VALUE,
                         "Invalid numeric value “%s”",
                         str);
            return NULL;
          }

        res = bobgui_number_accessible_value_new (value);
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_STRING:
      {
        res = bobgui_string_accessible_value_new (str);
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_REFERENCE:
    case BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST:
      {
        /* We do not error out, to let the caller code deal
         * with the references themselves
         */
        res = NULL;
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED:
    case BOBGUI_ACCESSIBLE_COLLECT_INVALID:
    default:
      g_assert_not_reached ();
      break;
    }

  return res;
}

static void
bobgui_accessible_attribute_init_value (const BobguiAccessibleCollect *cstate,
                                     GValue                     *value)
{
  BobguiAccessibleCollectType ctype = cstate->ctype;
  gboolean collects_undef = (ctype & BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED) != 0;

  ctype &= (BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED - 1);

  /* Tristate values include "undefined" by definition */
  if (ctype == BOBGUI_ACCESSIBLE_COLLECT_TRISTATE)
    collects_undef = TRUE;

  switch (ctype)
    {
    case BOBGUI_ACCESSIBLE_COLLECT_BOOLEAN:
      {
        if (collects_undef)
          g_value_init (value, G_TYPE_INT);
        else
          g_value_init (value, G_TYPE_BOOLEAN);
      }
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_TRISTATE:
      g_value_init (value, BOBGUI_TYPE_ACCESSIBLE_TRISTATE);
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_TOKEN:
      if (cstate->init_value != NULL)
        {
          BobguiAccessibleValueEnumInit init_value =
            (BobguiAccessibleValueEnumInit) cstate->init_value;

          (* init_value) (value);
        }
      else
        g_value_init (value, G_TYPE_INT);
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_INTEGER:
      g_value_init (value, G_TYPE_INT);
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_NUMBER:
      g_value_init (value, G_TYPE_DOUBLE);
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_STRING:
      g_value_init (value, G_TYPE_STRING);
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_REFERENCE:
      g_value_init (value, BOBGUI_TYPE_ACCESSIBLE);
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_REFERENCE_LIST:
      g_value_init (value, G_TYPE_POINTER);
      break;

    case BOBGUI_ACCESSIBLE_COLLECT_UNDEFINED:
    case BOBGUI_ACCESSIBLE_COLLECT_INVALID:
    default:
      g_assert_not_reached ();
      break;
    }
}

/*< private >
 * bobgui_accessible_value_collect_for_state:
 * @state: a `BobguiAccessibleState`
 * @error: return location for a `GError`
 * @args: a `va_list` reference
 *
 * Collects and consumes the next item in the @args variadic arguments list,
 * and returns a `BobguiAccessibleValue` for it.
 *
 * If the collection fails, @error is set and %NULL is returned.
 *
 * The returned value could be %NULL even on success, in which case the state
 * should be reset to its default value by the caller.
 *
 * Returns: (transfer full) (nullable): a `BobguiAccessibleValue`
 */
BobguiAccessibleValue *
bobgui_accessible_value_collect_for_state (BobguiAccessibleState   state,
                                        GError             **error,
                                        va_list             *args)
{
  const BobguiAccessibleCollect *cstate = &collect_states[state];

  g_return_val_if_fail (state <= BOBGUI_ACCESSIBLE_STATE_VISITED, NULL);

  return bobgui_accessible_value_collect_valist (cstate, error, args);
}

/*< private >
 * bobgui_accessible_value_collect_for_state_value:
 * @state: a `BobguiAccessibleState`
 * @value: a `GValue`
 * @error: return location for a `GError`
 *
 * Retrieves the value stored inside @value and returns a `BobguiAccessibleValue`
 * for the given @state.
 *
 * If the collection fails, @error is set and %NULL is returned.
 *
 * The returned value could be %NULL even on success, in which case the state
 * should be reset to its default value by the caller.
 *
 * Returns: (transfer full) (nullable): a `BobguiAccessibleValue`
 */
BobguiAccessibleValue *
bobgui_accessible_value_collect_for_state_value (BobguiAccessibleState   state,
                                              const GValue        *value,
                                              GError             **error)
{
  const BobguiAccessibleCollect *cstate = &collect_states[state];

  g_return_val_if_fail (state <= BOBGUI_ACCESSIBLE_STATE_VISITED, NULL);

  return bobgui_accessible_value_collect_value (cstate, value, error);
}

BobguiAccessibleValue *
bobgui_accessible_value_parse_for_state (BobguiAccessibleState   state,
                                      const char          *str,
                                      gsize                len,
                                      GError             **error)
{
  const BobguiAccessibleCollect *cstate = &collect_states[state];

  g_return_val_if_fail (state <= BOBGUI_ACCESSIBLE_STATE_VISITED, NULL);

  return bobgui_accessible_value_parse (cstate, str, len, error);
}

/**
 * bobgui_accessible_state_init_value:
 * @state: a `BobguiAccessibleState`
 * @value: an uninitialized `GValue`
 *
 * Initializes @value with the appropriate type for the @state.
 *
 * This function is mostly meant for language bindings, in conjunction
 * with bobgui_accessible_update_relation_state().
 */
void
bobgui_accessible_state_init_value (BobguiAccessibleState  state,
                                 GValue             *value)
{
  const BobguiAccessibleCollect *cstate = &collect_states[state];

  g_return_if_fail (state <= BOBGUI_ACCESSIBLE_STATE_SELECTED);

  bobgui_accessible_attribute_init_value (cstate, value);
}

/*< private >
 * bobgui_accessible_value_get_default_for_property:
 * @property: a `BobguiAccessibleProperty`
 *
 * Retrieves the `BobguiAccessibleValue` that contains the default for the
 * given @property.
 *
 * Returns: (transfer full): the `BobguiAccessibleValue`
 */
BobguiAccessibleValue *
bobgui_accessible_value_get_default_for_property (BobguiAccessibleProperty property)
{
  const BobguiAccessibleCollect *cstate = &collect_props[property];

  g_return_val_if_fail (property <= BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT, NULL);

  switch (cstate->value)
    {
    /* Boolean properties */
    case BOBGUI_ACCESSIBLE_PROPERTY_HAS_POPUP:
    case BOBGUI_ACCESSIBLE_PROPERTY_MODAL:
    case BOBGUI_ACCESSIBLE_PROPERTY_MULTI_LINE:
    case BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE:
    case BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY:
    case BOBGUI_ACCESSIBLE_PROPERTY_REQUIRED:
      return bobgui_boolean_accessible_value_new (FALSE);

    /* Integer properties */
    case BOBGUI_ACCESSIBLE_PROPERTY_LEVEL:
      return bobgui_int_accessible_value_new (0);

    /* Number properties */
    case BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX:
    case BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN:
    case BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW:
      return bobgui_number_accessible_value_new (0);

    /* String properties */
    case BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION:
    case BOBGUI_ACCESSIBLE_PROPERTY_KEY_SHORTCUTS:
    case BOBGUI_ACCESSIBLE_PROPERTY_LABEL:
    case BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER:
    case BOBGUI_ACCESSIBLE_PROPERTY_ROLE_DESCRIPTION:
    case BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT:
    case BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT:
      return bobgui_undefined_accessible_value_new ();

    /* Token properties */
    case BOBGUI_ACCESSIBLE_PROPERTY_AUTOCOMPLETE:
      return bobgui_autocomplete_accessible_value_new (BOBGUI_ACCESSIBLE_AUTOCOMPLETE_NONE);

    case BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION:
      return bobgui_undefined_accessible_value_new ();

    case BOBGUI_ACCESSIBLE_PROPERTY_SORT:
      return bobgui_sort_accessible_value_new (BOBGUI_ACCESSIBLE_SORT_NONE);

    default:
      g_critical ("Unknown value for accessible property “%s”", cstate->name);
      break;
    }

  return NULL;
}

/*< private >
 * bobgui_accessible_value_collect_for_property:
 * @property: a `BobguiAccessibleProperty`
 * @error: return location for a `GError`
 * @args: a `va_list` reference
 *
 * Collects and consumes the next item in the @args variadic arguments list,
 * and returns a `BobguiAccessibleValue` for it.
 *
 * If the collection fails, @error is set.
 *
 * Returns: (transfer full) (nullable): a `BobguiAccessibleValue`
 */
BobguiAccessibleValue *
bobgui_accessible_value_collect_for_property (BobguiAccessibleProperty   property,
                                           GError                **error,
                                           va_list                *args)
{
  const BobguiAccessibleCollect *cstate = &collect_props[property];

  g_return_val_if_fail (property <= BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT, NULL);

  return bobgui_accessible_value_collect_valist (cstate, error, args);
}

/*< private >
 * bobgui_accessible_value_collect_for_property_value:
 * @property: a `BobguiAccessibleProperty`
 * @value: a `GValue`
 * @error: return location for a `GError`
 *
 * Retrieves the value stored inside @value and returns a `BobguiAccessibleValue`
 * for the given @property.
 *
 * If the collection fails, @error is set.
 *
 * The returned value could be %NULL even on success, in which case the property
 * should be reset to its default value by the caller.
 *
 * Returns: (transfer full) (nullable): a `BobguiAccessibleValue`
 */
BobguiAccessibleValue *
bobgui_accessible_value_collect_for_property_value (BobguiAccessibleProperty   property,
                                                 const GValue           *value,
                                                 GError                **error)
{
  const BobguiAccessibleCollect *cstate = &collect_props[property];

  g_return_val_if_fail (property <= BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT, NULL);

  return bobgui_accessible_value_collect_value (cstate, value, error);
}

BobguiAccessibleValue *
bobgui_accessible_value_parse_for_property (BobguiAccessibleProperty   property,
                                         const char             *str,
                                         gsize                   len,
                                         GError                **error)
{
  const BobguiAccessibleCollect *cstate = &collect_props[property];

  g_return_val_if_fail (property <= BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT, NULL);

  return bobgui_accessible_value_parse (cstate, str, len, error);
}

/**
 * bobgui_accessible_property_init_value:
 * @property: a `BobguiAccessibleProperty`
 * @value: an uninitialized `GValue`
 *
 * Initializes @value with the appropriate type for the @property.
 *
 * This function is mostly meant for language bindings, in conjunction
 * with bobgui_accessible_update_property_value().
 */
void
bobgui_accessible_property_init_value (BobguiAccessibleProperty  property,
                                    GValue                *value)
{
  const BobguiAccessibleCollect *cstate = &collect_props[property];

  g_return_if_fail (property <= BOBGUI_ACCESSIBLE_PROPERTY_HELP_TEXT);

  bobgui_accessible_attribute_init_value (cstate, value);
}

/*< private >
 * bobgui_accessible_value_get_default_for_relation:
 * @relation: a `BobguiAccessibleRelation`
 *
 * Retrieves the `BobguiAccessibleValue` that contains the default for the
 * given @relation.
 *
 * Returns: (transfer full): the `BobguiAccessibleValue`
 */
BobguiAccessibleValue *
bobgui_accessible_value_get_default_for_relation (BobguiAccessibleRelation relation)
{
  const BobguiAccessibleCollect *cstate = &collect_rels[relation];

  g_return_val_if_fail (relation <= BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM, NULL);

  switch (cstate->value)
    {
    /* References */
    case BOBGUI_ACCESSIBLE_RELATION_ACTIVE_DESCENDANT:
    case BOBGUI_ACCESSIBLE_RELATION_CONTROLS:
    case BOBGUI_ACCESSIBLE_RELATION_CONTROLLED_BY:
    case BOBGUI_ACCESSIBLE_RELATION_DESCRIBED_BY:
    case BOBGUI_ACCESSIBLE_RELATION_DESCRIPTION_FOR:
    case BOBGUI_ACCESSIBLE_RELATION_DETAILS:
    case BOBGUI_ACCESSIBLE_RELATION_DETAILS_FOR:
    case BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE:
    case BOBGUI_ACCESSIBLE_RELATION_ERROR_MESSAGE_FOR:
    case BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM:
    case BOBGUI_ACCESSIBLE_RELATION_FLOW_TO:
    case BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY:
    case BOBGUI_ACCESSIBLE_RELATION_LABEL_FOR:
    case BOBGUI_ACCESSIBLE_RELATION_OWNS:
      return bobgui_undefined_accessible_value_new ();

    /* Integers */
    case BOBGUI_ACCESSIBLE_RELATION_COL_COUNT:
    case BOBGUI_ACCESSIBLE_RELATION_COL_INDEX:
    case BOBGUI_ACCESSIBLE_RELATION_COL_SPAN:
    case BOBGUI_ACCESSIBLE_RELATION_POS_IN_SET:
    case BOBGUI_ACCESSIBLE_RELATION_ROW_COUNT:
    case BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX:
    case BOBGUI_ACCESSIBLE_RELATION_ROW_SPAN:
    case BOBGUI_ACCESSIBLE_RELATION_SET_SIZE:
      return bobgui_int_accessible_value_new (0);

    /* Strings */
    case BOBGUI_ACCESSIBLE_RELATION_ROW_INDEX_TEXT:
    case BOBGUI_ACCESSIBLE_RELATION_COL_INDEX_TEXT:
      return bobgui_undefined_accessible_value_new ();

    default:
      g_critical ("Unknown value for accessible property “%s”", cstate->name);
      break;
    }

  return NULL;
}

/*< private >
 * bobgui_accessible_value_collect_for_relation:
 * @relation: a `BobguiAccessibleRelation`
 * @error: return location for a `GError`
 * @args: a `va_list` reference
 *
 * Collects and consumes the next item in the @args variadic arguments list,
 * and returns a `BobguiAccessibleValue` for it.
 *
 * If the collection fails, @error is set and %NULL is returned.
 *
 * The returned value could be %NULL even on success, in which case the relation
 * should be reset to its default value by the caller.
 *
 * Returns: (transfer full) (nullable): a `BobguiAccessibleValue`
 */
BobguiAccessibleValue *
bobgui_accessible_value_collect_for_relation (BobguiAccessibleRelation   relation,
                                           GError                **error,
                                           va_list                *args)
{
  const BobguiAccessibleCollect *cstate = &collect_rels[relation];

  g_return_val_if_fail (relation <= BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM, NULL);

  return bobgui_accessible_value_collect_valist (cstate, error, args);
}

/*< private >
 * bobgui_accessible_value_collect_for_relation_value:
 * @relation: a `BobguiAccessibleRelation`
 * @value: a `GValue`
 * @error: return location for a `GError`
 *
 * Retrieves the value stored inside @value and returns a `BobguiAccessibleValue`
 * for the given @relation.
 *
 * If the collection fails, @error is set and %NULL is returned.
 *
 * The returned value could be %NULL even on success, in which case the relation
 * should be reset to its default value by the caller.
 *
 * Returns: (transfer full) (nullable): a `BobguiAccessibleValue`
 */
BobguiAccessibleValue *
bobgui_accessible_value_collect_for_relation_value (BobguiAccessibleRelation   relation,
                                                 const GValue           *value,
                                                 GError                **error)
{
  const BobguiAccessibleCollect *cstate = &collect_rels[relation];

  g_return_val_if_fail (relation <= BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM, NULL);

  return bobgui_accessible_value_collect_value (cstate, value, error);
}

BobguiAccessibleValue *
bobgui_accessible_value_parse_for_relation (BobguiAccessibleRelation   relation,
                                         const char             *str,
                                         gsize                   len,
                                         GError                **error)
{
  const BobguiAccessibleCollect *cstate = &collect_rels[relation];

  g_return_val_if_fail (relation <= BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM, NULL);

  return bobgui_accessible_value_parse (cstate, str, len, error);
}

/**
 * bobgui_accessible_relation_init_value:
 * @relation: a `BobguiAccessibleRelation`
 * @value: an uninitialized `GValue`
 *
 * Initializes @value with the appropriate type for the @relation.
 *
 * This function is mostly meant for language bindings, in conjunction
 * with bobgui_accessible_update_relation_value().
 */
void
bobgui_accessible_relation_init_value (BobguiAccessibleRelation  relation,
                                    GValue                *value)
{
  const BobguiAccessibleCollect *cstate = &collect_rels[relation];

  g_return_if_fail (relation <= BOBGUI_ACCESSIBLE_RELATION_FLOW_FROM);

  bobgui_accessible_attribute_init_value (cstate, value);
}

/* }}} */
