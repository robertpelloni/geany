/* bobguiaccessiblevaluestatic.c: BobguiAccessibleValue static implementations
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

#include "config.h"

#include "bobguiaccessiblevalueprivate.h"
#include "bobguibuilderprivate.h"
#include "bobguienums.h"
#include "bobguitypebuiltins.h"

/* {{{ Undefined value */

static void
bobgui_undefined_accessible_value_print (const BobguiAccessibleValue *value,
                                      GString                  *buffer)
{
  g_string_append (buffer, "undefined");
}

static const BobguiAccessibleValueClass BOBGUI_UNDEFINED_ACCESSIBLE_VALUE = {
  .type = BOBGUI_ACCESSIBLE_VALUE_TYPE_UNDEFINED,
  .type_name = "BobguiUndefinedAccessibleValue",
  .instance_size = sizeof (BobguiAccessibleValue),
  .print = bobgui_undefined_accessible_value_print,
};

static BobguiAccessibleValue undefined_value =
  BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_UNDEFINED_ACCESSIBLE_VALUE);

BobguiAccessibleValue *
bobgui_undefined_accessible_value_new (void)
{
  return bobgui_accessible_value_ref (&undefined_value);
}

int
bobgui_undefined_accessible_value_get (const BobguiAccessibleValue *value)
{
  g_return_val_if_fail (value != NULL, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED);
  g_return_val_if_fail (value->value_class == &BOBGUI_UNDEFINED_ACCESSIBLE_VALUE,
                        BOBGUI_ACCESSIBLE_VALUE_UNDEFINED);

  return BOBGUI_ACCESSIBLE_VALUE_UNDEFINED;
}

/* }}} */

/* {{{ Boolean values */ 

typedef struct
{
  BobguiAccessibleValue parent;

  gboolean value;
} BobguiBooleanAccessibleValue;

static gboolean 
bobgui_boolean_accessible_value_equal (const BobguiAccessibleValue *value_a,
                                    const BobguiAccessibleValue *value_b)
{
  const BobguiBooleanAccessibleValue *bool_a = (BobguiBooleanAccessibleValue *) value_a;
  const BobguiBooleanAccessibleValue *bool_b = (BobguiBooleanAccessibleValue *) value_b;

  return bool_a->value == bool_b->value;
}

static void
bobgui_boolean_accessible_value_print (const BobguiAccessibleValue *value,
                                    GString                  *buffer)
{
  const BobguiBooleanAccessibleValue *self = (BobguiBooleanAccessibleValue *) value;

  g_string_append_printf (buffer, "%s", self->value ? "true" : "false");
}

static const BobguiAccessibleValueClass BOBGUI_BOOLEAN_ACCESSIBLE_VALUE = {
  .type = BOBGUI_ACCESSIBLE_VALUE_TYPE_BOOLEAN,
  .type_name = "BobguiBooleanAccessibleValue",
  .instance_size = sizeof (BobguiBooleanAccessibleValue),
  .equal = bobgui_boolean_accessible_value_equal,
  .print = bobgui_boolean_accessible_value_print,
};

static BobguiBooleanAccessibleValue boolean_values[] = {
  { BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_BOOLEAN_ACCESSIBLE_VALUE), FALSE },
  { BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_BOOLEAN_ACCESSIBLE_VALUE), TRUE },
};

BobguiAccessibleValue *
bobgui_boolean_accessible_value_new (gboolean state)
{
  if (state)
    return bobgui_accessible_value_ref ((BobguiAccessibleValue *) &boolean_values[1]);

  return bobgui_accessible_value_ref ((BobguiAccessibleValue *) &boolean_values[0]);
}

gboolean
bobgui_boolean_accessible_value_get (const BobguiAccessibleValue *value)
{
  BobguiBooleanAccessibleValue *self = (BobguiBooleanAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, FALSE);
  g_return_val_if_fail (value->value_class == &BOBGUI_BOOLEAN_ACCESSIBLE_VALUE, FALSE);

  return self->value;
}

/* }}} */

/* {{{ Tri-state values */

typedef struct {
  BobguiAccessibleValue parent;

  BobguiAccessibleTristate value;
} BobguiTristateAccessibleValue;

static gboolean
bobgui_tristate_accessible_value_equal (const BobguiAccessibleValue *value_a,
                                     const BobguiAccessibleValue *value_b)
{
  const BobguiTristateAccessibleValue *self_a = (BobguiTristateAccessibleValue *) value_a;
  const BobguiTristateAccessibleValue *self_b = (BobguiTristateAccessibleValue *) value_b;

  return self_a->value == self_b->value;
}

