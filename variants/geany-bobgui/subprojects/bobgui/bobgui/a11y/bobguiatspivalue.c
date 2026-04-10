/* bobguiatspivalue.c: AT-SPI Value implementation
 *
 * Copyright 2020 Red Hat, Inc.
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

#include "bobguiatspivalueprivate.h"

#include "a11y/atspi/atspi-value.h"

#include "bobguiaccessiblerangeprivate.h"
#include "bobguiatcontextprivate.h"
#include "bobguidebug.h"

#include <gio/gio.h>

static GVariant *
handle_value_get_property (GDBusConnection  *connection,
                           const gchar      *sender,
                           const gchar      *object_path,
                           const gchar      *interface_name,
                           const gchar      *property_name,
                           GError          **error,
                           gpointer          user_data)
{
  BobguiATContext *ctx = BOBGUI_AT_CONTEXT (user_data);

  /* Numeric attributes */
  struct {
    const char *name;
    BobguiAccessibleProperty property;
  } num_properties[] = {
    { "MinimumValue", BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN },
    { "MaximumValue", BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX },
    { "CurrentValue", BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW },
  };

  /* String attributes */
  struct {
    const char *name;
    BobguiAccessibleProperty property;
  } str_properties[] = {
    { "Text", BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT },
  };

  for (int i = 0; i < G_N_ELEMENTS (num_properties); i++)
    {
      if (g_strcmp0 (property_name,  num_properties[i].name) == 0)
        {
          if (bobgui_at_context_has_accessible_property (ctx, num_properties[i].property))
            {
              BobguiAccessibleValue *value =
                bobgui_at_context_get_accessible_property (ctx, num_properties[i].property);

              return g_variant_new_double (bobgui_number_accessible_value_get (value));
            }
        }
    }

  for (int i = 0; i < G_N_ELEMENTS (str_properties); i++)
    {
      if (g_strcmp0 (property_name, str_properties[i].name) == 0)
        {
          if (bobgui_at_context_has_accessible_property (ctx, str_properties[i].property))
            {
              BobguiAccessibleValue *value =
                bobgui_at_context_get_accessible_property (ctx, str_properties[i].property);

              return g_variant_new_string (bobgui_string_accessible_value_get (value));
            }
        }
    }

  /* Special-case Text, as it's not a double */
  if (g_strcmp0 (property_name, "Text") == 0)
    return g_variant_new_string ("");

  /* Special-case MinimumIncrement as it does not have an ARIA counterpart */
  if (g_strcmp0 (property_name, "MinimumIncrement") == 0)
    return g_variant_new_double (0.0);

  /* fall back for widgets that should have the
   * properties but don't
   */
  return g_variant_new_double (0.0);
}

static gboolean
handle_value_set_property (GDBusConnection  *connection,
                           const gchar      *sender,
                           const gchar      *object_path,
                           const gchar      *interface_name,
                           const gchar      *property_name,
                           GVariant         *value,
                           GError          **error,
                           gpointer          user_data)
{
  BobguiATContext *self = user_data;
  BobguiAccessibleRange *range = BOBGUI_ACCESSIBLE_RANGE (bobgui_at_context_get_accessible (self));

  if (g_strcmp0 (property_name, "CurrentValue") == 0)
    return bobgui_accessible_range_set_current_value (range, g_variant_get_double (value));

  return FALSE;
}

static const GDBusInterfaceVTable value_vtable = {
  NULL,
  handle_value_get_property,
  handle_value_set_property,
};

const GDBusInterfaceVTable *
bobgui_atspi_get_value_vtable (BobguiAccessible *accessible)
{
  if (BOBGUI_IS_ACCESSIBLE_RANGE (accessible))
    return &value_vtable;

  return NULL;
}
