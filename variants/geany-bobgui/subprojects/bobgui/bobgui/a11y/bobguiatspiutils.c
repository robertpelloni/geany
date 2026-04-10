/* bobguiatspiutils.c: Shared utilities for ATSPI
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

#include "bobguiatspiutilsprivate.h"

#include "bobguienums.h"
#include "bobguiscrolledwindow.h"

/*< private >
 * bobgui_accessible_role_to_atspi_role:
 * @role: a `BobguiAccessibleRole`
 *
 * Converts a `BobguiAccessibleRole` value to the equivalent ATSPI role.
 *
 * Returns: an #AtspiRole
 */
static AtspiRole
bobgui_accessible_role_to_atspi_role (BobguiAccessibleRole role)
{
  switch (role)
    {
    case BOBGUI_ACCESSIBLE_ROLE_ALERT:
      return ATSPI_ROLE_ALERT;

    case BOBGUI_ACCESSIBLE_ROLE_ALERT_DIALOG:
      return ATSPI_ROLE_ALERT;

    case BOBGUI_ACCESSIBLE_ROLE_APPLICATION:
      return ATSPI_ROLE_FRAME;

    case BOBGUI_ACCESSIBLE_ROLE_ARTICLE:
      return ATSPI_ROLE_ARTICLE;

    case BOBGUI_ACCESSIBLE_ROLE_BANNER:
      break;

    case BOBGUI_ACCESSIBLE_ROLE_BLOCK_QUOTE:
      return ATSPI_ROLE_BLOCK_QUOTE;

    case BOBGUI_ACCESSIBLE_ROLE_BUTTON:
      return ATSPI_ROLE_PUSH_BUTTON;

    case BOBGUI_ACCESSIBLE_ROLE_CAPTION:
      return ATSPI_ROLE_CAPTION;

    case BOBGUI_ACCESSIBLE_ROLE_CELL:
      return ATSPI_ROLE_TABLE_CELL;

    case BOBGUI_ACCESSIBLE_ROLE_CHECKBOX:
      return ATSPI_ROLE_CHECK_BOX;

    case BOBGUI_ACCESSIBLE_ROLE_COLUMN_HEADER:
      break;

    case BOBGUI_ACCESSIBLE_ROLE_COMBO_BOX:
      return ATSPI_ROLE_COMBO_BOX;

    case BOBGUI_ACCESSIBLE_ROLE_COMMAND:
      break;

    case BOBGUI_ACCESSIBLE_ROLE_COMMENT:
      return ATSPI_ROLE_COMMENT;

    case BOBGUI_ACCESSIBLE_ROLE_COMPOSITE:
      break;

    case BOBGUI_ACCESSIBLE_ROLE_DIALOG:
      return ATSPI_ROLE_DIALOG;

    case BOBGUI_ACCESSIBLE_ROLE_DOCUMENT:
      return ATSPI_ROLE_DOCUMENT_TEXT;

    case BOBGUI_ACCESSIBLE_ROLE_FEED:
      break;

    case BOBGUI_ACCESSIBLE_ROLE_FORM:
      return ATSPI_ROLE_FORM;

    case BOBGUI_ACCESSIBLE_ROLE_GENERIC:
      return ATSPI_ROLE_PANEL;

    case BOBGUI_ACCESSIBLE_ROLE_GRID:
      return ATSPI_ROLE_TABLE;

    case BOBGUI_ACCESSIBLE_ROLE_GRID_CELL:
      return ATSPI_ROLE_TABLE_CELL;

    case BOBGUI_ACCESSIBLE_ROLE_GROUP:
      return ATSPI_ROLE_GROUPING;

    case BOBGUI_ACCESSIBLE_ROLE_HEADING:
      return ATSPI_ROLE_HEADING;

    case BOBGUI_ACCESSIBLE_ROLE_IMG:
      return ATSPI_ROLE_IMAGE;

    case BOBGUI_ACCESSIBLE_ROLE_INPUT:
      return ATSPI_ROLE_ENTRY;

    case BOBGUI_ACCESSIBLE_ROLE_LABEL:
      return ATSPI_ROLE_LABEL;

    case BOBGUI_ACCESSIBLE_ROLE_LANDMARK:
      return ATSPI_ROLE_LANDMARK;

    case BOBGUI_ACCESSIBLE_ROLE_LEGEND:
      return ATSPI_ROLE_LABEL;

    case BOBGUI_ACCESSIBLE_ROLE_LINK:
      return ATSPI_ROLE_LINK;

    case BOBGUI_ACCESSIBLE_ROLE_LIST:
      return ATSPI_ROLE_LIST;

    case BOBGUI_ACCESSIBLE_ROLE_LIST_BOX:
      return ATSPI_ROLE_LIST_BOX;

    case BOBGUI_ACCESSIBLE_ROLE_LIST_ITEM:
      return ATSPI_ROLE_LIST_ITEM;

    case BOBGUI_ACCESSIBLE_ROLE_LOG:
      return ATSPI_ROLE_LOG;

    case BOBGUI_ACCESSIBLE_ROLE_MAIN:
      break;

    case BOBGUI_ACCESSIBLE_ROLE_MARQUEE:
      return ATSPI_ROLE_MARQUEE;

    case BOBGUI_ACCESSIBLE_ROLE_MATH:
      return ATSPI_ROLE_MATH;

    case BOBGUI_ACCESSIBLE_ROLE_METER:
      return ATSPI_ROLE_LEVEL_BAR;

    case BOBGUI_ACCESSIBLE_ROLE_MENU:
      return ATSPI_ROLE_MENU;

    case BOBGUI_ACCESSIBLE_ROLE_MENU_BAR:
      return ATSPI_ROLE_MENU_BAR;

    case BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM:
      return ATSPI_ROLE_MENU_ITEM;

    case BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_CHECKBOX:
      return ATSPI_ROLE_CHECK_MENU_ITEM;

    case BOBGUI_ACCESSIBLE_ROLE_MENU_ITEM_RADIO:
      return ATSPI_ROLE_RADIO_MENU_ITEM;

    case BOBGUI_ACCESSIBLE_ROLE_NAVIGATION:
      return ATSPI_ROLE_FILLER;

    case BOBGUI_ACCESSIBLE_ROLE_NONE:
      return ATSPI_ROLE_INVALID;

    case BOBGUI_ACCESSIBLE_ROLE_NOTE:
      return ATSPI_ROLE_FOOTNOTE;

    case BOBGUI_ACCESSIBLE_ROLE_OPTION:
      return ATSPI_ROLE_OPTION_PANE;

    case BOBGUI_ACCESSIBLE_ROLE_PARAGRAPH:
      return ATSPI_ROLE_PARAGRAPH;

    case BOBGUI_ACCESSIBLE_ROLE_PRESENTATION:
      return ATSPI_ROLE_INVALID;

    case BOBGUI_ACCESSIBLE_ROLE_PROGRESS_BAR:
      return ATSPI_ROLE_PROGRESS_BAR;

    case BOBGUI_ACCESSIBLE_ROLE_RADIO:
      return ATSPI_ROLE_RADIO_BUTTON;

    case BOBGUI_ACCESSIBLE_ROLE_RADIO_GROUP:
      return ATSPI_ROLE_GROUPING;

    case BOBGUI_ACCESSIBLE_ROLE_RANGE:
      break;

    case BOBGUI_ACCESSIBLE_ROLE_REGION:
      return ATSPI_ROLE_FILLER;

    case BOBGUI_ACCESSIBLE_ROLE_ROW:
      return ATSPI_ROLE_TABLE_ROW;

    case BOBGUI_ACCESSIBLE_ROLE_ROW_GROUP:
      return ATSPI_ROLE_GROUPING;

    case BOBGUI_ACCESSIBLE_ROLE_ROW_HEADER:
      return ATSPI_ROLE_ROW_HEADER;

    case BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR:
      return ATSPI_ROLE_SCROLL_BAR;

    case BOBGUI_ACCESSIBLE_ROLE_SEARCH:
      return ATSPI_ROLE_FORM;

    case BOBGUI_ACCESSIBLE_ROLE_SEARCH_BOX:
      return ATSPI_ROLE_ENTRY;

    case BOBGUI_ACCESSIBLE_ROLE_SECTION:
      return ATSPI_ROLE_SECTION;

    case BOBGUI_ACCESSIBLE_ROLE_SECTION_HEAD:
      return ATSPI_ROLE_FILLER;

    case BOBGUI_ACCESSIBLE_ROLE_SELECT:
      return ATSPI_ROLE_FILLER;

    case BOBGUI_ACCESSIBLE_ROLE_SEPARATOR:
      return ATSPI_ROLE_SEPARATOR;

    case BOBGUI_ACCESSIBLE_ROLE_SLIDER:
      return ATSPI_ROLE_SLIDER;

    case BOBGUI_ACCESSIBLE_ROLE_SPIN_BUTTON:
      return ATSPI_ROLE_SPIN_BUTTON;

    case BOBGUI_ACCESSIBLE_ROLE_STATUS:
      return ATSPI_ROLE_STATUS_BAR;

    case BOBGUI_ACCESSIBLE_ROLE_STRUCTURE:
      return ATSPI_ROLE_FILLER;

    case BOBGUI_ACCESSIBLE_ROLE_SWITCH:
      return ATSPI_ROLE_SWITCH;

    case BOBGUI_ACCESSIBLE_ROLE_TAB:
      return ATSPI_ROLE_PAGE_TAB;

    case BOBGUI_ACCESSIBLE_ROLE_TABLE:
      return ATSPI_ROLE_TABLE;

    case BOBGUI_ACCESSIBLE_ROLE_TAB_LIST:
      return ATSPI_ROLE_PAGE_TAB_LIST;

    case BOBGUI_ACCESSIBLE_ROLE_TAB_PANEL:
      return ATSPI_ROLE_PANEL;

    case BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX:
      return ATSPI_ROLE_TEXT;

    case BOBGUI_ACCESSIBLE_ROLE_TIME:
      return ATSPI_ROLE_TEXT;

    case BOBGUI_ACCESSIBLE_ROLE_TIMER:
      return ATSPI_ROLE_TIMER;

    case BOBGUI_ACCESSIBLE_ROLE_TOOLBAR:
      return ATSPI_ROLE_TOOL_BAR;

    case BOBGUI_ACCESSIBLE_ROLE_TOOLTIP:
      return ATSPI_ROLE_TOOL_TIP;

    case BOBGUI_ACCESSIBLE_ROLE_TREE:
      return ATSPI_ROLE_TREE;

    case BOBGUI_ACCESSIBLE_ROLE_TREE_GRID:
      return ATSPI_ROLE_TREE_TABLE;

    case BOBGUI_ACCESSIBLE_ROLE_TREE_ITEM:
      return ATSPI_ROLE_TREE_ITEM;

    case BOBGUI_ACCESSIBLE_ROLE_WIDGET:
      return ATSPI_ROLE_FILLER;

    case BOBGUI_ACCESSIBLE_ROLE_WINDOW:
      return ATSPI_ROLE_FRAME;

    case BOBGUI_ACCESSIBLE_ROLE_TOGGLE_BUTTON:
      return ATSPI_ROLE_TOGGLE_BUTTON;

    case BOBGUI_ACCESSIBLE_ROLE_TERMINAL:
      return ATSPI_ROLE_TERMINAL;

    default:
      break;
    }

  return ATSPI_ROLE_FILLER;
}

