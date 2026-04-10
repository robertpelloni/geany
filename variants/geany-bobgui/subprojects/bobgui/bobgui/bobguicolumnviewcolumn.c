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

#include "bobguicolumnviewcolumnprivate.h"
#include "bobguicolumnviewsorterprivate.h"

#include "bobguicolumnviewprivate.h"
#include "bobguicolumnviewrowwidgetprivate.h"
#include "bobguicolumnviewtitleprivate.h"
#include "bobguilistbaseprivate.h"
#include "bobguimain.h"
#include "bobguiprivate.h"
#include "bobguirbtreeprivate.h"
#include "bobguisizegroup.h"
#include "bobguiwidgetprivate.h"
#include "bobguisorter.h"

/**
 * BobguiColumnViewColumn:
 *
 * Represents the columns in a `BobguiColumnView`.
 *
 * The main ingredient for a `BobguiColumnViewColumn` is the `BobguiListItemFactory`
 * that tells the columnview how to create cells for this column from items in
 * the model.
 *
 * Columns have a title, and can optionally have a header menu set
 * with [method@Bobgui.ColumnViewColumn.set_header_menu].
 *
 * A sorter can be associated with a column using
 * [method@Bobgui.ColumnViewColumn.set_sorter], to let users influence sorting
 * by clicking on the column header.
 */

struct _BobguiColumnViewColumn
{
  GObject parent_instance;

  BobguiListItemFactory *factory;
  char *title;
  char *id;
  BobguiSorter *sorter;

  /* data for the view */
  BobguiColumnView *view;
  BobguiWidget *header;

  int minimum_size_request;
  int natural_size_request;
  int allocation_offset;
  int allocation_size;
  int header_position;

  int fixed_width;

  guint visible     : 1;
  guint resizable   : 1;
  guint expand      : 1;

  GMenuModel *menu;

  /* This list isn't sorted - this is just caching for performance */
  BobguiColumnViewCellWidget *first_cell; /* no reference, just caching */
};

struct _BobguiColumnViewColumnClass
{
  GObjectClass parent_class;
};

enum
{
  PROP_0,
  PROP_COLUMN_VIEW,
  PROP_FACTORY,
  PROP_TITLE,
  PROP_SORTER,
  PROP_VISIBLE,
  PROP_HEADER_MENU,
  PROP_RESIZABLE,
  PROP_EXPAND,
  PROP_FIXED_WIDTH,
  PROP_ID,

  N_PROPS
};

G_DEFINE_TYPE (BobguiColumnViewColumn, bobgui_column_view_column, G_TYPE_OBJECT)

static GParamSpec *properties[N_PROPS] = { NULL, };

static void
bobgui_column_view_column_dispose (GObject *object)
{
  BobguiColumnViewColumn *self = BOBGUI_COLUMN_VIEW_COLUMN (object);

  g_assert (self->view == NULL); /* would hold a ref otherwise */
  g_assert (self->first_cell == NULL); /* no view = no children */

  g_clear_object (&self->factory);
  g_clear_object (&self->sorter);
  g_clear_pointer (&self->title, g_free);
  g_clear_object (&self->menu);
  g_clear_pointer (&self->id, g_free);

  G_OBJECT_CLASS (bobgui_column_view_column_parent_class)->dispose (object);
}

