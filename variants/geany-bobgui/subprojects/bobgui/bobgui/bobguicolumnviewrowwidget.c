/*
 * Copyright © 2023 Benjamin Otte
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
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Benjamin Otte <otte@gnome.org>
 */

#include "config.h"

#include "bobguicolumnviewrowwidgetprivate.h"

#include "bobguibinlayout.h"
#include "bobguicolumnviewprivate.h"
#include "bobguicolumnviewcellwidgetprivate.h"
#include "bobguicolumnviewcolumnprivate.h"
#include "bobguicolumnviewrowprivate.h"
#include "bobguicolumnviewtitleprivate.h"
#include "bobguilistitemfactoryprivate.h"
#include "bobguilistbaseprivate.h"
#include "bobguiwidget.h"
#include "bobguiwidgetprivate.h"

G_DEFINE_TYPE (BobguiColumnViewRowWidget, bobgui_column_view_row_widget, BOBGUI_TYPE_LIST_FACTORY_WIDGET)

static BobguiColumnView *
bobgui_column_view_row_widget_get_column_view (BobguiColumnViewRowWidget *self)
{
  BobguiWidget *parent = _bobgui_widget_get_parent (BOBGUI_WIDGET (self));

  if (BOBGUI_IS_COLUMN_VIEW (parent))
    return BOBGUI_COLUMN_VIEW (parent);

  parent = _bobgui_widget_get_parent (parent);
  g_assert (BOBGUI_IS_COLUMN_VIEW (parent));

  return BOBGUI_COLUMN_VIEW (parent);
}

static gboolean
bobgui_column_view_row_widget_is_header (BobguiColumnViewRowWidget *self)
{
  return bobgui_widget_get_css_name (BOBGUI_WIDGET (self)) == g_intern_static_string ("header");
}

static BobguiColumnViewColumn *
bobgui_column_view_row_child_get_column (BobguiWidget *child)
{
  if (BOBGUI_IS_COLUMN_VIEW_CELL_WIDGET (child))
    return bobgui_column_view_cell_widget_get_column (BOBGUI_COLUMN_VIEW_CELL_WIDGET (child));
  else
    return bobgui_column_view_title_get_column (BOBGUI_COLUMN_VIEW_TITLE (child));

  g_return_val_if_reached (NULL);
}

static BobguiWidget *
bobgui_column_view_row_widget_find_child (BobguiColumnViewRowWidget *self,
                                       BobguiColumnViewColumn    *column)
{
  BobguiWidget *child;

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self));
       child;
       child = bobgui_widget_get_next_sibling (child))
    {
      if (bobgui_column_view_row_child_get_column (child) == column)
        return child;
    }

  return NULL;
}

static void
bobgui_column_view_row_widget_update (BobguiListItemBase *base,
                                   guint            position,
                                   gpointer         item,
                                   gboolean         selected)
{
  BobguiColumnViewRowWidget *self = BOBGUI_COLUMN_VIEW_ROW_WIDGET (base);
  BobguiWidget *child;

  if (bobgui_column_view_row_widget_is_header (self))
    return;

  BOBGUI_LIST_ITEM_BASE_CLASS (bobgui_column_view_row_widget_parent_class)->update (base, position, item, selected);

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self));
       child;
       child = bobgui_widget_get_next_sibling (child))
    {
      bobgui_list_item_base_update (BOBGUI_LIST_ITEM_BASE (child), position, item, selected);
    }
}

static gpointer
bobgui_column_view_row_widget_create_object (BobguiListFactoryWidget *fw)
{
  return bobgui_column_view_row_new ();
}

static void
bobgui_column_view_row_widget_setup_object (BobguiListFactoryWidget *fw,
                                         gpointer              object)
{
  BobguiColumnViewRowWidget *self = BOBGUI_COLUMN_VIEW_ROW_WIDGET (fw);
  BobguiColumnViewRow *row = object;

  g_assert (!bobgui_column_view_row_widget_is_header (self));

  BOBGUI_LIST_FACTORY_WIDGET_CLASS (bobgui_column_view_row_widget_parent_class)->setup_object (fw, object);

  row->owner = self;

  bobgui_list_factory_widget_set_activatable (fw, row->activatable);
  bobgui_list_factory_widget_set_selectable (fw, row->selectable);
  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), row->focusable);

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_LABEL, row->accessible_label,
                                  BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION, row->accessible_description,
                                  -1);

  bobgui_column_view_row_do_notify (row,
                                 bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self)) != NULL,
                                 bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self)) != BOBGUI_INVALID_LIST_POSITION,
                                 bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (self)));
}