/*<private>
 * bobgui_atspi_role_for_context:
 * @context: a `BobguiATContext`
 *
 * Returns a suitable ATSPI role for a context, taking into account
 * both the `BobguiAccessibleRole` set on the context and the type
 * of accessible.
 *
 * Returns: an #AtspiRole
 */
AtspiRole
bobgui_atspi_role_for_context (BobguiATContext *context)
{
  BobguiAccessible *accessible = bobgui_at_context_get_accessible (context);
  BobguiAccessibleRole role = bobgui_at_context_get_accessible_role (context);

  /* ARIA does not have a password entry role, so use the input purpose to distinguish them */
  if (role == BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX)
    {
      if (bobgui_accessible_is_password_text (accessible))
        return ATSPI_ROLE_PASSWORD_TEXT;
    }
  /* ARIA does not have a "scroll area" role */
  if (BOBGUI_IS_SCROLLED_WINDOW (accessible))
    return ATSPI_ROLE_SCROLL_PANE;

  return bobgui_accessible_role_to_atspi_role (role);
}

GVariant *
bobgui_at_spi_null_ref (void)
{
  return g_variant_new ("(so)", "", "/org/a11y/atspi/null");
}

void
bobgui_at_spi_emit_children_changed (GDBusConnection         *connection,
                                  const char              *path,
                                  BobguiAccessibleChildState  state,
                                  int                      idx,
                                  GVariant                *child_ref)
{
  const char *change;

  switch (state)
    {
    case BOBGUI_ACCESSIBLE_CHILD_STATE_ADDED:
      change = "add";
      break;

    case BOBGUI_ACCESSIBLE_CHILD_STATE_REMOVED:
      change = "remove";
      break;

    default:
      g_assert_not_reached ();
      return;
    }

  g_dbus_connection_emit_signal (connection,
                                 NULL,
                                 path,
                                 "org.a11y.atspi.Event.Object",
                                 "ChildrenChanged",
                                 g_variant_new ("(siiva{sv})", change, idx, 0, child_ref, NULL),
                                 NULL);
}