static void
bobgui_column_view_column_get_property (GObject    *object,
                                     guint       property_id,
                                     GValue     *value,
                                     GParamSpec *pspec)
{
  BobguiColumnViewColumn *self = BOBGUI_COLUMN_VIEW_COLUMN (object);

  switch (property_id)
    {
    case PROP_COLUMN_VIEW:
      g_value_set_object (value, self->view);
      break;

    case PROP_FACTORY:
      g_value_set_object (value, self->factory);
      break;

    case PROP_TITLE:
      g_value_set_string (value, self->title);
      break;

    case PROP_SORTER:
      g_value_set_object (value, self->sorter);
      break;

    case PROP_VISIBLE:
      g_value_set_boolean (value, self->visible);
      break;

    case PROP_HEADER_MENU:
      g_value_set_object (value, self->menu);
      break;

    case PROP_RESIZABLE:
      g_value_set_boolean (value, self->resizable);
      break;

    case PROP_EXPAND:
      g_value_set_boolean (value, self->expand);
      break;

    case PROP_FIXED_WIDTH:
      g_value_set_int (value, self->fixed_width);
      break;

    case PROP_ID:
      g_value_set_string (value, self->id);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_column_view_column_set_property (GObject      *object,
                                     guint         property_id,
                                     const GValue *value,
                                     GParamSpec   *pspec)
{
  BobguiColumnViewColumn *self = BOBGUI_COLUMN_VIEW_COLUMN (object);

  switch (property_id)
    {
    case PROP_FACTORY:
      bobgui_column_view_column_set_factory (self, g_value_get_object (value));
      break;

    case PROP_TITLE:
      bobgui_column_view_column_set_title (self, g_value_get_string (value));
      break;

    case PROP_SORTER:
      bobgui_column_view_column_set_sorter (self, g_value_get_object (value));
      break;

    case PROP_VISIBLE:
      bobgui_column_view_column_set_visible (self, g_value_get_boolean (value));
      break;

    case PROP_HEADER_MENU:
      bobgui_column_view_column_set_header_menu (self, g_value_get_object (value));
      break;

    case PROP_RESIZABLE:
      bobgui_column_view_column_set_resizable (self, g_value_get_boolean (value));
      break;

    case PROP_EXPAND:
      bobgui_column_view_column_set_expand (self, g_value_get_boolean (value));
      break;

    case PROP_FIXED_WIDTH:
      bobgui_column_view_column_set_fixed_width (self, g_value_get_int (value));
      break;

    case PROP_ID:
      bobgui_column_view_column_set_id (self, g_value_get_string (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_column_view_column_class_init (BobguiColumnViewColumnClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->dispose = bobgui_column_view_column_dispose;
  gobject_class->get_property = bobgui_column_view_column_get_property;
  gobject_class->set_property = bobgui_column_view_column_set_property;

  /**
   * BobguiColumnViewColumn:column-view:
   *
   * The `BobguiColumnView` this column is a part of.
   */
  properties[PROP_COLUMN_VIEW] =
    g_param_spec_object ("column-view", NULL, NULL,
                         BOBGUI_TYPE_COLUMN_VIEW,
                         G_PARAM_READABLE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnViewColumn:factory:
   *
   * Factory for populating list items.
   *
   * The factory must be for configuring [class@Bobgui.ColumnViewCell] objects.
   */
  properties[PROP_FACTORY] =
    g_param_spec_object ("factory", NULL, NULL,
                         BOBGUI_TYPE_LIST_ITEM_FACTORY,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnViewColumn:title:
   *
   * Title displayed in the header.
   */
  properties[PROP_TITLE] =
    g_param_spec_string ("title", NULL, NULL,
                          NULL,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiColumnViewColumn:sorter:
   *
   * Sorter for sorting items according to this column.
   */
  properties[PROP_SORTER] =
    g_param_spec_object ("sorter", NULL, NULL,
                         BOBGUI_TYPE_SORTER,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnViewColumn:visible:
   *
   * Whether this column is visible.
   */
  properties[PROP_VISIBLE] =
    g_param_spec_boolean ("visible", NULL, NULL,
                          TRUE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnViewColumn:header-menu:
   *
   * Menu model used to create the context menu for the column header.
   */
  properties[PROP_HEADER_MENU] =
    g_param_spec_object ("header-menu", NULL, NULL,
                         G_TYPE_MENU_MODEL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnViewColumn:resizable:
   *
   * Whether this column is resizable.
   */
  properties[PROP_RESIZABLE] =
    g_param_spec_boolean ("resizable", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnViewColumn:expand:
   *
   * Column gets share of extra width allocated to the view.
   */
  properties[PROP_EXPAND] =
    g_param_spec_boolean ("expand", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnViewColumn:fixed-width:
   *
   * If not -1, this is the width that the column is allocated,
   * regardless of the size of its content.
   */
  properties[PROP_FIXED_WIDTH] =
    g_param_spec_int ("fixed-width", NULL, NULL,
                      -1, G_MAXINT, -1,
                      G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiColumnViewColumn:id:
   *
   * An ID for the column.
   *
   * BOBGUI is not currently using the ID for anything, but
   * it can be used by applications when saving column view
   * configurations.
   *
   * It is up to applications to ensure uniqueness of IDs.
   *
   * Since: 4.10
   */
  properties[PROP_ID] =
    g_param_spec_string ("id", NULL, NULL,
                          NULL,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);
}

static void
bobgui_column_view_column_init (BobguiColumnViewColumn *self)
{
  self->minimum_size_request = -1;
  self->natural_size_request = -1;
  self->visible = TRUE;
  self->resizable = FALSE;
  self->expand = FALSE;
  self->fixed_width = -1;
}

/**
 * bobgui_column_view_column_new:
 * @title: (nullable): Title to use for this column
 * @factory: (transfer full) (nullable): The factory to populate items with
 *
 * Creates a new `BobguiColumnViewColumn` that uses the given @factory for
 * mapping items to widgets.
 *
 * You most likely want to call [method@Bobgui.ColumnView.append_column] next.
 *
 * The function takes ownership of the argument, so you can write code like:
 *
 * ```c
 * column = bobgui_column_view_column_new (_("Name"),
 *   bobgui_builder_list_item_factory_new_from_resource ("/name.ui"));
 * ```
 *
 * Returns: a new `BobguiColumnViewColumn` using the given @factory
 */
BobguiColumnViewColumn *
bobgui_column_view_column_new (const char         *title,
                            BobguiListItemFactory *factory)
{
  BobguiColumnViewColumn *result;

  g_return_val_if_fail (factory == NULL || BOBGUI_IS_LIST_ITEM_FACTORY (factory), NULL);

  result = g_object_new (BOBGUI_TYPE_COLUMN_VIEW_COLUMN,
                         "factory", factory,
                         "title", title,
                         NULL);

  g_clear_object (&factory);

  return result;
}

BobguiColumnViewCellWidget *
bobgui_column_view_column_get_first_cell (BobguiColumnViewColumn *self)
{
  return self->first_cell;
}

void
bobgui_column_view_column_add_cell (BobguiColumnViewColumn *self,
                                 BobguiColumnViewCellWidget   *cell)
{
  self->first_cell = cell;

  bobgui_widget_set_visible (BOBGUI_WIDGET (cell), self->visible);
  bobgui_column_view_column_queue_resize (self);
}

void
bobgui_column_view_column_remove_cell (BobguiColumnViewColumn *self,
                                    BobguiColumnViewCellWidget   *cell)
{
  if (cell == self->first_cell)
    self->first_cell = bobgui_column_view_cell_widget_get_next (cell);

  bobgui_column_view_column_queue_resize (self);
  bobgui_widget_queue_resize (BOBGUI_WIDGET (cell));
}

void
bobgui_column_view_column_queue_resize (BobguiColumnViewColumn *self)
{
  BobguiColumnViewCellWidget *cell;

  if (self->minimum_size_request < 0)
    return;

  self->minimum_size_request = -1;
  self->natural_size_request = -1;

  if (self->header)
    bobgui_widget_queue_resize (self->header);

  for (cell = self->first_cell; cell; cell = bobgui_column_view_cell_widget_get_next (cell))
    {
      bobgui_widget_queue_resize (BOBGUI_WIDGET (cell));
    }
}

void
bobgui_column_view_column_measure (BobguiColumnViewColumn *self,
                                int                 *minimum,
                                int                 *natural)
{
  if (self->fixed_width > -1)
    {
      self->minimum_size_request  = self->fixed_width;
      self->natural_size_request  = self->fixed_width;
    }

  if (self->minimum_size_request < 0)
    {
      BobguiColumnViewCellWidget *cell;
      int min, nat, cell_min, cell_nat;

      if (self->header)
        {
          bobgui_widget_measure (self->header, BOBGUI_ORIENTATION_HORIZONTAL, -1, &min, &nat, NULL, NULL);
        }
      else
        {
          min = 0;
          nat = 0;
        }

      for (cell = self->first_cell; cell; cell = bobgui_column_view_cell_widget_get_next (cell))
        {
          bobgui_widget_measure (BOBGUI_WIDGET (cell),
                              BOBGUI_ORIENTATION_HORIZONTAL,
                              -1,
                              &cell_min, &cell_nat,
                              NULL, NULL);

          min = MAX (min, cell_min);
          nat = MAX (nat, cell_nat);
        }

      self->minimum_size_request = min;
      self->natural_size_request = nat;
    }

  *minimum = self->minimum_size_request;
  *natural = self->natural_size_request;
}

void
bobgui_column_view_column_allocate (BobguiColumnViewColumn *self,
                                 int                  offset,
                                 int                  size)
{
  self->allocation_offset = offset;
  self->allocation_size = size;
  self->header_position = offset;
}

void
bobgui_column_view_column_get_allocation (BobguiColumnViewColumn *self,
                                       int                 *offset,
                                       int                 *size)
{
  if (offset)
    *offset = self->allocation_offset;
  if (size)
    *size = self->allocation_size;
}

static void
bobgui_column_view_column_create_cells (BobguiColumnViewColumn *self)
{
  BobguiListView *list;
  BobguiWidget *row;

  if (self->first_cell)
    return;

  list = bobgui_column_view_get_list_view (BOBGUI_COLUMN_VIEW (self->view));
  for (row = bobgui_widget_get_first_child (BOBGUI_WIDGET (list));
       row != NULL;
       row = bobgui_widget_get_next_sibling (row))
    {
      BobguiColumnViewRowWidget *list_item;
      BobguiListItemBase *base;
      BobguiWidget *cell;

      list_item = BOBGUI_COLUMN_VIEW_ROW_WIDGET (row);
      base = BOBGUI_LIST_ITEM_BASE (row);
      cell = bobgui_column_view_cell_widget_new (self, bobgui_column_view_is_inert (self->view));
      bobgui_column_view_row_widget_add_child (list_item, cell);
      bobgui_list_item_base_update (BOBGUI_LIST_ITEM_BASE (cell),
                                 bobgui_list_item_base_get_position (base),
                                 bobgui_list_item_base_get_item (base),
                                 bobgui_list_item_base_get_selected (base));
    }
}

static void
bobgui_column_view_column_remove_cells (BobguiColumnViewColumn *self)
{
  while (self->first_cell)
    bobgui_column_view_cell_widget_remove (self->first_cell);
}

static void
bobgui_column_view_column_create_header (BobguiColumnViewColumn *self)
{
  if (self->header != NULL)
    return;

  self->header = bobgui_column_view_title_new (self);
  bobgui_widget_set_visible (self->header, self->visible);
  bobgui_column_view_row_widget_add_child (bobgui_column_view_get_header_widget (self->view),
                                  self->header);
  bobgui_column_view_column_queue_resize (self);
}

static void
bobgui_column_view_column_remove_header (BobguiColumnViewColumn *self)
{
  if (self->header == NULL)
    return;

  bobgui_column_view_row_widget_remove_child (bobgui_column_view_get_header_widget (self->view),
                                     self->header);
  self->header = NULL;
  bobgui_column_view_column_queue_resize (self);
}

static void
bobgui_column_view_column_ensure_cells (BobguiColumnViewColumn *self)
{
  if (self->view && bobgui_column_view_column_get_visible (self))
    bobgui_column_view_column_create_cells (self);
  else
    bobgui_column_view_column_remove_cells (self);

  if (self->view)
    bobgui_column_view_column_create_header (self);
  else
    bobgui_column_view_column_remove_header (self);
}

/**
 * bobgui_column_view_column_get_column_view:
 * @self: a column
 *
 * Gets the column view that's currently displaying this column.
 *
 * If @self has not been added to a column view yet, `NULL` is returned.
 *
 * Returns: (nullable) (transfer none): The column view displaying @self.
 */
BobguiColumnView *
bobgui_column_view_column_get_column_view (BobguiColumnViewColumn *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self), NULL);

  return self->view;
}

void
bobgui_column_view_column_set_column_view (BobguiColumnViewColumn *self,
                                        BobguiColumnView       *view)
{
  if (self->view == view)
    return;

  bobgui_column_view_column_remove_cells (self);
  bobgui_column_view_column_remove_header (self);

  self->view = view;

  bobgui_column_view_column_ensure_cells (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_COLUMN_VIEW]);
}

void
bobgui_column_view_column_set_position (BobguiColumnViewColumn *self,
                                     guint                position)
{
  BobguiColumnViewCellWidget *cell;

  bobgui_column_view_row_widget_reorder_child (bobgui_column_view_get_header_widget (self->view),
                                      self->header,
                                      position);

  for (cell = self->first_cell; cell; cell = bobgui_column_view_cell_widget_get_next (cell))
    {
      BobguiColumnViewRowWidget *list_item;

      list_item = BOBGUI_COLUMN_VIEW_ROW_WIDGET (bobgui_widget_get_parent (BOBGUI_WIDGET (cell)));
      bobgui_column_view_row_widget_reorder_child (list_item, BOBGUI_WIDGET (cell), position);
    }
}

/**
 * bobgui_column_view_column_get_factory:
 * @self: a column
 *
 * Gets the factory that's currently used to populate list items
 * for this column.
 *
 * Returns: (nullable) (transfer none): The factory in use
 **/
BobguiListItemFactory *
bobgui_column_view_column_get_factory (BobguiColumnViewColumn *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self), NULL);

  return self->factory;
}

void
bobgui_column_view_column_update_factory (BobguiColumnViewColumn *self,
                                       gboolean             inert)
{
  BobguiListItemFactory *factory;
  BobguiColumnViewCellWidget *cell;

  if (self->factory == NULL)
    return;

  if (inert)
    factory = NULL;
  else
    factory = self->factory;

  for (cell = self->first_cell;
       cell;
       cell = bobgui_column_view_cell_widget_get_next (cell))
    {
      bobgui_list_factory_widget_set_factory (BOBGUI_LIST_FACTORY_WIDGET (cell), factory);
    }
}

/**
 * bobgui_column_view_column_set_factory:
 * @self: a column
 * @factory: (nullable) (transfer none): the factory to use
 *
 * Sets the `BobguiListItemFactory` to use for populating list items
 * for this column.
 */
void
bobgui_column_view_column_set_factory (BobguiColumnViewColumn *self,
                                    BobguiListItemFactory  *factory)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self));
  g_return_if_fail (factory == NULL || BOBGUI_LIST_ITEM_FACTORY (factory));

  if (self->factory && !factory)
    bobgui_column_view_column_update_factory (self, TRUE);

  if (!g_set_object (&self->factory, factory))
    return;

  if (self->view && !bobgui_column_view_is_inert (self->view))
    bobgui_column_view_column_update_factory (self, FALSE);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FACTORY]);
}

/**
 * bobgui_column_view_column_set_title:
 * @self: a column
 * @title: (nullable): Title to use for this column
 *
 * Sets the title of this column.
 *
 * The title is displayed in the header of a `BobguiColumnView`
 * for this column and is therefore user-facing text that should
 * be translated.
 */
void
bobgui_column_view_column_set_title (BobguiColumnViewColumn *self,
                                  const char          *title)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self));

  if (g_strcmp0 (self->title, title) == 0)
    return;

  g_free (self->title);
  self->title = g_strdup (title);

  if (self->header)
    bobgui_column_view_title_set_title (BOBGUI_COLUMN_VIEW_TITLE (self->header), title);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TITLE]);
}

