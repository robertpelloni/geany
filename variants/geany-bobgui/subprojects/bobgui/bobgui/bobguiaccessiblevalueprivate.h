/* bobguiaccessiblevalueprivate.h: Accessible value
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

#pragma once

#include <glib-object.h>

#include "bobguiaccessible.h"
#include "bobguienums.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_ACCESSIBLE_VALUE (bobgui_accessible_value_get_type())

#define BOBGUI_ACCESSIBLE_VALUE_ERROR (bobgui_accessible_value_error_quark())

typedef struct _BobguiAccessibleValue      BobguiAccessibleValue;
typedef struct _BobguiAccessibleValueClass BobguiAccessibleValueClass;

typedef enum {
  BOBGUI_ACCESSIBLE_VALUE_TYPE_UNDEFINED,
  BOBGUI_ACCESSIBLE_VALUE_TYPE_BOOLEAN,
  BOBGUI_ACCESSIBLE_VALUE_TYPE_TRISTATE,
  BOBGUI_ACCESSIBLE_VALUE_TYPE_TOKEN,
  BOBGUI_ACCESSIBLE_VALUE_TYPE_INTEGER,
  BOBGUI_ACCESSIBLE_VALUE_TYPE_NUMBER,
  BOBGUI_ACCESSIBLE_VALUE_TYPE_STRING,
  BOBGUI_ACCESSIBLE_VALUE_TYPE_REFERENCE,
  BOBGUI_ACCESSIBLE_VALUE_TYPE_REFERENCE_LIST
} BobguiAccessibleValueType;

struct _BobguiAccessibleValue
{
  const BobguiAccessibleValueClass *value_class;

  int ref_count;
};

#define BOBGUI_ACCESSIBLE_VALUE_INIT(klass)        { .value_class = (klass), .ref_count = 1 }

struct _BobguiAccessibleValueClass
{
  BobguiAccessibleValueType type;
  const char *type_name;
  gsize instance_size;

  void (* init) (BobguiAccessibleValue *self);
  void (* finalize) (BobguiAccessibleValue *self);
  void (* print) (const BobguiAccessibleValue *self,
                  GString *string);
  gboolean (* equal) (const BobguiAccessibleValue *value_a,
                      const BobguiAccessibleValue *value_b);
};

#define BOBGUI_IS_ACCESSIBLE_VALUE_TYPE(v,type) \
  ((v)->value_class->type == (type))

typedef enum {
  BOBGUI_ACCESSIBLE_VALUE_ERROR_READ_ONLY,
  BOBGUI_ACCESSIBLE_VALUE_ERROR_INVALID_VALUE,
  BOBGUI_ACCESSIBLE_VALUE_ERROR_INVALID_RANGE,
  BOBGUI_ACCESSIBLE_VALUE_ERROR_INVALID_TOKEN
} BobguiAccessibleValueError;

GType                   bobgui_accessible_value_get_type                   (void) G_GNUC_CONST;
GQuark                  bobgui_accessible_value_error_quark                (void);

BobguiAccessibleValue *    bobgui_accessible_value_alloc                      (const BobguiAccessibleValueClass *klass);
BobguiAccessibleValue *    bobgui_accessible_value_ref                        (BobguiAccessibleValue            *self);
void                    bobgui_accessible_value_unref                      (BobguiAccessibleValue            *self);
void                    bobgui_accessible_value_print                      (const BobguiAccessibleValue      *self,
                                                                         GString                       *buffer);
char *                  bobgui_accessible_value_to_string                  (const BobguiAccessibleValue      *self);
gboolean                bobgui_accessible_value_equal                      (const BobguiAccessibleValue      *value_a,
                                                                         const BobguiAccessibleValue      *value_b);

BobguiAccessibleValue *    bobgui_accessible_value_get_default_for_state      (BobguiAccessibleState             state);
BobguiAccessibleValue *    bobgui_accessible_value_collect_for_state          (BobguiAccessibleState             state,
                                                                         GError                       **error,
                                                                         va_list                       *args);
BobguiAccessibleValue *    bobgui_accessible_value_collect_for_state_value    (BobguiAccessibleState             state,
                                                                         const GValue                  *value,
                                                                         GError                       **error);
BobguiAccessibleValue *    bobgui_accessible_value_parse_for_state            (BobguiAccessibleState             state,
                                                                         const char                    *str,
                                                                         gsize                          len,
                                                                         GError                       **error);

BobguiAccessibleValue *    bobgui_accessible_value_get_default_for_property   (BobguiAccessibleProperty          property);
BobguiAccessibleValue *    bobgui_accessible_value_collect_for_property       (BobguiAccessibleProperty          property,
                                                                         GError                       **error,
                                                                         va_list                       *args);
BobguiAccessibleValue *    bobgui_accessible_value_collect_for_property_value (BobguiAccessibleProperty          property,
                                                                         const GValue                  *value,
                                                                         GError                       **error);
BobguiAccessibleValue *    bobgui_accessible_value_parse_for_property         (BobguiAccessibleProperty          property,
                                                                         const char                    *str,
                                                                         gsize                          len,
                                                                         GError                       **error);

BobguiAccessibleValue *    bobgui_accessible_value_get_default_for_relation   (BobguiAccessibleRelation          relation);
BobguiAccessibleValue *    bobgui_accessible_value_collect_for_relation       (BobguiAccessibleRelation          relation,
                                                                         GError                       **error,
                                                                         va_list                       *args);
BobguiAccessibleValue *    bobgui_accessible_value_collect_for_relation_value (BobguiAccessibleRelation          relation,
                                                                         const GValue                  *value,
                                                                         GError                       **error);
BobguiAccessibleValue *    bobgui_accessible_value_parse_for_relation         (BobguiAccessibleRelation          relation,
                                                                         const char                    *str,
                                                                         gsize                          len,
                                                                         GError                       **error);

/* Basic values */
BobguiAccessibleValue *            bobgui_undefined_accessible_value_new      (void);
int                             bobgui_undefined_accessible_value_get      (const BobguiAccessibleValue *value);

