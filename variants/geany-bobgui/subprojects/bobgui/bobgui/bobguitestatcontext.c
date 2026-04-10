/* bobguitestatcontext.c: Test AT context
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

#include "bobguitestatcontextprivate.h"

#include "bobguiatcontextprivate.h"
#include "bobguiaccessibleprivate.h"
#include "bobguiaccessibletextprivate.h"
#include "bobguidebug.h"
#include "bobguienums.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"

struct _BobguiTestATContext
{
  BobguiATContext parent_instance;
};

struct _BobguiTestATContextClass
{
  BobguiATContextClass parent_class;
};

enum {
  UPDATE_CARET_POSITION,
  UPDATE_SELECTION_BOUND,
  UPDATE_TEXT_CONTENTS,
  LAST_SIGNAL,
};

static guint signals[LAST_SIGNAL] = { 0, };

G_DEFINE_TYPE (BobguiTestATContext, bobgui_test_at_context, BOBGUI_TYPE_AT_CONTEXT)

static void
bobgui_test_at_context_state_change (BobguiATContext                *self,
                                  BobguiAccessibleStateChange     changed_states,
                                  BobguiAccessiblePropertyChange  changed_properties,
                                  BobguiAccessibleRelationChange  changed_relations,
                                  BobguiAccessibleAttributeSet   *states,
                                  BobguiAccessibleAttributeSet   *properties,
                                  BobguiAccessibleAttributeSet   *relations)
{
  if (BOBGUI_DEBUG_CHECK (A11Y))
    {
      char *states_str = bobgui_accessible_attribute_set_to_string (states);
      char *properties_str = bobgui_accessible_attribute_set_to_string (properties);
      char *relations_str = bobgui_accessible_attribute_set_to_string (relations);
      BobguiAccessibleRole role = bobgui_at_context_get_accessible_role (self);
      BobguiAccessible *accessible = bobgui_at_context_get_accessible (self);
      GEnumClass *class = g_type_class_ref (BOBGUI_TYPE_ACCESSIBLE_ROLE);
      GEnumValue *value = g_enum_get_value (class, role);

      g_print ("*** Accessible state changed for accessible “%s”, with role “%s” (%d):\n"
           "***     states = %s\n"
           "*** properties = %s\n"
           "***  relations = %s\n",
            G_OBJECT_TYPE_NAME (accessible),
           value->value_nick,
           role,
           states_str,
           properties_str,
           relations_str);
      g_type_class_unref (class);

      g_free (states_str);
      g_free (properties_str);
      g_free (relations_str);
    }
}

static void
bobgui_test_at_context_platform_change (BobguiATContext                *self,
                                     BobguiAccessiblePlatformChange  changed_platform)
{
  if (BOBGUI_DEBUG_CHECK (A11Y))
    {
      BobguiAccessible *accessible;

      accessible = bobgui_at_context_get_accessible (self);

      g_print ("*** Accessible platform state changed for accessible “%s”:\n",
               G_OBJECT_TYPE_NAME (accessible));
      if (changed_platform & BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_FOCUSABLE)
        g_print ("***  focusable = %d\n",
                 bobgui_accessible_get_platform_state (accessible, BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSABLE));
      if (changed_platform & BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_FOCUSED)
        g_print ("***    focused = %d\n",
                 bobgui_accessible_get_platform_state (accessible, BOBGUI_ACCESSIBLE_PLATFORM_STATE_FOCUSED));
      if (changed_platform & BOBGUI_ACCESSIBLE_PLATFORM_CHANGE_ACTIVE)
        g_print ("***    active = %d\n",
                 bobgui_accessible_get_platform_state (accessible, BOBGUI_ACCESSIBLE_PLATFORM_STATE_ACTIVE));
    }
}

static void
bobgui_test_at_context_update_caret_position (BobguiATContext *self)
{
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (self);
  BobguiAccessibleText *accessible_text = BOBGUI_ACCESSIBLE_TEXT (accessible);
  unsigned int position;

  position = bobgui_accessible_text_get_caret_position (accessible_text);
  g_signal_emit (self, signals[UPDATE_CARET_POSITION], 0, position);
}

static void
bobgui_test_at_context_update_selection_bound (BobguiATContext *self)
{
  g_signal_emit (self, signals[UPDATE_SELECTION_BOUND], 0);
}

static void
bobgui_test_at_context_update_text_contents (BobguiATContext                   *self,
                                          BobguiAccessibleTextContentChange  change,
                                          unsigned int                    start,
                                          unsigned int                    end)
{
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (self);
  BobguiAccessibleText *accessible_text = BOBGUI_ACCESSIBLE_TEXT (accessible);
  GBytes *contents;

  contents = bobgui_accessible_text_get_contents (accessible_text, start, end);
  g_signal_emit (self, signals[UPDATE_TEXT_CONTENTS], 0, change, start, end, contents);
  g_bytes_unref (contents);
}

static void
bobgui_test_at_context_class_init (BobguiTestATContextClass *klass)
{
  BobguiATContextClass *context_class = BOBGUI_AT_CONTEXT_CLASS (klass);

  context_class->state_change = bobgui_test_at_context_state_change;
  context_class->platform_change = bobgui_test_at_context_platform_change;

  context_class->update_caret_position = bobgui_test_at_context_update_caret_position;
  context_class->update_selection_bound = bobgui_test_at_context_update_selection_bound;
  context_class->update_text_contents = bobgui_test_at_context_update_text_contents;

  signals[UPDATE_CARET_POSITION] =
    g_signal_new ("update-caret-position",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 1, G_TYPE_UINT);

  signals[UPDATE_SELECTION_BOUND] =
    g_signal_new ("update-selection-bound",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 0);

  signals[UPDATE_TEXT_CONTENTS] =
    g_signal_new ("update-text-contents",
                  G_TYPE_FROM_CLASS (klass),
                  G_SIGNAL_RUN_FIRST,
                  0,
                  NULL, NULL,
                  NULL,
                  G_TYPE_NONE, 4,
                  BOBGUI_TYPE_ACCESSIBLE_TEXT_CONTENT_CHANGE,
                  G_TYPE_UINT,
                  G_TYPE_UINT,
                  G_TYPE_BYTES);
}

static void
bobgui_test_at_context_init (BobguiTestATContext *self)
{
}

/*< private >
 * bobgui_test_at_context_new:
 * @accessible_role: the `BobguiAccessibleRole` for the AT context
 * @accessible: the `BobguiAccessible` instance which owns the AT context
 * @display: a `GdkDisplay`
 *
 * Creates a new `BobguiTestATContext` instance for @accessible, using the
 * given @accessible_role.
 *
 * Returns: (transfer full): the newly created `BobguiTestATContext` instance
 */