void
bobgui_at_spi_translate_coordinates_to_accessible (BobguiAccessible  *accessible,
                                                AtspiCoordType  coordtype,
                                                int             xi,
                                                int             yi,
                                                int            *xo,
                                                int            *yo)
{
  BobguiAccessible *parent;
  int x, y, width, height;

  if (coordtype == ATSPI_COORD_TYPE_SCREEN)
    {
      *xo = 0;
      *yo = 0;
      return;
    }

  if (!bobgui_accessible_get_bounds (accessible, &x, &y, &width, &height))
    {
      *xo = xi;
      *yo = yi;
      return;
    }

  /* Transform coords to our parent, we will need that in any case */
  *xo = xi - x;
  *yo = yi - y;

  /* If that's what the caller requested, we're done */
  if (coordtype == ATSPI_COORD_TYPE_PARENT)
    return;

  if (coordtype == ATSPI_COORD_TYPE_WINDOW)
    {
      parent = bobgui_accessible_get_accessible_parent (accessible);
      while (parent != NULL)
        {
          g_object_unref (parent);

          if (bobgui_accessible_get_bounds (parent, &x, &y, &width, &height))
            {
              *xo = *xo - x;
              *yo = *yo - y;
              parent = bobgui_accessible_get_accessible_parent (parent);
            }
          else
            break;
        }
    }
  else
    g_assert_not_reached ();
}