/**
 * bobgui_column_view_column_get_title:
 * @self: a column
 *
 * Returns the title set with [method@Bobgui.ColumnViewColumn.set_title].
 *
 * Returns: (nullable): The column's title
 */
const char *
bobgui_column_view_column_get_title (BobguiColumnViewColumn *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self), FALSE);

  return self->title;
}

static void
bobgui_column_view_column_remove_from_sorter (BobguiColumnViewColumn *self)
{
  if (self->view == NULL)
    return;

  bobgui_column_view_sorter_remove_column (BOBGUI_COLUMN_VIEW_SORTER (bobgui_column_view_get_sorter (self->view)), self);
}

/**
 * bobgui_column_view_column_set_sorter:
 * @self: a column
 * @sorter: (nullable): the `BobguiSorter` to associate with @column
 *
 * Associates a sorter with the column.
 *
 * If @sorter is unset, the column will not let users change
 * the sorting by clicking on its header.
 *
 * This sorter can be made active by clicking on the column
 * header, or by calling [method@Bobgui.ColumnView.sort_by_column].
 *
 * See [method@Bobgui.ColumnView.get_sorter] for the necessary steps
 * for setting up customizable sorting for [class@Bobgui.ColumnView].
 */
void
bobgui_column_view_column_set_sorter (BobguiColumnViewColumn *self,
                                   BobguiSorter           *sorter)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self));
  g_return_if_fail (sorter == NULL || BOBGUI_IS_SORTER (sorter));

  if (!g_set_object (&self->sorter, sorter))
    return;

  bobgui_column_view_column_remove_from_sorter (self);

  if (self->header)
    bobgui_column_view_title_update_sort (BOBGUI_COLUMN_VIEW_TITLE (self->header));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SORTER]);
}