static void
bobgui_column_view_row_widget_teardown_object (BobguiListFactoryWidget *fw,
                                            gpointer              object)
{
  BobguiColumnViewRowWidget *self = BOBGUI_COLUMN_VIEW_ROW_WIDGET (fw);
  BobguiColumnViewRow *row = object;

  g_assert (!bobgui_column_view_row_widget_is_header (self));

  BOBGUI_LIST_FACTORY_WIDGET_CLASS (bobgui_column_view_row_widget_parent_class)->teardown_object (fw, object);

  row->owner = NULL;

  bobgui_list_factory_widget_set_activatable (fw, FALSE);
  bobgui_list_factory_widget_set_selectable (fw, FALSE);
  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);

  bobgui_accessible_reset_property (BOBGUI_ACCESSIBLE (self), BOBGUI_ACCESSIBLE_PROPERTY_LABEL);
  bobgui_accessible_reset_property (BOBGUI_ACCESSIBLE (self), BOBGUI_ACCESSIBLE_PROPERTY_DESCRIPTION);

  bobgui_column_view_row_do_notify (row,
                                 bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self)) != NULL,
                                 bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self)) != BOBGUI_INVALID_LIST_POSITION,
                                 bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (self)));
}

static void
bobgui_column_view_row_widget_update_object (BobguiListFactoryWidget *fw,
                                          gpointer              object,
                                          guint                 position,
                                          gpointer              item,
                                          gboolean              selected)
{
  BobguiColumnViewRowWidget *self = BOBGUI_COLUMN_VIEW_ROW_WIDGET (fw);
  BobguiListItemBase *base = BOBGUI_LIST_ITEM_BASE (self);
  BobguiColumnViewRow *row = object;
  /* Track notify manually instead of freeze/thaw_notify for performance reasons. */
  gboolean notify_item = FALSE, notify_position = FALSE, notify_selected = FALSE;

  g_assert (!bobgui_column_view_row_widget_is_header (self));

  /* FIXME: It's kinda evil to notify external objects from here... */
  notify_item = bobgui_list_item_base_get_item (base) != item;
  notify_position = bobgui_list_item_base_get_position (base) != position;
  notify_selected = bobgui_list_item_base_get_selected (base) != selected;

  BOBGUI_LIST_FACTORY_WIDGET_CLASS (bobgui_column_view_row_widget_parent_class)->update_object (fw,
                                                                                          object,
                                                                                          position,
                                                                                          item,
                                                                                          selected);

  if (row)
    bobgui_column_view_row_do_notify (row, notify_item, notify_position, notify_selected);
}

static BobguiWidget *
bobgui_column_view_next_focus_widget (BobguiWidget        *widget,
                                   BobguiWidget        *current,
                                   BobguiDirectionType  direction)
{
  gboolean forward;

  switch (direction)
    {
      case BOBGUI_DIR_TAB_FORWARD:
        forward = TRUE;
        break;
      case BOBGUI_DIR_TAB_BACKWARD:
        forward = FALSE;
        break;
      case BOBGUI_DIR_LEFT:
        forward = bobgui_widget_get_direction (widget) == BOBGUI_TEXT_DIR_RTL;
        break;
      case BOBGUI_DIR_RIGHT:
        forward = bobgui_widget_get_direction (widget) != BOBGUI_TEXT_DIR_RTL;
        break;
      case BOBGUI_DIR_UP:
      case BOBGUI_DIR_DOWN:
        return NULL;
      default:
        g_return_val_if_reached (NULL);
    }

  if (forward)
    {
      if (current == NULL)
        return widget;
      else if (current == widget)
        return bobgui_widget_get_first_child (widget);
      else
        return bobgui_widget_get_next_sibling (current);
    }
  else
    {
      if (current == NULL)
        return bobgui_widget_get_last_child (widget);
      else if (current == widget)
        return NULL;
      else
        {
          current = bobgui_widget_get_prev_sibling (current);
          if (current)
            return current;
          else
            return widget;
        }
    }
}

