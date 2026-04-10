/*
 * Copyright © 2019 Benjamin Otte
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

#include "bobguicolumnviewcellwidgetprivate.h"

#include "bobguicolumnviewcellprivate.h"
#include "bobguicolumnviewcolumnprivate.h"
#include "bobguicolumnviewrowwidgetprivate.h"
#include "bobguicssboxesprivate.h"
#include "bobguicssnodeprivate.h"
#include "bobguilistfactorywidgetprivate.h"
#include "bobguiprivate.h"
#include "bobguiwidgetprivate.h"


struct _BobguiColumnViewCellWidget
{
  BobguiListItemWidget parent_instance;

  BobguiColumnViewColumn *column;

  /* This list isn't sorted - next/prev refer to list elements, not rows in the list */
  BobguiColumnViewCellWidget *next_cell;
  BobguiColumnViewCellWidget *prev_cell;
};

struct _BobguiColumnViewCellWidgetClass
{
  BobguiListItemWidgetClass parent_class;
};

G_DEFINE_TYPE (BobguiColumnViewCellWidget, bobgui_column_view_cell_widget, BOBGUI_TYPE_LIST_FACTORY_WIDGET)

static gboolean
bobgui_column_view_cell_widget_focus (BobguiWidget        *widget,
                                   BobguiDirectionType  direction)
{
  BobguiWidget *child = bobgui_widget_get_first_child (widget);

  if (bobgui_widget_get_focus_child (widget))
    {
      /* focus is in the child */

      /* Try moving inside the child */
      if (bobgui_widget_child_focus (child, direction))
        return TRUE;

      /* That failed, exit it */
      if (direction == BOBGUI_DIR_TAB_BACKWARD)
        return bobgui_widget_grab_focus_self (widget);
      else
        return FALSE;
    }
  else if (bobgui_widget_is_focus (widget))
    {
      /* The widget has focus */
      if (direction == BOBGUI_DIR_TAB_FORWARD)
        {
          if (child)
            return bobgui_widget_child_focus (child, direction);
        }

      return FALSE;
    }
  else
    {
      /* focus coming in from the outside */
      if (direction == BOBGUI_DIR_TAB_BACKWARD)
        {
          if (child &&
              bobgui_widget_child_focus (child, direction))
            return TRUE;

          return bobgui_widget_grab_focus_self (widget);
        }
      else
        {
          if (bobgui_widget_grab_focus_self (widget))
            return TRUE;

          if (child &&
              bobgui_widget_child_focus (child, direction))
            return TRUE;

          return FALSE;
        }
    }
}

static gboolean
bobgui_column_view_cell_widget_grab_focus (BobguiWidget *widget)
{
  BobguiWidget *child;

  if (BOBGUI_WIDGET_CLASS (bobgui_column_view_cell_widget_parent_class)->grab_focus (widget))
    return TRUE;

  child = bobgui_widget_get_first_child (widget);
  if (child && bobgui_widget_grab_focus (child))
    return TRUE;

  return FALSE;
}

static gpointer
bobgui_column_view_cell_widget_create_object (BobguiListFactoryWidget *fw)
{
  return bobgui_column_view_cell_new ();
}

static void
bobgui_column_view_cell_widget_setup_object (BobguiListFactoryWidget *fw,
                                          gpointer              object)
{
  BobguiColumnViewCellWidget *self = BOBGUI_COLUMN_VIEW_CELL_WIDGET (fw);
  BobguiColumnViewCell *cell = object;

  BOBGUI_LIST_FACTORY_WIDGET_CLASS (bobgui_column_view_cell_widget_parent_class)->setup_object (fw, object);

  cell->cell = self;

  bobgui_column_view_cell_widget_set_child (BOBGUI_COLUMN_VIEW_CELL_WIDGET (self), cell->child);

  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), cell->focusable);

  bobgui_column_view_cell_do_notify (cell,
                                  bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self)) != NULL,
                                  bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self)) != BOBGUI_INVALID_LIST_POSITION,
                                  bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (self)));
}

static void
bobgui_column_view_cell_widget_teardown_object (BobguiListFactoryWidget *fw,
                                             gpointer              object)
{
  BobguiColumnViewCellWidget *self = BOBGUI_COLUMN_VIEW_CELL_WIDGET (fw);
  BobguiColumnViewCell *cell = object;

  BOBGUI_LIST_FACTORY_WIDGET_CLASS (bobgui_column_view_cell_widget_parent_class)->teardown_object (fw, object);

  cell->cell = NULL;

  bobgui_column_view_cell_widget_set_child (BOBGUI_COLUMN_VIEW_CELL_WIDGET (self), NULL);

  bobgui_widget_set_focusable (BOBGUI_WIDGET (self), FALSE);

  bobgui_column_view_cell_do_notify (cell,
                                  bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (self)) != NULL,
                                  bobgui_list_item_base_get_position (BOBGUI_LIST_ITEM_BASE (self)) != BOBGUI_INVALID_LIST_POSITION,
                                  bobgui_list_item_base_get_selected (BOBGUI_LIST_ITEM_BASE (self)));
}