/**
 * bobgui_column_view_column_get_sorter:
 * @self: a column
 *
 * Returns the sorter that is associated with the column.
 *
 * Returns: (nullable) (transfer none): the `BobguiSorter` of @self
 */
BobguiSorter *
bobgui_column_view_column_get_sorter (BobguiColumnViewColumn *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self), NULL);

  return self->sorter;
}

void
bobgui_column_view_column_notify_sort (BobguiColumnViewColumn *self)
{
  if (self->header)
    bobgui_column_view_title_update_sort (BOBGUI_COLUMN_VIEW_TITLE (self->header));
}

/**
 * bobgui_column_view_column_set_visible:
 * @self: a column
 * @visible: whether this column should be visible
 *
 * Sets whether this column should be visible in views.
 */
void
bobgui_column_view_column_set_visible (BobguiColumnViewColumn *self,
                                    gboolean             visible)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self));

  if (self->visible == visible)
    return;

  self->visible = visible;

  self->minimum_size_request = -1;
  self->natural_size_request = -1;

  if (self->header)
    bobgui_widget_set_visible (BOBGUI_WIDGET (self->header), visible);

  bobgui_column_view_column_ensure_cells (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_VISIBLE]);
}

/**
 * bobgui_column_view_column_get_visible:
 * @self: a column
 *
 * Returns whether this column is visible.
 *
 * Returns: true if this column is visible
 */
