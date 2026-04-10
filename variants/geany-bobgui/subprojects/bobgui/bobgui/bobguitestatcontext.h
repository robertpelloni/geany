/* bobguitestatcontext.h: Test AT context
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

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiatcontext.h>

G_BEGIN_DECLS

/**
 * bobgui_test_accessible_assert_role:
 * @accessible: a `BobguiAccessible`
 * @role: a `BobguiAccessibleRole`
 *
 * Checks whether a `BobguiAccessible` implementation has the given @role,
 * and raises an assertion if the condition is failed.
 */
#define bobgui_test_accessible_assert_role(accessible,role) \
G_STMT_START { \
  BobguiAccessible *__a = BOBGUI_ACCESSIBLE (accessible); \
  BobguiAccessibleRole __r1 = (role); \
  BobguiAccessibleRole __r2 = bobgui_accessible_get_accessible_role (__a); \
  if (__r1 == __r2) ; else { \
    bobgui_test_accessible_assertion_message_role (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, \
                                                #accessible ".accessible-role == " #role, \
                                                __a, __r1, __r2); \
  } \
} G_STMT_END

/**
 * bobgui_test_accessible_assert_property:
 * @accessible: a `BobguiAccessible`
 * @property: a `BobguiAccessibleProperty`
 * @...: the value of @property
 *
 * Checks whether a `BobguiAccessible` implementation has its accessible
 * property set to the expected value, and raises an assertion if the
 * condition is not satisfied.
 */
#define bobgui_test_accessible_assert_property(accessible,property,...) \
G_STMT_START { \
  BobguiAccessible *__a = BOBGUI_ACCESSIBLE (accessible); \
  BobguiAccessibleProperty __p = (property); \
  char *value__ = bobgui_test_accessible_check_property (__a, __p, __VA_ARGS__); \
  if (value__ == NULL) ; else { \
    char *msg__ = g_strdup_printf ("assertion failed: (" #accessible ".accessible-property(" #property ") == " # __VA_ARGS__ "): value = '%s'", value__); \
    g_assertion_message (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, msg__); \
    g_free (msg__); \
  } \
} G_STMT_END

/**
 * bobgui_test_accessible_assert_relation:
 * @accessible: a `BobguiAccessible`
 * @relation: a `BobguiAccessibleRelation`
 * @...: the expected value of @relation
 *
 * Checks whether a `BobguiAccessible` implementation has its accessible
 * relation set to the expected value, and raises an assertion if the
 * condition is not satisfied.
 */
#define bobgui_test_accessible_assert_relation(accessible,relation,...) \
G_STMT_START { \
  BobguiAccessible *__a = BOBGUI_ACCESSIBLE (accessible); \
  BobguiAccessibleRelation __r = (relation); \
  char *value__ = bobgui_test_accessible_check_relation (__a, __r, __VA_ARGS__); \
  if (value__ == NULL); else { \
    char *msg__ = g_strdup_printf ("assertion failed: (" #accessible ".accessible-relation(" #relation ") == " # __VA_ARGS__ "): value = '%s'", value__); \
    g_assertion_message (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, msg__); \
    g_free (msg__); \
  } \
} G_STMT_END

/**
 * bobgui_test_accessible_assert_state:
 * @accessible: a `BobguiAccessible`
 * @state: a `BobguiAccessibleRelation`
 * @...: the expected value of @state
 *
 * Checks whether a `BobguiAccessible` implementation has its accessible
 * state set to the expected value, and raises an assertion if the
 * condition is not satisfied.
 */
#define bobgui_test_accessible_assert_state(accessible,state,...) \
G_STMT_START { \
  BobguiAccessible *__a = BOBGUI_ACCESSIBLE (accessible); \
  BobguiAccessibleState __s = (state); \
  char *value__ = bobgui_test_accessible_check_state (__a, __s, __VA_ARGS__); \
  if (value__ == NULL); else { \
    char *msg__ = g_strdup_printf ("assertion failed: (" #accessible ".accessible-state(" #state ") == " # __VA_ARGS__ "): value = '%s'", value__); \
    g_assertion_message (G_LOG_DOMAIN, __FILE__, __LINE__, G_STRFUNC, msg__); \
    g_free (msg__); \
  } \
} G_STMT_END

GDK_AVAILABLE_IN_ALL
gboolean        bobgui_test_accessible_has_role            (BobguiAccessible         *accessible,
                                                         BobguiAccessibleRole      role);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_test_accessible_has_property        (BobguiAccessible         *accessible,
                                                         BobguiAccessibleProperty  property);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_test_accessible_has_relation        (BobguiAccessible         *accessible,
                                                         BobguiAccessibleRelation  relation);
GDK_AVAILABLE_IN_ALL
gboolean        bobgui_test_accessible_has_state           (BobguiAccessible         *accessible,
                                                         BobguiAccessibleState     state);

GDK_AVAILABLE_IN_ALL
char *          bobgui_test_accessible_check_property      (BobguiAccessible         *accessible,
                                                         BobguiAccessibleProperty  property,
                                                         ...);
GDK_AVAILABLE_IN_ALL
char *          bobgui_test_accessible_check_relation      (BobguiAccessible         *accessible,
                                                         BobguiAccessibleRelation  relation,
                                                         ...);
GDK_AVAILABLE_IN_ALL
char *          bobgui_test_accessible_check_state         (BobguiAccessible         *accessible,
                                                         BobguiAccessibleState     state,
                                                         ...);

GDK_AVAILABLE_IN_ALL
void    bobgui_test_accessible_assertion_message_role      (const char        *domain,
                                                         const char        *file,
                                                         int                line,
                                                         const char        *func,
                                                         const char        *expr,
                                                         BobguiAccessible     *accessible,
                                                         BobguiAccessibleRole  expected_role,
                                                         BobguiAccessibleRole  actual_role);

G_END_DECLS