static gboolean
bobgui_column_view_row_widget_focus (BobguiWidget        *widget,
                                  BobguiDirectionType  direction)
{
  BobguiColumnViewRowWidget *self = BOBGUI_COLUMN_VIEW_ROW_WIDGET (widget);
  BobguiWidget *child, *current;
  BobguiColumnView *view;

  current = bobgui_widget_get_focus_child (widget);

  view = bobgui_column_view_row_widget_get_column_view (self);
  if (bobgui_column_view_get_tab_behavior (view) == BOBGUI_LIST_TAB_CELL &&
      (direction == BOBGUI_DIR_TAB_FORWARD || direction == BOBGUI_DIR_TAB_BACKWARD))
    {
      if (current || bobgui_widget_is_focus (widget))
        return FALSE;
    }

  if (current &&
      (direction == BOBGUI_DIR_UP || direction == BOBGUI_DIR_DOWN ||
      direction == BOBGUI_DIR_LEFT || direction == BOBGUI_DIR_RIGHT))
    {
      if (bobgui_widget_child_focus (current, direction))
        return TRUE;
    }

  if (current == NULL)
    {
      BobguiColumnViewColumn *focus_column = bobgui_column_view_get_focus_column (view);
      if (focus_column)
        {
          current = bobgui_column_view_row_widget_find_child (self, focus_column);
          if (current && bobgui_widget_child_focus (current, direction))
            return TRUE;
        }
    }

  if (bobgui_widget_is_focus (widget))
    current = widget;

  for (child = bobgui_column_view_next_focus_widget (widget, current, direction);
       child;
       child = bobgui_column_view_next_focus_widget (widget, child, direction))
    {
      if (child == widget)
        {
          if (bobgui_widget_grab_focus_self (widget))
            {
              bobgui_column_view_set_focus_column (view, NULL, FALSE);
              return TRUE;
            }
        }
      else if (child)
        {
          if (bobgui_widget_child_focus (child, direction))
            return TRUE;
        }
    }

  return FALSE;
}

static gboolean
bobgui_column_view_row_widget_grab_focus (BobguiWidget *widget)
{
  BobguiColumnViewRowWidget *self = BOBGUI_COLUMN_VIEW_ROW_WIDGET (widget);
  BobguiWidget *child, *focus_child;
  BobguiColumnViewColumn *focus_column;
  BobguiColumnView *view;

  view = bobgui_column_view_row_widget_get_column_view (self);
  focus_column = bobgui_column_view_get_focus_column (view);
  if (focus_column)
    {
      focus_child = bobgui_column_view_row_widget_find_child (self, focus_column);
      if (focus_child && bobgui_widget_grab_focus (focus_child))
        return TRUE;
    }
  else
    focus_child = NULL;

  if (bobgui_widget_grab_focus_self (widget))
    {
      bobgui_column_view_set_focus_column (view, NULL, FALSE);
      return TRUE;
    }

  for (child = focus_child ? bobgui_widget_get_next_sibling (focus_child) : bobgui_widget_get_first_child (widget);
       child != focus_child;
       child = child ? bobgui_widget_get_next_sibling (child) : bobgui_widget_get_first_child (widget))
    {
      /* When we started iterating at focus_child, we want to iterate over the rest
       * of the children, too */
      if (child == NULL)
        continue;

      if (bobgui_widget_grab_focus (child))
        return TRUE;
    }

  return FALSE;
}

static void
bobgui_column_view_row_widget_set_focus_child (BobguiWidget *widget,
                                            BobguiWidget *child)
{
  BobguiColumnViewRowWidget *self = BOBGUI_COLUMN_VIEW_ROW_WIDGET (widget);

  BOBGUI_WIDGET_CLASS (bobgui_column_view_row_widget_parent_class)->set_focus_child (widget, child);

  if (child)
    {
      bobgui_column_view_set_focus_column (bobgui_column_view_row_widget_get_column_view (self),
                                        bobgui_column_view_row_child_get_column (child),
                                        TRUE);
    }
}