static void
bobgui_column_view_cell_widget_update_object (BobguiListFactoryWidget *fw,
                                           gpointer              object,
                                           guint                 position,
                                           gpointer              item,
                                           gboolean              selected)
{
  BobguiColumnViewCellWidget *self = BOBGUI_COLUMN_VIEW_CELL_WIDGET (fw);
  BobguiListItemBase *base = BOBGUI_LIST_ITEM_BASE (self);
  BobguiColumnViewCell *cell = object;
  /* Track notify manually instead of freeze/thaw_notify for performance reasons. */
  gboolean notify_item = FALSE, notify_position = FALSE, notify_selected = FALSE;

  /* FIXME: It's kinda evil to notify external objects from here... */
  notify_item = bobgui_list_item_base_get_item (base) != item;
  notify_position = bobgui_list_item_base_get_position (base) != position;
  notify_selected = bobgui_list_item_base_get_selected (base) != selected;

  BOBGUI_LIST_FACTORY_WIDGET_CLASS (bobgui_column_view_cell_widget_parent_class)->update_object (fw,
                                                                                           object,
                                                                                           position,
                                                                                           item,
                                                                                           selected);

  if (cell)
    bobgui_column_view_cell_do_notify (cell, notify_item, notify_position, notify_selected);
}

static int
unadjust_width (BobguiWidget *widget,
                int        width)
{
  BobguiCssBoxes boxes;

  if (width <= -1)
    return -1;

  bobgui_css_boxes_init_border_box (&boxes,
                                 bobgui_css_node_get_style (bobgui_widget_get_css_node (widget)),
                                 0, 0,
                                 width, 100000);
  return MAX (0, floor (bobgui_css_boxes_get_content_rect (&boxes)->size.width));
}

static void
bobgui_column_view_cell_widget_measure (BobguiWidget      *widget,
                                     BobguiOrientation  orientation,
                                     int             for_size,
                                     int            *minimum,
                                     int            *natural,
                                     int            *minimum_baseline,
                                     int            *natural_baseline)
{
  BobguiColumnViewCellWidget *cell = BOBGUI_COLUMN_VIEW_CELL_WIDGET (widget);
  BobguiWidget *child = bobgui_widget_get_first_child (widget);
  int fixed_width, unadj_width;

  fixed_width = bobgui_column_view_column_get_fixed_width (cell->column);
  unadj_width = unadjust_width (widget, fixed_width);

  if (orientation == BOBGUI_ORIENTATION_VERTICAL)
    {
      if (fixed_width > -1)
        {
          int min;

          if (for_size == -1)
            for_size = unadj_width;
          else
            for_size = MIN (for_size, unadj_width);

          bobgui_widget_measure (child, BOBGUI_ORIENTATION_HORIZONTAL, -1, &min, NULL, NULL, NULL);
          for_size = MAX (for_size, min);
        }
    }

  if (child)
    bobgui_widget_measure (child, orientation, for_size, minimum, natural, minimum_baseline, natural_baseline);

  if (orientation == BOBGUI_ORIENTATION_HORIZONTAL)
    {
      if (fixed_width > -1)
        {
          *minimum = 0;
          *natural = unadj_width;
        }
    }
}

static void
bobgui_column_view_cell_widget_size_allocate (BobguiWidget *widget,
                                           int        width,
                                           int        height,
                                           int        baseline)
{
  BobguiWidget *child = bobgui_widget_get_first_child (widget);

  if (child)
    {
      int min;

      bobgui_widget_measure (child, BOBGUI_ORIENTATION_HORIZONTAL, height, &min, NULL, NULL, NULL);

      bobgui_widget_allocate (child, MAX (min, width), height, baseline, NULL);
    }
}

/* This should be to be called when unsetting the parent, but we have no
 * set_parent vfunc().
 */
void
bobgui_column_view_cell_widget_unset_column (BobguiColumnViewCellWidget *self)
{
  if (self->column)
    {
      bobgui_column_view_column_remove_cell (self->column, self);

      if (self->prev_cell)
        self->prev_cell->next_cell = self->next_cell;
      if (self->next_cell)
        self->next_cell->prev_cell = self->prev_cell;

      self->prev_cell = NULL;
      self->next_cell = NULL;

      g_clear_object (&self->column);
    }
}