BobguiATContext *
bobgui_test_at_context_new (BobguiAccessibleRole  accessible_role,
                         BobguiAccessible     *accessible,
                         GdkDisplay        *display)
{
  return g_object_new (BOBGUI_TYPE_TEST_AT_CONTEXT,
                       "accessible-role", accessible_role,
                       "accessible", accessible,
                       "display", display,
                       NULL);
}

/**
 * bobgui_test_accessible_has_role:
 * @accessible: a `BobguiAccessible`
 * @role: a `BobguiAccessibleRole`
 *
 * Checks whether the `BobguiAccessible:accessible-role` of the accessible
 * is @role.
 *
 * Returns: %TRUE if the role matches
 */
gboolean
bobgui_test_accessible_has_role (BobguiAccessible     *accessible,
                              BobguiAccessibleRole  role)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (accessible), FALSE);

  return bobgui_accessible_get_accessible_role (accessible) == role;
}

/**
 * bobgui_test_accessible_has_property:
 * @accessible: a `BobguiAccessible`
 * @property: a `BobguiAccessibleProperty`
 *
 * Checks whether the `BobguiAccessible` has @property set.
 *
 * Returns: %TRUE if the @property is set in the @accessible
 */
gboolean
bobgui_test_accessible_has_property (BobguiAccessible         *accessible,
                                  BobguiAccessibleProperty  property)
{
  BobguiATContext *context;
  gboolean res;

  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (accessible), FALSE);

  context = bobgui_accessible_get_at_context (accessible);
  if (context == NULL)
    return FALSE;

  res = bobgui_at_context_has_accessible_property (context, property);

  g_object_unref (context);

  return res;
}

/**
 * bobgui_test_accessible_check_property:
 * @accessible: a `BobguiAccessible`
 * @property: a `BobguiAccessibleProperty`
 * @...: the expected value of @property
 *
 * Checks whether the accessible @property of @accessible is set to
 * a specific value.
 *
 * Returns: (transfer full): the value of the accessible property
 */
char *
bobgui_test_accessible_check_property (BobguiAccessible         *accessible,
                                    BobguiAccessibleProperty  property,
                                    ...)
{
  char *res = NULL;
  va_list args;

  va_start (args, property);

  GError *error = NULL;
  BobguiAccessibleValue *check_value =
    bobgui_accessible_value_collect_for_property (property, &error, &args);

  va_end (args);

  if (error != NULL)
    {
      res = g_strdup (error->message);
      g_error_free (error);
      return res;
    }

  if (check_value == NULL)
    check_value = bobgui_accessible_value_get_default_for_property (property);

  BobguiATContext *context = bobgui_accessible_get_at_context (accessible);
  BobguiAccessibleValue *real_value =
    bobgui_at_context_get_accessible_property (context, property);

  if (bobgui_accessible_value_equal (check_value, real_value))
    goto out;

  res = bobgui_accessible_value_to_string (real_value);

out:
  bobgui_accessible_value_unref (check_value);
  g_object_unref (context);

  return res;
}

/**
 * bobgui_test_accessible_has_state:
 * @accessible: a `BobguiAccessible`
 * @state: a `BobguiAccessibleState`
 *
 * Checks whether the `BobguiAccessible` has @state set.
 *
 * Returns: %TRUE if the @state is set in the @accessible
 */