static void
bobgui_column_view_row_widget_dispose (GObject *object)
{
  BobguiColumnViewRowWidget *self = BOBGUI_COLUMN_VIEW_ROW_WIDGET (object);
  BobguiWidget *child;

  while ((child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self))))
    {
      bobgui_column_view_row_widget_remove_child (self, child);
    }

  G_OBJECT_CLASS (bobgui_column_view_row_widget_parent_class)->dispose (object);
}

static void
bobgui_column_view_row_widget_measure_along (BobguiColumnViewRowWidget *self,
                                          int                     for_size,
                                          int                    *minimum,
                                          int                    *natural,
                                          int                    *minimum_baseline,
                                          int                    *natural_baseline)
{
  BobguiOrientation orientation = BOBGUI_ORIENTATION_VERTICAL;
  BobguiColumnView *view;
  BobguiWidget *child;
  guint i, n;
  BobguiRequestedSize *sizes = NULL;

  view = bobgui_column_view_row_widget_get_column_view (self);

  if (for_size > -1)
    {
      n = g_list_model_get_n_items (bobgui_column_view_get_columns (view));
      sizes = g_newa (BobguiRequestedSize, n);
      bobgui_column_view_distribute_width (view, for_size, sizes);
    }

  for (child = _bobgui_widget_get_first_child (BOBGUI_WIDGET (self)), i = 0;
       child != NULL;
       child = _bobgui_widget_get_next_sibling (child), i++)
    {
      int child_min = 0;
      int child_nat = 0;
      int child_min_baseline = -1;
      int child_nat_baseline = -1;

      if (!bobgui_widget_should_layout (child))
        continue;

      bobgui_widget_measure (child, orientation,
                          for_size > -1 ? sizes[i].minimum_size : -1,
                          &child_min, &child_nat,
                          &child_min_baseline, &child_nat_baseline);

      *minimum = MAX (*minimum, child_min);
      *natural = MAX (*natural, child_nat);

      if (child_min_baseline > -1)
        *minimum_baseline = MAX (*minimum_baseline, child_min_baseline);
      if (child_nat_baseline > -1)
        *natural_baseline = MAX (*natural_baseline, child_nat_baseline);
    }
}

static void
bobgui_column_view_row_widget_measure (BobguiWidget      *widget,
                                    BobguiOrientation  orientation,
                                    int             for_size,
                                    int            *minimum,
                                    int            *natural,
                                    int            *minimum_baseline,
                                    int            *natural_baseline)
{
  BobguiColumnViewRowWidget *self = BOBGUI_COLUMN_VIEW_ROW_WIDGET (widget);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      bobgui_column_view_measure_across (bobgui_column_view_row_widget_get_column_view (self),
                                      minimum,
                                      natural);
    }
  else
    {
      bobgui_column_view_row_widget_measure_along (self,
                                                for_size,
                                                minimum,
                                                natural,
                                                minimum_baseline,
                                                natural_baseline);
    }
}

static void
bobgui_column_view_row_widget_allocate (BobguiWidget *widget,
                                     int        width,
                                     int        height,
                                     int        baseline)
{
  BobguiWidget *child;

  for (child = _bobgui_widget_get_first_child (widget);
       child != NULL;
       child = _bobgui_widget_get_next_sibling (child))
    {
      BobguiColumnViewColumn *column;
      int col_x, col_width, min;

      if (!bobgui_widget_should_layout (child))
        continue;

      column = bobgui_column_view_row_child_get_column (child);
      bobgui_column_view_column_get_header_allocation (column, &col_x, &col_width);

      bobgui_widget_measure (child, BOBGUI_ORIENTATION_HORIZONTAL, -1, &min, NULL, NULL, NULL);

      bobgui_widget_size_allocate (child, &(BobguiAllocation) { col_x, 0, MAX (min, col_width), height }, baseline);
    }
}