gboolean
bobgui_column_view_column_get_visible (BobguiColumnViewColumn *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self), TRUE);

  return self->visible;
}

/**
 * bobgui_column_view_column_set_header_menu:
 * @self: a column
 * @menu: (nullable): a `GMenuModel`
 *
 * Sets the menu model that is used to create the context menu
 * for the column header.
 */
void
bobgui_column_view_column_set_header_menu (BobguiColumnViewColumn *self,
                                        GMenuModel          *menu)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self));
  g_return_if_fail (menu == NULL || G_IS_MENU_MODEL (menu));

  if (!g_set_object (&self->menu, menu))
    return;

  if (self->header)
    bobgui_column_view_title_set_menu (BOBGUI_COLUMN_VIEW_TITLE (self->header), menu);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HEADER_MENU]);
}

/**
 * bobgui_column_view_column_get_header_menu:
 * @self: a column
 *
 * Gets the menu model that is used to create the context menu
 * for the column header.
 *
 * Returns: (transfer none) (nullable): the `GMenuModel`
 */
GMenuModel *
bobgui_column_view_column_get_header_menu (BobguiColumnViewColumn *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self), NULL);

  return self->menu;
}

/**
 * bobgui_column_view_column_set_expand:
 * @self: a column
 * @expand: whether this column should expand to fill available space
 *
 * Sets the column to take available extra space.
 *
 * The extra space is shared equally amongst all columns that
 * have are set to expand.
 */