gboolean
bobgui_test_accessible_has_state (BobguiAccessible      *accessible,
                               BobguiAccessibleState  state)
{
  BobguiATContext *context;
  gboolean res;

  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (accessible), FALSE);

  context = bobgui_accessible_get_at_context (accessible);
  if (context == NULL)
    return FALSE;

  res = bobgui_at_context_has_accessible_state (context, state);

  g_object_unref (context);

  return res;
}

/**
 * bobgui_test_accessible_check_state:
 * @accessible: a `BobguiAccessible`
 * @state: a `BobguiAccessibleState`
 * @...: the expected value of @state
 *
 * Checks whether the accessible @state of @accessible is set to
 * a specific value.
 *
 * Returns: (transfer full): the value of the accessible state
 */
char *
bobgui_test_accessible_check_state (BobguiAccessible      *accessible,
                                 BobguiAccessibleState  state,
                                 ...)
{
  char *res = NULL;
  va_list args;

  va_start (args, state);

  GError *error = NULL;
  BobguiAccessibleValue *check_value =
    bobgui_accessible_value_collect_for_state (state, &error, &args);

  va_end (args);

  if (error != NULL)
    {
      res = g_strdup (error->message);
      g_error_free (error);
      return res;
    }

  if (check_value == NULL)
    check_value = bobgui_accessible_value_get_default_for_state (state);

  BobguiATContext *context = bobgui_accessible_get_at_context (accessible);
  BobguiAccessibleValue *real_value =
    bobgui_at_context_get_accessible_state (context, state);

  if (bobgui_accessible_value_equal (check_value, real_value))
    goto out;

  res = bobgui_accessible_value_to_string (real_value);

out:
  bobgui_accessible_value_unref (check_value);
  g_object_unref (context);

  return res;
}

/**
 * bobgui_test_accessible_has_relation:
 * @accessible: a `BobguiAccessible`
 * @relation: a `BobguiAccessibleRelation`
 *
 * Checks whether the `BobguiAccessible` has @relation set.
 *
 * Returns: %TRUE if the @relation is set in the @accessible
 */
gboolean
bobgui_test_accessible_has_relation (BobguiAccessible         *accessible,
                                  BobguiAccessibleRelation  relation)
{
  BobguiATContext *context;
  gboolean res;

  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE (accessible), FALSE);

  context = bobgui_accessible_get_at_context (accessible);
  if (context == NULL)
    return FALSE;

  res = bobgui_at_context_has_accessible_relation (context, relation);

  g_object_unref (context);

  return res;
}

/**
 * bobgui_test_accessible_check_relation:
 * @accessible: a `BobguiAccessible`
 * @relation: a `BobguiAccessibleRelation`
 * @...: the expected value of @relation
 *
 * Checks whether the accessible @relation of @accessible is set to
 * a specific value.
 *
 * Returns: (transfer full): the value of the accessible relation
 */
char *
bobgui_test_accessible_check_relation (BobguiAccessible         *accessible,
                                    BobguiAccessibleRelation  relation,
                                    ...)
{
  char *res = NULL;
  va_list args;

  va_start (args, relation);

  GError *error = NULL;
  BobguiAccessibleValue *check_value =
    bobgui_accessible_value_collect_for_relation (relation, &error, &args);

  va_end (args);

  if (error != NULL)
    {
      res = g_strdup (error->message);
      g_error_free (error);
      return res;
    }

  if (check_value == NULL)
    check_value = bobgui_accessible_value_get_default_for_relation (relation);

  BobguiATContext *context = bobgui_accessible_get_at_context (accessible);
  BobguiAccessibleValue *real_value =
    bobgui_at_context_get_accessible_relation (context, relation);

  if (bobgui_accessible_value_equal (check_value, real_value))
    goto out;

  res = bobgui_accessible_value_to_string (real_value);

out:
  bobgui_accessible_value_unref (check_value);
  g_object_unref (context);

  return res;
}

/**
 * bobgui_test_accessible_assertion_message_role:
 * @domain: a domain
 * @file: a file name
 * @line: the line in @file
 * @func: a function name in @file
 * @expr: the expression being tested
 * @accessible: a `BobguiAccessible`
 * @expected_role: the expected `BobguiAccessibleRole`
 * @actual_role: the actual `BobguiAccessibleRole`
 *
 * Prints an assertion message for bobgui_test_accessible_assert_role().
 */
void
bobgui_test_accessible_assertion_message_role (const char        *domain,
                                            const char        *file,
                                            int                line,
                                            const char        *func,
                                            const char        *expr,
                                            BobguiAccessible     *accessible,
                                            BobguiAccessibleRole  expected_role,
                                            BobguiAccessibleRole  actual_role)
{
  char *role_name = g_enum_to_string (BOBGUI_TYPE_ACCESSIBLE_ROLE, actual_role);
  char *s = g_strdup_printf ("assertion failed: (%s): %s.accessible-role = %s (%d)",
                             expr,
                             G_OBJECT_TYPE_NAME (accessible),
                             role_name,
                             actual_role);

  g_assertion_message (domain, file, line, func, s);

  g_free (role_name);
  g_free (s);
}