static void
add_arrow_bindings (BobguiWidgetClass   *widget_class,
		    guint             keysym,
		    BobguiDirectionType  direction)
{
  guint keypad_keysym = keysym - GDK_KEY_Left + GDK_KEY_KP_Left;

  bobgui_widget_class_add_binding_signal (widget_class, keysym, 0,
                                       "move-focus",
                                       "(i)",
                                       direction);
  bobgui_widget_class_add_binding_signal (widget_class, keysym, GDK_CONTROL_MASK,
                                       "move-focus",
                                       "(i)",
                                       direction);
  bobgui_widget_class_add_binding_signal (widget_class, keypad_keysym, 0,
                                       "move-focus",
                                       "(i)",
                                       direction);
  bobgui_widget_class_add_binding_signal (widget_class, keypad_keysym, GDK_CONTROL_MASK,
                                       "move-focus",
                                       "(i)",
                                       direction);
}

static void
bobgui_column_view_row_widget_class_init (BobguiColumnViewRowWidgetClass *klass)
{
  BobguiListFactoryWidgetClass *factory_class = BOBGUI_LIST_FACTORY_WIDGET_CLASS (klass);
  BobguiListItemBaseClass *base_class = BOBGUI_LIST_ITEM_BASE_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  factory_class->create_object = bobgui_column_view_row_widget_create_object;
  factory_class->setup_object = bobgui_column_view_row_widget_setup_object;
  factory_class->update_object = bobgui_column_view_row_widget_update_object;
  factory_class->teardown_object = bobgui_column_view_row_widget_teardown_object;

  base_class->update = bobgui_column_view_row_widget_update;

  widget_class->focus = bobgui_column_view_row_widget_focus;
  widget_class->grab_focus = bobgui_column_view_row_widget_grab_focus;
  widget_class->set_focus_child = bobgui_column_view_row_widget_set_focus_child;
  widget_class->measure = bobgui_column_view_row_widget_measure;
  widget_class->size_allocate = bobgui_column_view_row_widget_allocate;

  object_class->dispose = bobgui_column_view_row_widget_dispose;

  add_arrow_bindings (widget_class, GDK_KEY_Left, BOBGUI_DIR_LEFT);
  add_arrow_bindings (widget_class, GDK_KEY_Right, BOBGUI_DIR_RIGHT);

  /* This gets overwritten by bobgui_column_view_row_widget_new() but better safe than sorry */
  bobgui_widget_class_set_css_name (widget_class, g_intern_static_string ("row"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_ROW);
}

static void
bobgui_column_view_row_widget_init (BobguiColumnViewRowWidget *self)
{
  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), TRUE);
}

BobguiWidget *
bobgui_column_view_row_widget_new (BobguiListItemFactory *factory,
                                gboolean            is_header)
{
  return g_object_new (BOBGUI_TYPE_COLUMN_VIEW_ROW_WIDGET,
                       "factory", factory,
                       "css-name", is_header ? "header" : "row",
                       "selectable", TRUE,
                       "activatable", TRUE,
                       NULL);
}

void
bobgui_column_view_row_widget_add_child (BobguiColumnViewRowWidget *self,
                                      BobguiWidget              *child)
{
  bobgui_widget_set_parent (child, BOBGUI_WIDGET (self));
}

void
bobgui_column_view_row_widget_reorder_child (BobguiColumnViewRowWidget *self,
                                          BobguiWidget              *child,
                                          guint                   position)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);
  BobguiWidget *sibling = NULL;

  if (position > 0)
    {
      BobguiWidget *c;
      guint i;

      for (c = bobgui_widget_get_first_child (widget), i = 0;
           c;
           c = bobgui_widget_get_next_sibling (c), i++)
        {
          if (i + 1 == position)
            {
              sibling = c;
              break;
            }
        }
    }

  if (child != sibling)
    bobgui_widget_insert_after (child, widget, sibling);
}

void
bobgui_column_view_row_widget_remove_child (BobguiColumnViewRowWidget *self,
                                         BobguiWidget              *child)
{
  if (BOBGUI_IS_COLUMN_VIEW_CELL_WIDGET (child))
    bobgui_column_view_cell_widget_unset_column (BOBGUI_COLUMN_VIEW_CELL_WIDGET (child));

  bobgui_widget_unparent (child);
}

