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

#include "bobguicsstypesprivate.h"

#include "bobguicssnumbervalueprivate.h"

void
bobgui_css_change_print (BobguiCssChange  change,
                      GString      *string)
{
  const struct {
    BobguiCssChange flags;
    const char *name;
  } names[] = {
    { BOBGUI_CSS_CHANGE_CLASS, "class" },
    { BOBGUI_CSS_CHANGE_NAME, "name" },
    { BOBGUI_CSS_CHANGE_ID, "id" },
    { BOBGUI_CSS_CHANGE_FIRST_CHILD, "first-child" },
    { BOBGUI_CSS_CHANGE_LAST_CHILD, "last-child" },
    { BOBGUI_CSS_CHANGE_NTH_CHILD, "nth-child" },
    { BOBGUI_CSS_CHANGE_NTH_LAST_CHILD, "nth-last-child" },
    { BOBGUI_CSS_CHANGE_STATE, "state" },
    { BOBGUI_CSS_CHANGE_HOVER, "hover" },
    { BOBGUI_CSS_CHANGE_DISABLED, "disabled" },
    { BOBGUI_CSS_CHANGE_BACKDROP, "backdrop" },
    { BOBGUI_CSS_CHANGE_SELECTED, "selected" },

    { BOBGUI_CSS_CHANGE_SIBLING_CLASS, "sibling-class" },
    { BOBGUI_CSS_CHANGE_SIBLING_NAME, "sibling-name" },
    { BOBGUI_CSS_CHANGE_SIBLING_ID, "sibling-id" },
    { BOBGUI_CSS_CHANGE_SIBLING_FIRST_CHILD, "sibling-first-child" },
    { BOBGUI_CSS_CHANGE_SIBLING_LAST_CHILD, "sibling-last-child" },
    { BOBGUI_CSS_CHANGE_SIBLING_NTH_CHILD, "sibling-nth-child" },
    { BOBGUI_CSS_CHANGE_SIBLING_NTH_LAST_CHILD, "sibling-nth-last-child" },
    { BOBGUI_CSS_CHANGE_SIBLING_STATE, "sibling-state" },
    { BOBGUI_CSS_CHANGE_SIBLING_HOVER, "sibling-hover" },
    { BOBGUI_CSS_CHANGE_SIBLING_DISABLED, "sibling-disabled" },
    { BOBGUI_CSS_CHANGE_SIBLING_BACKDROP, "sibling-backdrop" },
    { BOBGUI_CSS_CHANGE_SIBLING_SELECTED, "sibling-selected" },

    { BOBGUI_CSS_CHANGE_PARENT_CLASS, "parent-class" },
    { BOBGUI_CSS_CHANGE_PARENT_NAME, "parent-name" },
    { BOBGUI_CSS_CHANGE_PARENT_ID, "parent-id" },
    { BOBGUI_CSS_CHANGE_PARENT_FIRST_CHILD, "parent-first-child" },
    { BOBGUI_CSS_CHANGE_PARENT_LAST_CHILD, "parent-last-child" },
    { BOBGUI_CSS_CHANGE_PARENT_NTH_CHILD, "parent-nth-child" },
    { BOBGUI_CSS_CHANGE_PARENT_NTH_LAST_CHILD, "parent-nth-last-child" },
    { BOBGUI_CSS_CHANGE_PARENT_STATE, "parent-state" },
    { BOBGUI_CSS_CHANGE_PARENT_HOVER, "parent-hover" },
    { BOBGUI_CSS_CHANGE_PARENT_DISABLED, "parent-disabled" },
    { BOBGUI_CSS_CHANGE_PARENT_BACKDROP, "parent-backdrop" },
    { BOBGUI_CSS_CHANGE_PARENT_SELECTED, "parent-selected" },

    { BOBGUI_CSS_CHANGE_PARENT_SIBLING_CLASS, "parent-sibling-class" },
    { BOBGUI_CSS_CHANGE_PARENT_SIBLING_NAME, "parent-sibling-name" },
    { BOBGUI_CSS_CHANGE_PARENT_SIBLING_ID, "parent-sibling-id" },
    { BOBGUI_CSS_CHANGE_PARENT_SIBLING_FIRST_CHILD, "parent-sibling-first-child" },
    { BOBGUI_CSS_CHANGE_PARENT_SIBLING_LAST_CHILD, "parent-sibling-last-child" },
    { BOBGUI_CSS_CHANGE_PARENT_SIBLING_NTH_CHILD, "parent-sibling-nth-child" },
    { BOBGUI_CSS_CHANGE_PARENT_SIBLING_NTH_LAST_CHILD, "parent-sibling-nth-last-child" },
    { BOBGUI_CSS_CHANGE_PARENT_SIBLING_STATE, "parent-sibling-state" },
    { BOBGUI_CSS_CHANGE_PARENT_SIBLING_HOVER, "parent-sibling-hover" },
    { BOBGUI_CSS_CHANGE_PARENT_SIBLING_DISABLED, "parent-sibling-disabled" },
    { BOBGUI_CSS_CHANGE_PARENT_SIBLING_BACKDROP, "parent-sibling-backdrop" },
    { BOBGUI_CSS_CHANGE_PARENT_SIBLING_SELECTED, "parent-sibling-selected" },

    { BOBGUI_CSS_CHANGE_SOURCE, "source" },
    { BOBGUI_CSS_CHANGE_PARENT_STYLE, "parent-style" },
    { BOBGUI_CSS_CHANGE_TIMESTAMP, "timestamp" },
    { BOBGUI_CSS_CHANGE_ANIMATIONS, "animations" },
  };
  guint i;
  gboolean first;

  first = TRUE;

  for (i = 0; i < G_N_ELEMENTS (names); i++)
    {
      if (change & names[i].flags)
        {
          if (first)
            first = FALSE;
          else
            g_string_append (string, "|");
          g_string_append (string, names[i].name);
        }
    }
}

