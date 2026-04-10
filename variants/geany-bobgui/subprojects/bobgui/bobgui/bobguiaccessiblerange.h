/* bobguiaccessiblerange.h: Accessible range interface
 *
 * SPDX-FileCopyrightText: 2022 Red Hat Inc.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiaccessible.h>

G_BEGIN_DECLS
#define BOBGUI_TYPE_ACCESSIBLE_RANGE (bobgui_accessible_range_get_type())

GDK_AVAILABLE_IN_4_10
G_DECLARE_INTERFACE (BobguiAccessibleRange, bobgui_accessible_range, BOBGUI, ACCESSIBLE_RANGE, BobguiAccessible)

struct _BobguiAccessibleRangeInterface
{
  GTypeInterface g_iface;

  /**
   * BobguiAccessibleRangeInterface::set_current_value:
   * @self: a `BobguiAccessibleRange`
   * @value: the value to set
   *
   * Sets the current value of the accessible range.
   *
   * This operation should behave similarly as if the user performed the
   * action.
   *
   * Returns: true if the operation was performed, false otherwise
   *
   * Since: 4.10
   */
  gboolean (* set_current_value) (BobguiAccessibleRange *self,
                                  double              value);
};

G_END_DECLS
