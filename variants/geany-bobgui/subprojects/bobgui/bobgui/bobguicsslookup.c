/* BOBGUI - The Bobgui Framework
 * Copyright (C) 2011 Benjamin Otte <otte@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "bobguicsslookupprivate.h"

#include "bobguicsscustompropertypoolprivate.h"
#include "bobguicssstylepropertyprivate.h"
#include "bobguicsstypesprivate.h"
#include "bobguiprivatetypebuiltins.h"
#include "bobguiprivate.h"

void
_bobgui_css_lookup_init (BobguiCssLookup     *lookup)
{
  memset (lookup, 0, sizeof (*lookup));

  lookup->set_values = _bobgui_bitmask_new ();
}

void
_bobgui_css_lookup_destroy (BobguiCssLookup *lookup)
{
  _bobgui_bitmask_free (lookup->set_values);

  if (lookup->custom_values)
    g_hash_table_unref (lookup->custom_values);
}

gboolean
_bobgui_css_lookup_is_missing (const BobguiCssLookup *lookup,
                            guint               id)
{
  bobgui_internal_return_val_if_fail (lookup != NULL, FALSE);

  return !_bobgui_bitmask_get (lookup->set_values, id);
}

/**
 * _bobgui_css_lookup_set:
 * @lookup: the lookup
 * @id: id of the property to set, see _bobgui_style_property_get_id()
 * @section: (nullable): The @section the value was defined in
 * @value: the “cascading value” to use
 *
 * Sets the @value for a given @id. No value may have been set for @id
 * before. See _bobgui_css_lookup_is_missing(). This function is used to
 * set the “winning declaration” of a lookup. Note that for performance
 * reasons @value and @section are not copied. It is your responsibility
 * to ensure they are kept alive until _bobgui_css_lookup_free() is called.
 **/
void
_bobgui_css_lookup_set (BobguiCssLookup  *lookup,
                     guint          id,
                     BobguiCssSection *section,
                     BobguiCssValue   *value)
{
  bobgui_internal_return_if_fail (lookup != NULL);
  bobgui_internal_return_if_fail (value != NULL);
  bobgui_internal_return_if_fail (lookup->values[id].value == NULL);

  lookup->values[id].value = value;
  lookup->values[id].section = section;
  lookup->set_values = _bobgui_bitmask_set (lookup->set_values, id, TRUE);
}

void
_bobgui_css_lookup_set_custom (BobguiCssLookup        *lookup,
                            int                  id,
                            BobguiCssVariableValue *value)
{
  bobgui_internal_return_if_fail (lookup != NULL);

  if (!lookup->custom_values)
    lookup->custom_values = g_hash_table_new (g_direct_hash, g_direct_equal);

  if (!g_hash_table_contains (lookup->custom_values, GINT_TO_POINTER (id)))
    g_hash_table_replace (lookup->custom_values, GINT_TO_POINTER (id), value);
}