BobguiCssDimension
bobgui_css_unit_get_dimension (BobguiCssUnit unit)
{
  switch (unit)
    {
    case BOBGUI_CSS_NUMBER:
      return BOBGUI_CSS_DIMENSION_NUMBER;

    case BOBGUI_CSS_PERCENT:
      return BOBGUI_CSS_DIMENSION_PERCENTAGE;

    case BOBGUI_CSS_PX:
    case BOBGUI_CSS_PT:
    case BOBGUI_CSS_EM:
    case BOBGUI_CSS_EX:
    case BOBGUI_CSS_REM:
    case BOBGUI_CSS_PC:
    case BOBGUI_CSS_IN:
    case BOBGUI_CSS_CM:
    case BOBGUI_CSS_MM:
      return BOBGUI_CSS_DIMENSION_LENGTH;

    case BOBGUI_CSS_RAD:
    case BOBGUI_CSS_DEG:
    case BOBGUI_CSS_GRAD:
    case BOBGUI_CSS_TURN:
      return BOBGUI_CSS_DIMENSION_ANGLE;

    case BOBGUI_CSS_S:
    case BOBGUI_CSS_MS:
      return BOBGUI_CSS_DIMENSION_TIME;

    default:
      g_assert_not_reached ();
      return BOBGUI_CSS_DIMENSION_PERCENTAGE;
    }
}

char *
bobgui_css_change_to_string (BobguiCssChange change)
{
  GString *string = g_string_new (NULL);

  bobgui_css_change_print (change, string);

  return g_string_free (string, FALSE);
}

const char *
bobgui_css_pseudoclass_name (BobguiStateFlags state)
{
  static const char * state_names[] = {
    "active",
    "hover",
    "selected",
    "disabled",
    "indeterminate",
    "focus",
    "backdrop",
    "dir(ltr)",
    "dir(rtl)",
    "link",
    "visited",
    "checked",
    "drop(active)",
    "focus-visible",
    "focus-within"
  };
  guint i;

  for (i = 0; i < G_N_ELEMENTS (state_names); i++)
    {
      if (state == (1 << i))
        return state_names[i];
    }

  return NULL;
}