void
bobgui_at_spi_translate_coordinates_from_accessible (BobguiAccessible *accessible,
                                                  AtspiCoordType     coordtype,
                                                  int                xi,
                                                  int                yi,
                                                  int               *xo,
                                                  int               *yo)
{
  BobguiAccessible *parent;
  int x, y, width, height;

  if (coordtype == ATSPI_COORD_TYPE_SCREEN)
    {
      *xo = 0;
      *yo = 0;
      return;
    }

  if (!bobgui_accessible_get_bounds (accessible, &x, &y, &width, &height))
    {
      *xo = xi;
      *yo = yi;
      return;
    }

  /* Transform coords to our parent, we will need that in any case */
  *xo = xi + x;
  *yo = yi + y;

  /* If that's what the caller requested, we're done */
  if (coordtype == ATSPI_COORD_TYPE_PARENT)
    return;

  if (coordtype == ATSPI_COORD_TYPE_WINDOW)
    {
      parent = bobgui_accessible_get_accessible_parent (accessible);
      while (parent != NULL)
        {
          g_object_unref (parent);

          if (bobgui_accessible_get_bounds (parent, &x, &y, &width, &height))
            {
              *xo = *xo + x;
              *yo = *yo + y;
              parent = bobgui_accessible_get_accessible_parent (parent);
            }
          else
            break;
        }
    }
  else
    g_assert_not_reached ();
}