static void
bobgui_tristate_accessible_value_print (const BobguiAccessibleValue *value,
                                     GString                  *buffer)
{
  const BobguiTristateAccessibleValue *self = (BobguiTristateAccessibleValue *) value;

  switch (self->value)
    {
    case BOBGUI_ACCESSIBLE_TRISTATE_FALSE:
      g_string_append (buffer, "false");
      break;

    case BOBGUI_ACCESSIBLE_TRISTATE_TRUE:
      g_string_append (buffer, "true");
      break;

    case BOBGUI_ACCESSIBLE_TRISTATE_MIXED:
      g_string_append (buffer, "mixed");
      break;

    default:
      g_assert_not_reached ();
      break;
    }
}

static const BobguiAccessibleValueClass BOBGUI_TRISTATE_ACCESSIBLE_VALUE = {
  .type = BOBGUI_ACCESSIBLE_VALUE_TYPE_TRISTATE,
  .type_name = "BobguiTristateAccessibleValue",
  .instance_size = sizeof (BobguiTristateAccessibleValue),
  .equal = bobgui_tristate_accessible_value_equal,
  .print = bobgui_tristate_accessible_value_print,
};

static BobguiTristateAccessibleValue tristate_values[] = {
  [BOBGUI_ACCESSIBLE_TRISTATE_FALSE] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_TRISTATE_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_TRISTATE_FALSE
  },
  [BOBGUI_ACCESSIBLE_TRISTATE_TRUE] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_TRISTATE_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_TRISTATE_TRUE
  },
  [BOBGUI_ACCESSIBLE_TRISTATE_MIXED] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_TRISTATE_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_TRISTATE_MIXED
  },
};

BobguiAccessibleValue *
bobgui_tristate_accessible_value_new (BobguiAccessibleTristate value)
{
  g_return_val_if_fail (value >= BOBGUI_ACCESSIBLE_TRISTATE_FALSE &&
                        value <= BOBGUI_ACCESSIBLE_TRISTATE_MIXED, NULL);

  return bobgui_accessible_value_ref ((BobguiAccessibleValue *) &tristate_values[value]);
}

BobguiAccessibleTristate
bobgui_tristate_accessible_value_get (const BobguiAccessibleValue *value)
{
  g_return_val_if_fail (value != NULL, BOBGUI_ACCESSIBLE_TRISTATE_FALSE);
  g_return_val_if_fail (value->value_class == &BOBGUI_TRISTATE_ACCESSIBLE_VALUE,
                        BOBGUI_ACCESSIBLE_TRISTATE_FALSE);

  BobguiTristateAccessibleValue *self = (BobguiTristateAccessibleValue *) value;

  return self->value;
}

/* }}} */

/* {{{ Enumeration values */

typedef struct {
  BobguiAccessibleValue parent;

  int value;
  const char *token;
} BobguiTokenAccessibleValue;

static gboolean
bobgui_token_accessible_value_equal (const BobguiAccessibleValue *value_a,
                                  const BobguiAccessibleValue *value_b)
{
  const BobguiTokenAccessibleValue *self_a = (BobguiTokenAccessibleValue *) value_a;
  const BobguiTokenAccessibleValue *self_b = (BobguiTokenAccessibleValue *) value_b;

  return self_a->value == self_b->value;
}

static void
bobgui_token_accessible_value_print (const BobguiAccessibleValue *value,
                                  GString                  *buffer)
{
  const BobguiTokenAccessibleValue *self = (BobguiTokenAccessibleValue *) value;

  g_string_append (buffer, self->token);
}

static const BobguiAccessibleValueClass BOBGUI_INVALID_ACCESSIBLE_VALUE = {
  .type = BOBGUI_ACCESSIBLE_VALUE_TYPE_TOKEN,
  .type_name = "BobguiInvalidAccessibleValue",
  .instance_size = sizeof (BobguiTokenAccessibleValue),
  .equal = bobgui_token_accessible_value_equal,
  .print = bobgui_token_accessible_value_print,
};

static BobguiTokenAccessibleValue invalid_values[] = {
  [BOBGUI_ACCESSIBLE_INVALID_FALSE] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_INVALID_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_INVALID_FALSE, "false"
  },
  [BOBGUI_ACCESSIBLE_INVALID_TRUE] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_INVALID_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_INVALID_TRUE, "true"
  },
  [BOBGUI_ACCESSIBLE_INVALID_GRAMMAR] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_INVALID_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_INVALID_GRAMMAR, "grammar"
  },
  [BOBGUI_ACCESSIBLE_INVALID_SPELLING] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_INVALID_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_INVALID_SPELLING, "spelling"
  },
};