void
bobgui_column_view_column_set_expand (BobguiColumnViewColumn *self,
                                   gboolean             expand)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self));

  if (self->expand == expand)
    return;

  self->expand = expand;

  if (self->visible && self->view)
    bobgui_widget_queue_resize (BOBGUI_WIDGET (self->view));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_EXPAND]);
}

/**
 * bobgui_column_view_column_get_expand:
 * @self: a column
 *
 * Returns whether this column should expand.
 *
 * Returns: true if this column expands
 */
gboolean
bobgui_column_view_column_get_expand (BobguiColumnViewColumn *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self), TRUE);

  return self->expand;
}

/**
 * bobgui_column_view_column_set_resizable:
 * @self: a column
 * @resizable: whether this column should be resizable
 *
 * Sets whether this column should be resizable by dragging.
 */
void
bobgui_column_view_column_set_resizable (BobguiColumnViewColumn *self,
                                      gboolean             resizable)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self));

  if (self->resizable == resizable)
    return;

  self->resizable = resizable;

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_RESIZABLE]);
}

/**
 * bobgui_column_view_column_get_resizable:
 * @self: a column
 *
 * Returns whether this column is resizable.
 *
 * Returns: true if this column is resizable
 */
gboolean
bobgui_column_view_column_get_resizable (BobguiColumnViewColumn *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self), TRUE);

  return self->resizable;
}