BobguiAccessibleValue *            bobgui_boolean_accessible_value_new        (gboolean                  value);
gboolean                        bobgui_boolean_accessible_value_get        (const BobguiAccessibleValue *value);

BobguiAccessibleValue *            bobgui_tristate_accessible_value_new       (BobguiAccessibleTristate     value);
BobguiAccessibleTristate           bobgui_tristate_accessible_value_get       (const BobguiAccessibleValue *value);

BobguiAccessibleValue *            bobgui_int_accessible_value_new            (int                       value);
int                             bobgui_int_accessible_value_get            (const BobguiAccessibleValue *value);

BobguiAccessibleValue *            bobgui_number_accessible_value_new         (double                    value);
double                          bobgui_number_accessible_value_get         (const BobguiAccessibleValue *value);

BobguiAccessibleValue *            bobgui_string_accessible_value_new         (const char               *value);
const char *                    bobgui_string_accessible_value_get         (const BobguiAccessibleValue *value);

BobguiAccessibleValue *            bobgui_reference_accessible_value_new      (BobguiAccessible            *value);
BobguiAccessible *                 bobgui_reference_accessible_value_get      (const BobguiAccessibleValue *value);

BobguiAccessibleValue *            bobgui_reference_list_accessible_value_new (GList                    *value);
GList *                         bobgui_reference_list_accessible_value_get (const BobguiAccessibleValue *value);
void                            bobgui_reference_list_accessible_value_append (BobguiAccessibleValue *value,
                                                                            BobguiAccessible      *reference);
void                            bobgui_reference_list_accessible_value_remove (BobguiAccessibleValue *value,
                                                                            BobguiAccessible      *reference);

/* Token values */
BobguiAccessibleValue *            bobgui_invalid_accessible_value_new        (BobguiAccessibleInvalidState value);
BobguiAccessibleInvalidState       bobgui_invalid_accessible_value_get        (const BobguiAccessibleValue *value);
BobguiAccessibleValue *            bobgui_invalid_accessible_value_parse      (const char               *str,
                                                                         gsize                     len,
                                                                         GError                  **error);
void                            bobgui_invalid_accessible_value_init_value (GValue                   *value);

BobguiAccessibleValue *            bobgui_autocomplete_accessible_value_new   (BobguiAccessibleAutocomplete value);
BobguiAccessibleAutocomplete       bobgui_autocomplete_accessible_value_get   (const BobguiAccessibleValue *value);
BobguiAccessibleValue *            bobgui_autocomplete_accessible_value_parse (const char               *str,
                                                                         gsize                     len,
                                                                         GError                  **error);
void                            bobgui_autocomplete_accessible_value_init_value (GValue              *value);

BobguiAccessibleValue *            bobgui_orientation_accessible_value_new    (BobguiOrientation            value);
BobguiOrientation                  bobgui_orientation_accessible_value_get    (const BobguiAccessibleValue *value);
BobguiAccessibleValue *            bobgui_orientation_accessible_value_parse  (const char               *str,
                                                                         gsize                     len,
                                                                         GError                  **error);
void                            bobgui_orientation_accessible_value_init_value (GValue               *value);

BobguiAccessibleValue *            bobgui_sort_accessible_value_new           (BobguiAccessibleSort         value);
BobguiAccessibleSort               bobgui_sort_accessible_value_get           (const BobguiAccessibleValue *value);
BobguiAccessibleValue *            bobgui_sort_accessible_value_parse         (const char               *str,
                                                                         gsize                     len,
                                                                         GError                  **error);
void                            bobgui_sort_accessible_value_init_value    (GValue                   *value);

G_END_DECLS