BobguiAccessibleValue *
bobgui_invalid_accessible_value_new (BobguiAccessibleInvalidState state)
{
  g_return_val_if_fail (state >= BOBGUI_ACCESSIBLE_INVALID_FALSE &&
                        state <= BOBGUI_ACCESSIBLE_INVALID_SPELLING,
                        NULL);

  return bobgui_accessible_value_ref ((BobguiAccessibleValue *) &invalid_values[state]);
}

BobguiAccessibleInvalidState
bobgui_invalid_accessible_value_get (const BobguiAccessibleValue *value)
{
  BobguiTokenAccessibleValue *self = (BobguiTokenAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, BOBGUI_ACCESSIBLE_INVALID_FALSE);
  g_return_val_if_fail (value->value_class == &BOBGUI_INVALID_ACCESSIBLE_VALUE,
                        BOBGUI_ACCESSIBLE_INVALID_FALSE);

  return self->value;
}

BobguiAccessibleValue *
bobgui_invalid_accessible_value_parse (const char  *str,
                                    gsize        len,
                                    GError     **error)
{
  int value;

  if (_bobgui_builder_enum_from_string (BOBGUI_TYPE_ACCESSIBLE_INVALID_STATE, str, &value, error))
    return bobgui_invalid_accessible_value_new (value);

  return NULL;
}

void
bobgui_invalid_accessible_value_init_value (GValue *value)
{
  g_value_init (value, BOBGUI_TYPE_ACCESSIBLE_INVALID_STATE);
}

static const BobguiAccessibleValueClass BOBGUI_AUTOCOMPLETE_ACCESSIBLE_VALUE = {
  .type = BOBGUI_ACCESSIBLE_VALUE_TYPE_TOKEN,
  .type_name = "BobguiAutocompleteAccessibleValue",
  .instance_size = sizeof (BobguiTokenAccessibleValue),
  .equal = bobgui_token_accessible_value_equal,
  .print = bobgui_token_accessible_value_print,
};

static BobguiTokenAccessibleValue autocomplete_values[] = {
  [BOBGUI_ACCESSIBLE_AUTOCOMPLETE_NONE] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_AUTOCOMPLETE_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_AUTOCOMPLETE_NONE, "none"
  },
  [BOBGUI_ACCESSIBLE_AUTOCOMPLETE_INLINE] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_AUTOCOMPLETE_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_AUTOCOMPLETE_INLINE, "inline"
  },
  [BOBGUI_ACCESSIBLE_AUTOCOMPLETE_LIST] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_AUTOCOMPLETE_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_AUTOCOMPLETE_LIST, "list"
  },
  [BOBGUI_ACCESSIBLE_AUTOCOMPLETE_BOTH] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_AUTOCOMPLETE_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_AUTOCOMPLETE_BOTH, "both"
  },
};

BobguiAccessibleValue *
bobgui_autocomplete_accessible_value_new (BobguiAccessibleAutocomplete value)
{
  g_return_val_if_fail (value >= BOBGUI_ACCESSIBLE_AUTOCOMPLETE_NONE &&
                        value <= BOBGUI_ACCESSIBLE_AUTOCOMPLETE_BOTH,
                        NULL);

  return bobgui_accessible_value_ref ((BobguiAccessibleValue *) &autocomplete_values[value]);
}

BobguiAccessibleAutocomplete
bobgui_autocomplete_accessible_value_get (const BobguiAccessibleValue *value)
{
  BobguiTokenAccessibleValue *self = (BobguiTokenAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, BOBGUI_ACCESSIBLE_AUTOCOMPLETE_NONE);
  g_return_val_if_fail (value->value_class == &BOBGUI_AUTOCOMPLETE_ACCESSIBLE_VALUE,
                        BOBGUI_ACCESSIBLE_AUTOCOMPLETE_NONE);

  return self->value;
}

BobguiAccessibleValue *
bobgui_autocomplete_accessible_value_parse (const char  *str,
                                         gsize        len,
                                         GError     **error)
{
  int value;

  if (_bobgui_builder_enum_from_string (BOBGUI_TYPE_ACCESSIBLE_AUTOCOMPLETE, str, &value, error))
    return bobgui_autocomplete_accessible_value_new (value);

  return NULL;
}

void
bobgui_autocomplete_accessible_value_init_value (GValue *value)
{
  g_value_init (value, BOBGUI_TYPE_ACCESSIBLE_AUTOCOMPLETE);
}

static const BobguiAccessibleValueClass BOBGUI_ORIENTATION_ACCESSIBLE_VALUE = {
  .type = BOBGUI_ACCESSIBLE_VALUE_TYPE_TOKEN,
  .type_name = "BobguiOrientationAccessibleValue",
  .instance_size = sizeof (BobguiTokenAccessibleValue),
  .equal = bobgui_token_accessible_value_equal,
  .print = bobgui_token_accessible_value_print,
};