/**
 * bobgui_column_view_column_set_fixed_width:
 * @self: a column
 * @fixed_width: the new fixed width, or -1
 *
 * Sets the fixed width of the column.
 *
 * If @fixed_width is -1, the fixed width of the column is unset.
 *
 * Setting a fixed width overrides the automatically calculated
 * width. Interactive resizing also sets the “fixed-width” property.
 */
void
bobgui_column_view_column_set_fixed_width (BobguiColumnViewColumn *self,
                                        int                  fixed_width)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self));
  g_return_if_fail (fixed_width >= -1);

  if (self->fixed_width == fixed_width)
    return;

  self->fixed_width = fixed_width;

  bobgui_column_view_column_queue_resize (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FIXED_WIDTH]);
}

/**
 * bobgui_column_view_column_get_fixed_width:
 * @self: a column
 *
 * Gets the fixed width of the column.
 *
 * Returns: the fixed with of the column
 */
int
bobgui_column_view_column_get_fixed_width (BobguiColumnViewColumn *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self), -1);

  return self->fixed_width;
}

BobguiWidget *
bobgui_column_view_column_get_header (BobguiColumnViewColumn *self)
{
  return self->header;
}

void
bobgui_column_view_column_set_header_position (BobguiColumnViewColumn *self,
                                            int                  offset)
{
  self->header_position = offset;
}

void
bobgui_column_view_column_get_header_allocation (BobguiColumnViewColumn *self,
                                              int                 *offset,
                                              int                 *size)
{
  if (offset)
    *offset = self->header_position;

  if (size)
    *size = self->allocation_size;
}

/**
 * bobgui_column_view_column_set_id:
 * @self: a column
 * @id: (nullable): ID to use for this column
 *
 * Sets the id of this column.
 *
 * BOBGUI makes no use of this, but applications can use it when
 * storing column view configuration.
 *
 * It is up to callers to ensure uniqueness of IDs.
 *
 * Since: 4.10
 */
void
bobgui_column_view_column_set_id (BobguiColumnViewColumn *self,
                               const char          *id)
{
  g_return_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self));

  if (g_strcmp0 (self->id, id) == 0)
    return;

  g_free (self->id);
  self->id = g_strdup (id);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ID]);
}

/**
 * bobgui_column_view_column_get_id:
 * @self: a column
 *
 * Returns the ID set with [method@Bobgui.ColumnViewColumn.set_id].
 *
 * Returns: (nullable): The column's ID
 *
 * Since: 4.10
 */
const char *
bobgui_column_view_column_get_id (BobguiColumnViewColumn *self)
{
  g_return_val_if_fail (BOBGUI_IS_COLUMN_VIEW_COLUMN (self), NULL);

  return self->id;
}