static void
bobgui_column_view_cell_widget_dispose (GObject *object)
{
  BobguiColumnViewCellWidget *self = BOBGUI_COLUMN_VIEW_CELL_WIDGET (object);

  /* unset_parent() forgot to call this. Be very angry. */
  g_warn_if_fail (self->column == NULL);

  G_OBJECT_CLASS (bobgui_column_view_cell_widget_parent_class)->dispose (object);
}

static BobguiSizeRequestMode
bobgui_column_view_cell_widget_get_request_mode (BobguiWidget *widget)
{
  BobguiWidget *child = bobgui_widget_get_first_child (widget);

  if (child)
    return bobgui_widget_get_request_mode (child);
  else
    return BOBGUI_SIZE_REQUEST_CONSTANT_SIZE;
}

static void
bobgui_column_view_cell_widget_class_init (BobguiColumnViewCellWidgetClass *klass)
{
  BobguiListFactoryWidgetClass *factory_class = BOBGUI_LIST_FACTORY_WIDGET_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  factory_class->create_object = bobgui_column_view_cell_widget_create_object;
  factory_class->setup_object = bobgui_column_view_cell_widget_setup_object;
  factory_class->update_object = bobgui_column_view_cell_widget_update_object;
  factory_class->teardown_object = bobgui_column_view_cell_widget_teardown_object;

  widget_class->focus = bobgui_column_view_cell_widget_focus;
  widget_class->grab_focus = bobgui_column_view_cell_widget_grab_focus;
  widget_class->measure = bobgui_column_view_cell_widget_measure;
  widget_class->size_allocate = bobgui_column_view_cell_widget_size_allocate;
  widget_class->get_request_mode = bobgui_column_view_cell_widget_get_request_mode;

  gobject_class->dispose = bobgui_column_view_cell_widget_dispose;

  bobgui_widget_class_set_css_name (widget_class, I_("cell"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GRID_CELL);
}

static void
bobgui_column_view_cell_widget_resize_func (BobguiWidget *widget)
{
  BobguiColumnViewCellWidget *self = BOBGUI_COLUMN_VIEW_CELL_WIDGET (widget);

  if (self->column)
    bobgui_column_view_column_queue_resize (self->column);
}

static void
bobgui_column_view_cell_widget_init (BobguiColumnViewCellWidget *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  bobgui_widget_set_focusable (widget, FALSE);
  bobgui_widget_set_overflow (widget, BOBGUI_OVERFLOW_HIDDEN);
  /* FIXME: Figure out if setting the manager class to INVALID should work */
  bobgui_widget_set_layout_manager (widget, NULL);
  widget->priv->resize_func = bobgui_column_view_cell_widget_resize_func;
}

BobguiWidget *
bobgui_column_view_cell_widget_new (BobguiColumnViewColumn *column,
                                 gboolean             inert)
{
  BobguiColumnViewCellWidget *self;

  self = g_object_new (BOBGUI_TYPE_COLUMN_VIEW_CELL_WIDGET,
                       "factory", inert ? NULL : bobgui_column_view_column_get_factory (column),
                       NULL);

  self->column = g_object_ref (column);

  self->next_cell = bobgui_column_view_column_get_first_cell (self->column);
  if (self->next_cell)
    self->next_cell->prev_cell = self;

  bobgui_column_view_column_add_cell (self->column, self);

  return BOBGUI_WIDGET (self);
}

void
bobgui_column_view_cell_widget_remove (BobguiColumnViewCellWidget *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  bobgui_column_view_row_widget_remove_child (BOBGUI_COLUMN_VIEW_ROW_WIDGET (bobgui_widget_get_parent (widget)), widget);
}

BobguiColumnViewCellWidget *
bobgui_column_view_cell_widget_get_next (BobguiColumnViewCellWidget *self)
{
  return self->next_cell;
}

BobguiColumnViewCellWidget *
bobgui_column_view_cell_widget_get_prev (BobguiColumnViewCellWidget *self)
{
  return self->prev_cell;
}

BobguiColumnViewColumn *
bobgui_column_view_cell_widget_get_column (BobguiColumnViewCellWidget *self)
{
  return self->column;
}

void
bobgui_column_view_cell_widget_set_child (BobguiColumnViewCellWidget *self,
                                       BobguiWidget               *child)
{
  BobguiWidget *cur_child = bobgui_widget_get_first_child (BOBGUI_WIDGET (self));

  if (cur_child == child)
    return;

  g_clear_pointer (&cur_child, bobgui_widget_unparent);

  if (child)
    bobgui_widget_set_parent (child, BOBGUI_WIDGET (self));
}