static BobguiTokenAccessibleValue orientation_values[] = {
  [BOBGUI_ORIENTATION_HORIZONTAL] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_ORIENTATION_ACCESSIBLE_VALUE), BOBGUI_ORIENTATION_HORIZONTAL, "horizontal"
  },
  [BOBGUI_ORIENTATION_VERTICAL] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_ORIENTATION_ACCESSIBLE_VALUE), BOBGUI_ORIENTATION_VERTICAL, "vertical"
  },
};

BobguiAccessibleValue *
bobgui_orientation_accessible_value_new (BobguiOrientation value)
{
  g_return_val_if_fail (value >= BOBGUI_ORIENTATION_HORIZONTAL &&
                        value <= BOBGUI_ORIENTATION_VERTICAL,
                        NULL);

  return bobgui_accessible_value_ref ((BobguiAccessibleValue *) &orientation_values[value]);
}

BobguiOrientation
bobgui_orientation_accessible_value_get (const BobguiAccessibleValue *value)
{
  BobguiTokenAccessibleValue *self = (BobguiTokenAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED);
  g_return_val_if_fail (value->value_class == &BOBGUI_ORIENTATION_ACCESSIBLE_VALUE,
                        BOBGUI_ACCESSIBLE_VALUE_UNDEFINED);

  return self->value;
}

BobguiAccessibleValue *
bobgui_orientation_accessible_value_parse (const char  *str,
                                        gsize        len,
                                        GError     **error)
{
  int value;

  if (_bobgui_builder_enum_from_string (BOBGUI_TYPE_ORIENTATION, str, &value, error))
    return bobgui_orientation_accessible_value_new (value);

  return NULL;
}

void
bobgui_orientation_accessible_value_init_value (GValue *value)
{
  g_value_init (value, BOBGUI_TYPE_ORIENTATION);
}

static const BobguiAccessibleValueClass BOBGUI_SORT_ACCESSIBLE_VALUE = {
  .type = BOBGUI_ACCESSIBLE_VALUE_TYPE_TOKEN,
  .type_name = "BobguiSortAccessibleValue",
  .instance_size = sizeof (BobguiTokenAccessibleValue),
  .equal = bobgui_token_accessible_value_equal,
  .print = bobgui_token_accessible_value_print,
};

static BobguiTokenAccessibleValue sort_values[] = {
  [BOBGUI_ACCESSIBLE_SORT_NONE] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_SORT_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_SORT_NONE, "none"
  },
  [BOBGUI_ACCESSIBLE_SORT_ASCENDING] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_SORT_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_SORT_ASCENDING, "ascending"
  },
  [BOBGUI_ACCESSIBLE_SORT_DESCENDING] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_SORT_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_SORT_DESCENDING, "descending"
  },
  [BOBGUI_ACCESSIBLE_SORT_OTHER] = {
    BOBGUI_ACCESSIBLE_VALUE_INIT (&BOBGUI_SORT_ACCESSIBLE_VALUE), BOBGUI_ACCESSIBLE_SORT_OTHER, "other"
  },
};

BobguiAccessibleValue *
bobgui_sort_accessible_value_new (BobguiAccessibleSort value)
{
  g_return_val_if_fail (value >= BOBGUI_ACCESSIBLE_SORT_NONE &&
                        value <= BOBGUI_ACCESSIBLE_SORT_OTHER,
                        NULL);

  return bobgui_accessible_value_ref ((BobguiAccessibleValue *) &sort_values[value]);
}

BobguiAccessibleSort
bobgui_sort_accessible_value_get (const BobguiAccessibleValue *value)
{
  BobguiTokenAccessibleValue *self = (BobguiTokenAccessibleValue *) value;

  g_return_val_if_fail (value != NULL, BOBGUI_ACCESSIBLE_SORT_NONE);
  g_return_val_if_fail (value->value_class == &BOBGUI_SORT_ACCESSIBLE_VALUE,
                        BOBGUI_ACCESSIBLE_SORT_NONE);

  return self->value;
}

BobguiAccessibleValue *
bobgui_sort_accessible_value_parse (const char  *str,
                                 gsize        len,
                                 GError     **error)
{
  int value;

  if (_bobgui_builder_enum_from_string (BOBGUI_TYPE_ACCESSIBLE_SORT, str, &value, error))
    return bobgui_sort_accessible_value_new (value);

  return NULL;
}

void
bobgui_sort_accessible_value_init_value (GValue *value)
{
  g_value_init (value, BOBGUI_TYPE_ACCESSIBLE_SORT);
}

/* }}} */
