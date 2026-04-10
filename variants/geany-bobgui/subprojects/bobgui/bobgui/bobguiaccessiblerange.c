/* bobguiaccessiblerange.c: Accessible range interface
 *
 * SPDX-FileCopyrightText: 2022 Red Hat Inc.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

/**
 * BobguiAccessibleRange:
 *
 * An interface for accessible objects containing a numeric value.
 *
 * `BobguiAccessibleRange` describes ranged controls for Assistive Technologies.
 *
 * Ranged controls have a single value within an allowed range that can
 * optionally be changed by the user.
 *
 * This interface is expected to be implemented by controls using the
 * following roles:
 *
 * - `BOBGUI_ACCESSIBLE_ROLE_METER`
 * - `BOBGUI_ACCESSIBLE_ROLE_PROGRESS_BAR`
 * - `BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR`
 * - `BOBGUI_ACCESSIBLE_ROLE_SLIDER`
 * - `BOBGUI_ACCESSIBLE_ROLE_SPIN_BUTTON`
 *
 * If that is not the case, a warning will be issued at run time.
 *
 * In addition to this interface, its implementers are expected to provide the
 * correct values for the following properties:
 *
 * - `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX`
 * - `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN`
 * - `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW`
 * - `BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT`
 *
 * Since: 4.10
 */

#include "config.h"

#include "bobguiaccessiblerangeprivate.h"

#include "bobguiaccessibleprivate.h"
#include "bobguiatcontextprivate.h"
#include "bobguiaccessiblevalueprivate.h"

G_DEFINE_INTERFACE (BobguiAccessibleRange, bobgui_accessible_range, BOBGUI_TYPE_ACCESSIBLE)

static gboolean
bobgui_accessible_range_default_set_current_value (BobguiAccessibleRange *accessible_range,
                                                double              value)
{
  return TRUE;
}

static void
bobgui_accessible_range_default_init (BobguiAccessibleRangeInterface *iface)
{
  iface->set_current_value = bobgui_accessible_range_default_set_current_value;
}

/*< private >
 * bobgui_accessible_range_set_current_value:
 * @self: a `BobguiAccessibleRange`
 *
 * Sets the current value of this `BobguiAccessibleRange` to the given value
 *
 * Note that for some widgets implementing this interface, setting a value
 * through the accessibility API makes no sense, so calling this function
 * may in some cases do nothing
 *
 * Returns: true if the call changed the value, and false otherwise
 */
gboolean
bobgui_accessible_range_set_current_value (BobguiAccessibleRange *self, double value)
{
  g_return_val_if_fail (BOBGUI_IS_ACCESSIBLE_RANGE (self), FALSE);

  BobguiAccessibleRangeInterface *iface = BOBGUI_ACCESSIBLE_RANGE_GET_IFACE (self);

  return iface->set_current_value (self, value);
}
