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

#include "bobguigridview.h"

#include "bobguibitset.h"
#include "bobguilistbaseprivate.h"
#include "bobguilistitemfactory.h"
#include "bobguilistitemmanagerprivate.h"
#include "bobguilistitemwidgetprivate.h"
#include "bobguimultiselection.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"

/* Maximum number of list items created by the gridview.
 * For debugging, you can set this to G_MAXUINT to ensure
 * there's always a list item for every row.
 *
 * We multiply this number with BobguiGridView:max-columns so
 * that we can always display at least this many rows.
 */
#define BOBGUI_GRID_VIEW_MAX_VISIBLE_ROWS (30)

#define DEFAULT_MAX_COLUMNS (7)

/**
 * BobguiGridView:
 *
 * Presents a large dynamic grid of items.
 *
 * `BobguiGridView` uses its factory to generate one child widget for each
 * visible item and shows them in a grid. The orientation of the grid view
 * determines if the grid reflows vertically or horizontally.
 *
 * `BobguiGridView` allows the user to select items according to the selection
 * characteristics of the model. For models that allow multiple selected items,
 * it is possible to turn on _rubberband selection_, using
 * [property@Bobgui.GridView:enable-rubberband].
 *
 * To learn more about the list widget framework, see the
 * [overview](section-list-widget.html).
 *
 * # Actions
 *
 * `BobguiGridView` defines a set of built-in actions:
 *
 * - `list.activate-item` activates the item at given position by emitting the
 *   the [signal@Bobgui.GridView::activate] signal.
 *
 * # CSS nodes
 *
 * ```
 * gridview
 * ├── child[.activatable]
 * │
 * ├── child[.activatable]
 * │
 * ┊
 * ╰── [rubberband]
 * ```
 *
 * `BobguiGridView` uses a single CSS node with name `gridview`. Each child uses
 * a single CSS node with name `child`. If the [property@Bobgui.ListItem:activatable]
 * property is set, the corresponding row will have the `.activatable` style
 * class. For rubberband selection, a subnode with name `rubberband` is used.
 *
 * # Accessibility
 *
 * `BobguiGridView` uses the [enum@Bobgui.AccessibleRole.grid] role, and the items
 * use the [enum@Bobgui.AccessibleRole.grid_cell] role.
 */

struct _BobguiGridView
{
  BobguiListBase parent_instance;

  BobguiListItemManager *item_manager;
  BobguiListItemFactory *factory;
  guint min_columns;
  guint max_columns;
  gboolean single_click_activate;
  /* set in size_allocate */
  guint n_columns;
  double column_width;
};

struct _BobguiGridViewClass
{
  BobguiListBaseClass parent_class;
};

enum
{
  PROP_0,
  PROP_ENABLE_RUBBERBAND,
  PROP_FACTORY,
  PROP_MAX_COLUMNS,
  PROP_MIN_COLUMNS,
  PROP_MODEL,
  PROP_SINGLE_CLICK_ACTIVATE,
  PROP_TAB_BEHAVIOR,

  N_PROPS
};

enum {
  ACTIVATE,
  LAST_SIGNAL
};

G_DEFINE_TYPE (BobguiGridView, bobgui_grid_view, BOBGUI_TYPE_LIST_BASE)

static GParamSpec *properties[N_PROPS] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

static void G_GNUC_UNUSED
dump (BobguiGridView *self)
{
  BobguiListTile *tile;
  guint n_widgets, n_list_rows, n_items;

  n_widgets = 0;
  n_list_rows = 0;
  n_items = 0;
  //g_print ("ANCHOR: %u - %u\n", self->anchor_start, self->anchor_end);
  for (tile = bobgui_list_item_manager_get_first (self->item_manager);
       tile;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      if (tile->widget)
        n_widgets++;
      n_list_rows++;
      n_items += tile->n_items;
      g_print ("%6u%6u %5ux%3u %s (%d,%d,%d,%d)\n",
               tile->n_items, n_items,
               n_items / (self->n_columns ? self->n_columns : self->min_columns),
               n_items % (self->n_columns ? self->n_columns : self->min_columns),
               tile->widget ? " (widget)" : "",
               tile->area.x, tile->area.y, tile->area.width, tile->area.height);
    }

  g_print ("  => %u widgets in %u list rows\n", n_widgets, n_list_rows);
}

static int
column_index (BobguiGridView *self,
              int          spacing,
              int          x)
{
  return (x + spacing / 2.0) / (self->column_width + spacing);
}

static int
column_start (BobguiGridView *self,
              int          spacing,
              int          col)
{
  return ceil ((self->column_width + spacing) * col);
}

static int
column_end (BobguiGridView *self,
            int          spacing,
            int          col)
{
  return ceil (self->column_width * (col + 1) + (spacing * col));
}

static BobguiListTile *
bobgui_grid_view_split (BobguiListBase *base,
                     BobguiListTile *tile,
                     guint        n_items)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (base);
  BobguiListTile *split;
  guint col, row_height;
  int xspacing, yspacing;

  bobgui_list_base_get_border_spacing (base, &xspacing, &yspacing);

  row_height = (tile->area.height + yspacing) / MAX (tile->n_items / self->n_columns, 1) - yspacing;

  /* split off the multirow at the top */
  if (n_items >= self->n_columns)
    {
      guint top_rows = n_items / self->n_columns;
      guint top_items = top_rows * self->n_columns;

      split = tile;
      tile = bobgui_list_tile_split (self->item_manager, tile, top_items);
      bobgui_list_tile_set_area (self->item_manager,
                              tile,
                              &(GdkRectangle) {
                                split->area.x,
                                split->area.y + (row_height + yspacing) * top_rows,
                                split->area.width,
                                split->area.height - (row_height + yspacing) * top_rows,
                              });
      bobgui_list_tile_set_area_size (self->item_manager,
                                   split,
                                   split->area.width,
                                   row_height * top_rows + yspacing * (top_rows - 1));
      n_items -= top_items;
      if (n_items == 0)
        return tile;
    }

  /* split off the multirow at the bottom */
  if (tile->n_items > self->n_columns)
    {
      split = bobgui_list_tile_split (self->item_manager, tile, self->n_columns);
      bobgui_list_tile_set_area (self->item_manager,
                              split,
                              &(GdkRectangle) {
                                tile->area.x,
                                tile->area.y + row_height + yspacing,
                                tile->area.width,
                                tile->area.height - row_height - yspacing,
                              });
      bobgui_list_tile_set_area_size (self->item_manager,
                                   tile,
                                   tile->area.width,
                                   row_height);
    }

  g_assert (n_items < tile->n_items);
  g_assert (tile->n_items <= self->n_columns);

  /* now it's a single row, do a split at the column boundary */
  col = column_index (self, xspacing, tile->area.x);
  split = bobgui_list_tile_split (self->item_manager, tile, n_items);
  bobgui_list_tile_set_area (self->item_manager,
                          split,
                          &(GdkRectangle) {
                            column_start (self, xspacing, col + n_items),
                            tile->area.y,
                            column_end (self, xspacing, col + n_items + split->n_items - 1)
                            - column_start (self, xspacing, col + n_items),
                            tile->area.height,
                          });
  bobgui_list_tile_set_area_size (self->item_manager,
                               tile,
                               column_end (self, xspacing, col + n_items - 1) - tile->area.x,
                               tile->area.height);

  return split;
}

/* We define the listview as **inert** when the factory isn't used. */
static gboolean
bobgui_grid_view_is_inert (BobguiGridView *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  return !bobgui_widget_get_visible (widget) ||
         bobgui_widget_get_root (widget) == NULL;
}

static void
bobgui_grid_view_update_factories_with (BobguiGridView        *self,
                                     BobguiListItemFactory *factory)
{
  BobguiListTile *tile;

  for (tile = bobgui_list_item_manager_get_first (self->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      if (tile->widget)
        bobgui_list_factory_widget_set_factory (BOBGUI_LIST_FACTORY_WIDGET (tile->widget), factory);
    }
}

static void
bobgui_grid_view_update_factories (BobguiGridView *self)
{
  bobgui_grid_view_update_factories_with (self,
                                       bobgui_grid_view_is_inert (self) ? NULL : self->factory);
}

static void
bobgui_grid_view_clear_factories (BobguiGridView *self)
{
  bobgui_grid_view_update_factories_with (self, NULL);
}

static BobguiListItemBase *
bobgui_grid_view_create_list_widget (BobguiListBase *base)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (base);
  BobguiListItemFactory *factory;
  BobguiWidget *result;

  if (bobgui_grid_view_is_inert (self))
    factory = NULL;
  else
    factory = self->factory;

  result = bobgui_list_item_widget_new (factory,
                                     "child",
                                     BOBGUI_ACCESSIBLE_ROLE_GRID_CELL);

  bobgui_list_factory_widget_set_single_click_activate (BOBGUI_LIST_FACTORY_WIDGET (result), self->single_click_activate);

  return BOBGUI_LIST_ITEM_BASE (result);
}

static gboolean
bobgui_grid_view_get_allocation (BobguiListBase  *base,
                              guint         pos,
                              GdkRectangle *area)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (base);
  BobguiListTile *tile;
  guint offset;
  int xspacing, yspacing;

  tile = bobgui_list_item_manager_get_nth (self->item_manager, pos, &offset);
  if (tile == NULL)
    return FALSE;

  bobgui_list_base_get_border_spacing (base, &xspacing, &yspacing);

  if (tile->area.width <= 0 || tile->area.height <= 0)
    {
      /* item is not allocated yet */
      BobguiListTile *other;

      for (other = bobgui_rb_tree_node_get_previous (tile);
           other;
           other = bobgui_rb_tree_node_get_previous (other))
        {
          if (other->area.width || other->area.height)
            {
              area->x = other->area.x + other->area.width;
              area->y = other->area.y + other->area.height;
              break;
            }
        }
      if (other == NULL)
        {
          for (other = bobgui_rb_tree_node_get_next (tile);
               other;
               other = bobgui_rb_tree_node_get_next (other))
            {
              if (other->area.width || other->area.height)
                {
                  area->x = other->area.x;
                  area->y = other->area.y;
                  break;
                }
            }
        }
      return TRUE;
    }

  *area = tile->area;

  if (tile->n_items > self->n_columns)
    {
      area->height = (area->height + yspacing) / (tile->n_items / self->n_columns) - yspacing;
      area->y += (offset / self->n_columns) * (area->height + yspacing);
      offset %= self->n_columns;
    }

  if (tile->n_items > 1)
    {
      guint col = column_index (self, xspacing, area->x);
      area->x = column_start (self, xspacing, col + offset);
      area->width = column_end (self, xspacing, col + offset) - area->x;
    }

  return TRUE;
}

static gboolean
bobgui_grid_view_get_position_from_allocation (BobguiListBase           *base,
                                            int                    x,
                                            int                    y,
                                            guint                 *position,
                                            cairo_rectangle_int_t *area)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (base);
  BobguiListTile *tile;
  guint pos;

  tile = bobgui_list_item_manager_get_nearest_tile (self->item_manager, x, y);
  if (tile == NULL)
    return FALSE;

  while (tile && tile->n_items == 0)
    tile = bobgui_rb_tree_node_get_previous (tile);
  if (tile == NULL)
    {
      tile = bobgui_list_item_manager_get_first (self->item_manager);
      while (tile && tile->n_items == 0)
        tile = bobgui_rb_tree_node_get_next (tile);
      if (tile == NULL)
        return FALSE;
    }

  pos = bobgui_list_tile_get_position (self->item_manager, tile);
  if (tile->n_items > 1 && tile->area.width > 0 && tile->area.height > 0)
    {
      int xspacing, yspacing;

      bobgui_list_base_get_border_spacing (base, &xspacing, &yspacing);

      /* offset in x direction */
      pos += column_index (self, xspacing, MAX (tile->area.width - 1, x - tile->area.x));
      if (area)
        {
          guint col = MIN (column_index (self, xspacing, x), self->n_columns - 1);
          area->x = column_start (self, xspacing, col);
          area->width = column_end (self, xspacing, col) - area->x;
        }

      /* offset in y direction */
      if (tile->n_items > self->n_columns)
        {
          guint rows_in_tile = tile->n_items / self->n_columns;
          guint row_height = (tile->area.height + yspacing) / rows_in_tile - yspacing;
          guint row_index = MIN (tile->area.height - 1, y - tile->area.y) / (row_height + yspacing);
          pos += self->n_columns * row_index;

          if (area)
            {
              area->y = tile->area.y  + row_index * (row_height + yspacing);
              area->height = row_height;
            }
        }
      else
        {
          if (area)
            {
              area->y = tile->area.y;
              area->height = tile->area.height;
            }
        }

    }
  else
    {
      if (area)
        *area = tile->area;
    }

  *position = pos;

  return TRUE;
}

static BobguiBitset *
bobgui_grid_view_get_items_in_rect (BobguiListBase        *base,
                                 const GdkRectangle *rect)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (base);
  guint first_row, last_row, first_column, last_column;
  cairo_rectangle_int_t area;
  int xspacing, yspacing;
  BobguiBitset *result;

  bobgui_list_base_get_border_spacing (base, &xspacing, &yspacing);
  result = bobgui_bitset_new_empty ();

  first_column = MAX (column_index (self, xspacing, rect->x), 0);
  if (column_end (self, xspacing, first_column) <= rect->x)
    first_column++;
  last_column = MIN (column_index (self, xspacing, rect->x + rect->width), self->n_columns - 1);
  if (column_start (self, xspacing, last_column) > rect->x + rect->width)
    last_column--;
  /* match y = 0 here because we care about the rows, not the cells */
  if (!bobgui_grid_view_get_position_from_allocation (base, column_start (self, xspacing, 0), rect->y, &first_row, &area))
    g_return_val_if_reached (result);
  if (area.y + area.height < rect->y)
    first_row += self->n_columns;
  if (!bobgui_grid_view_get_position_from_allocation (base, column_start (self, xspacing, 0), rect->y + rect->height, &last_row, NULL))
    g_return_val_if_reached (result);
  if (area.y >= rect->y + rect->height)
    last_row -= self->n_columns;

  if (first_column <= last_column && first_row <= last_row)
    {
      bobgui_bitset_add_rectangle (result,
                                first_row + first_column,
                                last_column - first_column + 1,
                                (last_row - first_row) / self->n_columns + 1,
                                self->n_columns);
    }

  return result;
}

static guint
bobgui_grid_view_move_focus_along (BobguiListBase *base,
                                guint        pos,
                                int          steps)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (base);

  steps *= self->n_columns;

  if (steps < 0)
    {
      if (pos >= self->n_columns)
        pos -= MIN (pos, -steps);
    }
  else
    {
      guint n_items = bobgui_list_base_get_n_items (base);
      if (n_items / self->n_columns > pos / self->n_columns)
        pos += MIN (n_items - pos - 1, steps);
    }

  return pos;
}

static guint
bobgui_grid_view_move_focus_across (BobguiListBase *base,
                                 guint        pos,
                                 int          steps)
{
  if (steps < 0)
    return pos - MIN (pos, -steps);
  else
    {
      guint n_items = bobgui_list_base_get_n_items (base);
      pos += MIN (n_items - pos - 1, steps);
    }

  return pos;
}

static int
compare_ints (gconstpointer first,
              gconstpointer second)
{
  return *(int *) first - *(int *) second;
}

static int
bobgui_grid_view_get_unknown_row_size (BobguiGridView *self,
                                    GArray      *heights)
{
  g_return_val_if_fail (heights->len > 0, 0);

  /* return the median and hope rows are generally uniform with few outliers */
  g_array_sort (heights, compare_ints);

  return g_array_index (heights, int, heights->len / 2);
}

static void
bobgui_grid_view_measure_column_size (BobguiGridView *self,
                                   int         *minimum,
                                   int         *natural)
{
  BobguiOrientation opposite;
  BobguiListTile *tile;
  int min, nat, child_min, child_nat;

  min = 0;
  nat = 0;
  opposite = bobgui_list_base_get_opposite_orientation (BOBGUI_LIST_BASE (self));

  for (tile = bobgui_list_item_manager_get_first (self->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      if (tile->widget == NULL)
        continue;

      bobgui_widget_measure (tile->widget,
                          opposite, -1,
                          &child_min, &child_nat, NULL, NULL);
      min = MAX (min, child_min);
      nat = MAX (nat, child_nat);
    }

  *minimum = min;
  *natural = nat;
}

static void
bobgui_grid_view_measure_across (BobguiWidget *widget,
                              int        for_size,
                              int       *minimum,
                              int       *natural)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (widget);
  int xspacing;

  bobgui_list_base_get_border_spacing (BOBGUI_LIST_BASE (widget), &xspacing, NULL);

  bobgui_grid_view_measure_column_size (self, minimum, natural);

  *minimum = (*minimum + xspacing) * self->min_columns - xspacing;
  *natural = (*natural + xspacing) * self->max_columns - xspacing;
}

static guint
bobgui_grid_view_compute_n_columns (BobguiGridView *self,
                                 guint        for_size,
                                 int          border_spacing,
                                 int          min,
                                 int          nat)
{
  guint n_columns;

  /* rounding down is exactly what we want here, so int division works */
  if (bobgui_list_base_get_scroll_policy (BOBGUI_LIST_BASE (self),
                                       bobgui_list_base_get_opposite_orientation (BOBGUI_LIST_BASE (self))) == BOBGUI_SCROLL_MINIMUM)
    n_columns = (for_size + border_spacing) / MAX (1, min + border_spacing);
  else
    n_columns = (for_size + border_spacing) / MAX (1, nat + border_spacing);

  n_columns = CLAMP (n_columns, self->min_columns, self->max_columns);

  g_assert (n_columns > 0);

  return n_columns;
}

static void
bobgui_grid_view_measure_list (BobguiWidget *widget,
                            int        for_size,
                            int       *minimum,
                            int       *natural)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (widget);
  BobguiScrollablePolicy scroll_policy;
  BobguiListTile *tile;
  int height, row_height, child_min, child_nat, column_size, col_min, col_nat;
  int xspacing, yspacing;
  gboolean measured;
  GArray *heights;
  guint n_unknown, n_columns;
  guint i;

  bobgui_list_base_get_border_spacing (BOBGUI_LIST_BASE (self), &xspacing, &yspacing);
  scroll_policy = bobgui_list_base_get_scroll_policy (BOBGUI_LIST_BASE (self), bobgui_list_base_get_orientation (BOBGUI_LIST_BASE (self)));
  heights = g_array_new (FALSE, FALSE, sizeof (int));
  n_unknown = 0;
  height = 0;

  bobgui_grid_view_measure_column_size (self, &col_min, &col_nat);
  for_size = MAX (for_size, col_min * (int) self->min_columns);
  n_columns = bobgui_grid_view_compute_n_columns (self, for_size, xspacing, col_min, col_nat);
  column_size = (for_size + xspacing) / n_columns - xspacing;

  i = 0;
  row_height = 0;
  measured = FALSE;
  for (tile = bobgui_list_item_manager_get_first (self->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      if (tile->widget)
        {
          bobgui_widget_measure (tile->widget,
                              bobgui_list_base_get_orientation (BOBGUI_LIST_BASE (self)),
                              column_size,
                              &child_min, &child_nat, NULL, NULL);
          if (scroll_policy == BOBGUI_SCROLL_MINIMUM)
            row_height = MAX (row_height, child_min);
          else
            row_height = MAX (row_height, child_nat);
          measured = TRUE;
        }

      i += tile->n_items;

      if (i >= n_columns)
        {
          if (measured)
            {
              g_array_append_val (heights, row_height);
              i -= n_columns;
              height += row_height + yspacing;
              measured = FALSE;
              row_height = 0;
            }
          n_unknown += i / n_columns;
          i %= n_columns;
        }
    }

  if (i > 0)
    {
      if (measured)
        {
          g_array_append_val (heights, row_height);
          height += row_height + yspacing;
        }
      else
        n_unknown++;
    }

  if (n_unknown)
    height += n_unknown * (bobgui_grid_view_get_unknown_row_size (self, heights) + yspacing);
  /* if we have a height, we have at least one row, and because we added spacing for every row... */
  if (height)
    height -= yspacing;

  g_array_free (heights, TRUE);

  *minimum = height;
  *natural = height;
}

static void
bobgui_grid_view_measure (BobguiWidget      *widget,
                       BobguiOrientation  orientation,
                       int             for_size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (widget);

  if (orientation == bobgui_list_base_get_orientation (BOBGUI_LIST_BASE (self)))
    bobgui_grid_view_measure_list (widget, for_size, minimum, natural);
  else
    bobgui_grid_view_measure_across (widget, for_size, minimum, natural);
}

static void
bobgui_grid_view_size_allocate (BobguiWidget *widget,
                             int        width,
                             int        height,
                             int        baseline)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (widget);
  BobguiListTile *tile, *start, *footer;
  GArray *heights;
  int min_row_height, unknown_row_height, row_height, col_min, col_nat;
  BobguiOrientation orientation;
  BobguiScrollablePolicy scroll_policy;
  int y, xspacing, yspacing;
  guint i;

  orientation = bobgui_list_base_get_orientation (BOBGUI_LIST_BASE (self));
  scroll_policy = bobgui_list_base_get_scroll_policy (BOBGUI_LIST_BASE (self), orientation);
  min_row_height = ceil ((double) height / BOBGUI_GRID_VIEW_MAX_VISIBLE_ROWS);
  bobgui_list_base_get_border_spacing (BOBGUI_LIST_BASE (self), &xspacing, &yspacing);

  bobgui_list_item_manager_gc_tiles (self->item_manager);

  /* step 0: exit early if list is empty */
  tile = bobgui_list_item_manager_get_first (self->item_manager);
  if (tile == NULL)
    {
      bobgui_list_base_allocate (BOBGUI_LIST_BASE (self));
      return;
    }

  /* step 1: determine width of the list */
  bobgui_grid_view_measure_column_size (self, &col_min, &col_nat);
  self->n_columns = bobgui_grid_view_compute_n_columns (self,
                                                     orientation == BOBGUI_ORIENTATION_VERTICAL ? width : height,
                                                     xspacing,
                                                     col_min, col_nat);
  self->column_width = ((orientation == BOBGUI_ORIENTATION_VERTICAL ? width : height) + xspacing) / self->n_columns - xspacing;
  self->column_width = MAX (self->column_width, col_min);

  /* step 2: determine height of known rows */
  heights = g_array_new (FALSE, FALSE, sizeof (int));

  while (tile != NULL)
    {
      /* if it's a multirow tile, handle it here */
      if (tile->n_items > 1 && tile->n_items >= self->n_columns)
        {
          if (tile->n_items % self->n_columns)
            bobgui_list_tile_split (self->item_manager, tile, tile->n_items / self->n_columns * self->n_columns);
          tile = bobgui_rb_tree_node_get_next (tile);
          continue;
        }

      /* Not a multirow tile */
      i = 0;
      row_height = 0;

      for (i = 0, start = tile;
           i < self->n_columns && tile != NULL;
           tile = bobgui_rb_tree_node_get_next (tile))
        {
          if (tile->widget)
            {
              int min, nat, size;
              bobgui_widget_measure (tile->widget,
                                  bobgui_list_base_get_orientation (BOBGUI_LIST_BASE (self)),
                                  self->column_width,
                                  &min, &nat, NULL, NULL);
              if (scroll_policy == BOBGUI_SCROLL_MINIMUM)
                size = min;
              else
                size = nat;
              size = MAX (size, min_row_height);
              g_array_append_val (heights, size);
              row_height = MAX (row_height, size);
            }
          if (tile->n_items > self->n_columns - i)
            bobgui_list_tile_split (self->item_manager, tile, self->n_columns - i);
          i += tile->n_items;
        }
      if (row_height > 0)
        {
          for (i = 0;
               start != tile;
               start = bobgui_rb_tree_node_get_next (start))
            {
              bobgui_list_tile_set_area_size (self->item_manager,
                                           start,
                                           column_end (self, xspacing, i + start->n_items - 1)
                                           - column_start (self, xspacing, i),
                                           row_height);
              i += start->n_items;
            }
          g_assert (i <= self->n_columns);
        }
    }

  /* step 3: determine height of rows with only unknown items */
  unknown_row_height = bobgui_grid_view_get_unknown_row_size (self, heights);
  g_array_free (heights, TRUE);

  /* step 4: determine height for remaining rows and set each row's position */
  y = 0;
  i = 0;
  for (tile = bobgui_list_item_manager_get_first (self->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      bobgui_list_tile_set_area_position (self->item_manager,
                                       tile,
                                       column_start (self, xspacing, i),
                                       y);
      if (tile->n_items >= self->n_columns && tile->widget == NULL)
        {
          g_assert (i == 0);
          g_assert (tile->n_items % self->n_columns == 0);
          bobgui_list_tile_set_area_size (self->item_manager,
                                       tile,
                                       column_end (self, xspacing, self->n_columns - 1)
                                       - column_start (self, xspacing, 0),
                                       (unknown_row_height + yspacing) * (tile->n_items / self->n_columns) - yspacing);
          y += tile->area.height + yspacing;
        }
      else
        {
          if (tile->area.height == 0)
            {
              /* this case is for the last row - it may not be a full row so it won't
               * be a multirow tile but it may have no widgets either */
              bobgui_list_tile_set_area_size (self->item_manager,
                                           tile,
                                           column_end (self, xspacing, i + tile->n_items - 1) - tile->area.x,
                                           unknown_row_height);
            }
          i += tile->n_items;
        }

      if (i >= self->n_columns)
        {
          g_assert (i == self->n_columns);
          y += tile->area.height + yspacing;
          i = 0;
        }
    }
  footer = bobgui_list_item_manager_get_last (self->item_manager);
  g_assert (bobgui_list_tile_is_footer (footer));
  /* Make the footer tile fill the empty space in the bottom right */
  if (i > 0)
    {
      tile = bobgui_rb_tree_node_get_previous (footer);
      bobgui_list_tile_set_area_position (self->item_manager,
                                       footer,
                                       column_start (self, xspacing, i),
                                       y);
      bobgui_list_tile_set_area_size (self->item_manager,
                                   footer,
                                   column_end (self, xspacing, self->n_columns - 1) - footer->area.x,
                                   tile->area.height);
    }
  else
    {
      bobgui_list_tile_set_area_size (self->item_manager,
                                   footer,
                                   0,
                                   0);
    }

  /* step 5: allocate the rest */
  bobgui_list_base_allocate (BOBGUI_LIST_BASE (self));
}

static void
bobgui_grid_view_root (BobguiWidget *widget)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (widget);

  BOBGUI_WIDGET_CLASS (bobgui_grid_view_parent_class)->root (widget);

  if (!bobgui_grid_view_is_inert (self))
    bobgui_grid_view_update_factories (self);
}

static void
bobgui_grid_view_unroot (BobguiWidget *widget)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (widget);

  if (!bobgui_grid_view_is_inert (self))
    bobgui_grid_view_clear_factories (self);

  BOBGUI_WIDGET_CLASS (bobgui_grid_view_parent_class)->unroot (widget);
}

static void
bobgui_grid_view_show (BobguiWidget *widget)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (widget);

  BOBGUI_WIDGET_CLASS (bobgui_grid_view_parent_class)->show (widget);

  if (!bobgui_grid_view_is_inert (self))
    bobgui_grid_view_update_factories (self);
}

static void
bobgui_grid_view_hide (BobguiWidget *widget)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (widget);

  if (!bobgui_grid_view_is_inert (self))
    bobgui_grid_view_clear_factories (self);

  BOBGUI_WIDGET_CLASS (bobgui_grid_view_parent_class)->hide (widget);
}

static void
bobgui_grid_view_dispose (GObject *object)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (object);

  self->item_manager = NULL;

  g_clear_object (&self->factory);

  G_OBJECT_CLASS (bobgui_grid_view_parent_class)->dispose (object);
}

static void
bobgui_grid_view_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (object);

  switch (property_id)
    {
    case PROP_ENABLE_RUBBERBAND:
      g_value_set_boolean (value, bobgui_list_base_get_enable_rubberband (BOBGUI_LIST_BASE (self)));
      break;

    case PROP_FACTORY:
      g_value_set_object (value, self->factory);
      break;

    case PROP_MAX_COLUMNS:
      g_value_set_uint (value, self->max_columns);
      break;

    case PROP_MIN_COLUMNS:
      g_value_set_uint (value, self->min_columns);
      break;

    case PROP_MODEL:
      g_value_set_object (value, bobgui_list_base_get_model (BOBGUI_LIST_BASE (self)));
      break;

    case PROP_SINGLE_CLICK_ACTIVATE:
      g_value_set_boolean (value, self->single_click_activate);
      break;

    case PROP_TAB_BEHAVIOR:
      g_value_set_enum (value, bobgui_list_base_get_tab_behavior (BOBGUI_LIST_BASE (self)));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_grid_view_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (object);

  switch (property_id)
    {
    case PROP_ENABLE_RUBBERBAND:
      bobgui_grid_view_set_enable_rubberband (self, g_value_get_boolean (value));
      break;

    case PROP_FACTORY:
      bobgui_grid_view_set_factory (self, g_value_get_object (value));
      break;

    case PROP_MAX_COLUMNS:
      bobgui_grid_view_set_max_columns (self, g_value_get_uint (value));
      break;

    case PROP_MIN_COLUMNS:
      bobgui_grid_view_set_min_columns (self, g_value_get_uint (value));
      break;

    case PROP_MODEL:
      bobgui_grid_view_set_model (self, g_value_get_object (value));
      break;

    case PROP_SINGLE_CLICK_ACTIVATE:
      bobgui_grid_view_set_single_click_activate (self, g_value_get_boolean (value));
      break;

    case PROP_TAB_BEHAVIOR:
      bobgui_grid_view_set_tab_behavior (self, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_grid_view_activate_item (BobguiWidget  *widget,
                             const char *action_name,
                             GVariant   *parameter)
{
  BobguiGridView *self = BOBGUI_GRID_VIEW (widget);
  guint pos;

  if (!g_variant_check_format_string (parameter, "u", FALSE))
    return;

  g_variant_get (parameter, "u", &pos);
  if (pos >= bobgui_list_base_get_n_items (BOBGUI_LIST_BASE (self)))
    return;

  g_signal_emit (widget, signals[ACTIVATE], 0, pos);
}

static void
bobgui_grid_view_class_init (BobguiGridViewClass *klass)
{
  BobguiListBaseClass *list_base_class = BOBGUI_LIST_BASE_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  list_base_class->split = bobgui_grid_view_split;
  list_base_class->create_list_widget = bobgui_grid_view_create_list_widget;
  list_base_class->get_allocation = bobgui_grid_view_get_allocation;
  list_base_class->get_items_in_rect = bobgui_grid_view_get_items_in_rect;
  list_base_class->get_position_from_allocation = bobgui_grid_view_get_position_from_allocation;
  list_base_class->move_focus_along = bobgui_grid_view_move_focus_along;
  list_base_class->move_focus_across = bobgui_grid_view_move_focus_across;

  widget_class->measure = bobgui_grid_view_measure;
  widget_class->size_allocate = bobgui_grid_view_size_allocate;
  widget_class->root = bobgui_grid_view_root;
  widget_class->unroot = bobgui_grid_view_unroot;
  widget_class->show = bobgui_grid_view_show;
  widget_class->hide = bobgui_grid_view_hide;

  gobject_class->dispose = bobgui_grid_view_dispose;
  gobject_class->get_property = bobgui_grid_view_get_property;
  gobject_class->set_property = bobgui_grid_view_set_property;

  /**
   * BobguiGridView:enable-rubberband:
   *
   * Allow rubberband selection.
   */
  properties[PROP_ENABLE_RUBBERBAND] =
    g_param_spec_boolean ("enable-rubberband", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGridView:factory:
   *
   * Factory for populating list items.
   *
   * The factory must be for configuring [class@Bobgui.ListItem] objects.
   */
  properties[PROP_FACTORY] =
    g_param_spec_object ("factory", NULL, NULL,
                         BOBGUI_TYPE_LIST_ITEM_FACTORY,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);


  /**
   * BobguiGridView:max-columns:
   *
   * Maximum number of columns per row.
   *
   * If this number is smaller than [property@Bobgui.GridView:min-columns],
   * that value is used instead.
   */
  properties[PROP_MAX_COLUMNS] =
    g_param_spec_uint ("max-columns", NULL, NULL,
                       1, G_MAXUINT, DEFAULT_MAX_COLUMNS,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiGridView:min-columns:
   *
   * Minimum number of columns per row.
   */
  properties[PROP_MIN_COLUMNS] =
    g_param_spec_uint ("min-columns", NULL, NULL,
                       1, G_MAXUINT, 1,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiGridView:model:
   *
   * Model for the items displayed.
   */
  properties[PROP_MODEL] =
    g_param_spec_object ("model", NULL, NULL,
                         BOBGUI_TYPE_SELECTION_MODEL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiGridView:single-click-activate:
   *
   * Activate rows on single click and select them on hover.
   */
  properties[PROP_SINGLE_CLICK_ACTIVATE] =
    g_param_spec_boolean ("single-click-activate", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiGridView:tab-behavior:
   *
   * Behavior of the <kbd>Tab</kbd> key
   *
   * Since: 4.12
   */
  properties[PROP_TAB_BEHAVIOR] =
    g_param_spec_enum ("tab-behavior", NULL, NULL,
                       BOBGUI_TYPE_LIST_TAB_BEHAVIOR,
                       BOBGUI_LIST_TAB_ALL,
                       G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  g_object_class_install_properties (gobject_class, N_PROPS, properties);

  /**
   * BobguiGridView::activate:
   * @self: The `BobguiGridView`
   * @position: position of item to activate
   *
   * Emitted when a cell has been activated by the user,
   * usually via activating the BobguiGridView|list.activate-item action.
   *
   * This allows for a convenient way to handle activation in a gridview.
   * See [property@Bobgui.ListItem:activatable] for details on how to use
   * this signal.
   */
  signals[ACTIVATE] =
    g_signal_new (I_("activate"),
                  G_TYPE_FROM_CLASS (gobject_class),
                  G_SIGNAL_RUN_LAST,
                  0,
                  NULL, NULL,
                  g_cclosure_marshal_VOID__UINT,
                  G_TYPE_NONE, 1,
                  G_TYPE_UINT);
  g_signal_set_va_marshaller (signals[ACTIVATE],
                              G_TYPE_FROM_CLASS (gobject_class),
                              g_cclosure_marshal_VOID__UINTv);

  /**
   * BobguiGridView|list.activate-item:
   * @position: position of item to activate
   *
   * Activates the item given in @position by emitting the
   * [signal@Bobgui.GridView::activate] signal.
   */
  bobgui_widget_class_install_action (widget_class,
                                   "list.activate-item",
                                   "u",
                                   bobgui_grid_view_activate_item);

  bobgui_widget_class_set_css_name (widget_class, I_("gridview"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_GRID);
}

static void
bobgui_grid_view_init (BobguiGridView *self)
{
  self->item_manager = bobgui_list_base_get_manager (BOBGUI_LIST_BASE (self));

  self->min_columns = 1;
  self->max_columns = DEFAULT_MAX_COLUMNS;
  self->n_columns = 1;

  bobgui_list_base_set_anchor_max_widgets (BOBGUI_LIST_BASE (self),
                                        self->max_columns * BOBGUI_GRID_VIEW_MAX_VISIBLE_ROWS,
                                        self->max_columns);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "view");
}

/**
 * bobgui_grid_view_new:
 * @model: (nullable) (transfer full): the model to use
 * @factory: (nullable) (transfer full): The factory to populate items with
 *
 * Creates a new `BobguiGridView` that uses the given @factory for
 * mapping items to widgets.
 *
 * The function takes ownership of the
 * arguments, so you can write code like
 * ```c
 * grid_view = bobgui_grid_view_new (create_model (),
 *   bobgui_builder_list_item_factory_new_from_resource ("/resource.ui"));
 * ```
 *
 * Returns: a new `BobguiGridView` using the given @model and @factory
 */
BobguiWidget *
bobgui_grid_view_new (BobguiSelectionModel  *model,
                   BobguiListItemFactory *factory)
{
  BobguiWidget *result;

  g_return_val_if_fail (model == NULL || BOBGUI_IS_SELECTION_MODEL (model), NULL);
  g_return_val_if_fail (factory == NULL || BOBGUI_IS_LIST_ITEM_FACTORY (factory), NULL);

  result = g_object_new (BOBGUI_TYPE_GRID_VIEW,
                         "model", model,
                         "factory", factory,
                         NULL);

  /* consume the references */
  g_clear_object (&model);
  g_clear_object (&factory);

  return result;
}

/**
 * bobgui_grid_view_get_model:
 * @self: a `BobguiGridView`
 *
 * Gets the model that's currently used to read the items displayed.
 *
 * Returns: (nullable) (transfer none): The model in use
 **/
BobguiSelectionModel *
bobgui_grid_view_get_model (BobguiGridView *self)
{
  g_return_val_if_fail (BOBGUI_IS_GRID_VIEW (self), NULL);

  return bobgui_list_base_get_model (BOBGUI_LIST_BASE (self));
}

/**
 * bobgui_grid_view_set_model:
 * @self: a `BobguiGridView`
 * @model: (nullable) (transfer none): the model to use
 *
 * Sets the model to use.
 *
 * This must be a [iface@Bobgui.SelectionModel].
 */
void
bobgui_grid_view_set_model (BobguiGridView       *self,
                         BobguiSelectionModel *model)
{
  g_return_if_fail (BOBGUI_IS_GRID_VIEW (self));
  g_return_if_fail (model == NULL || BOBGUI_IS_SELECTION_MODEL (model));

  if (!bobgui_list_base_set_model (BOBGUI_LIST_BASE (self), model))
    return;

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE, BOBGUI_IS_MULTI_SELECTION (model),
                                  -1);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODEL]);
}

/**
 * bobgui_grid_view_get_factory:
 * @self: a `BobguiGridView`
 *
 * Gets the factory that's currently used to populate list items.
 *
 * Returns: (nullable) (transfer none): The factory in use
 */
BobguiListItemFactory *
bobgui_grid_view_get_factory (BobguiGridView *self)
{
  g_return_val_if_fail (BOBGUI_IS_GRID_VIEW (self), NULL);

  return self->factory;
}

/**
 * bobgui_grid_view_set_factory:
 * @self: a `BobguiGridView`
 * @factory: (nullable) (transfer none): the factory to use
 *
 * Sets the `BobguiListItemFactory` to use for populating list items.
 */
void
bobgui_grid_view_set_factory (BobguiGridView        *self,
                           BobguiListItemFactory *factory)
{
  g_return_if_fail (BOBGUI_IS_GRID_VIEW (self));
  g_return_if_fail (factory == NULL || BOBGUI_IS_LIST_ITEM_FACTORY (factory));

  if (!g_set_object (&self->factory, factory))
    return;

  bobgui_grid_view_update_factories (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FACTORY]);
}

/**
 * bobgui_grid_view_get_max_columns:
 * @self: a `BobguiGridView`
 *
 * Gets the maximum number of columns that the grid will use.
 *
 * Returns: The maximum number of columns
 */
guint
bobgui_grid_view_get_max_columns (BobguiGridView *self)
{
  g_return_val_if_fail (BOBGUI_IS_GRID_VIEW (self), DEFAULT_MAX_COLUMNS);

  return self->max_columns;
}

/**
 * bobgui_grid_view_set_max_columns:
 * @self: a `BobguiGridView`
 * @max_columns: The maximum number of columns
 *
 * Sets the maximum number of columns to use.
 *
 * This number must be at least 1.
 *
 * If @max_columns is smaller than the minimum set via
 * [method@Bobgui.GridView.set_min_columns], that value is used instead.
 */
void
bobgui_grid_view_set_max_columns (BobguiGridView *self,
                               guint        max_columns)
{
  g_return_if_fail (BOBGUI_IS_GRID_VIEW (self));
  g_return_if_fail (max_columns > 0);

  if (self->max_columns == max_columns)
    return;

  self->max_columns = max_columns;

  bobgui_list_base_set_anchor_max_widgets (BOBGUI_LIST_BASE (self),
                                        self->max_columns * BOBGUI_GRID_VIEW_MAX_VISIBLE_ROWS,
                                        self->max_columns);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MAX_COLUMNS]);
}

/**
 * bobgui_grid_view_get_min_columns:
 * @self: a `BobguiGridView`
 *
 * Gets the minimum number of columns that the grid will use.
 *
 * Returns: The minimum number of columns
 */
guint
bobgui_grid_view_get_min_columns (BobguiGridView *self)
{
  g_return_val_if_fail (BOBGUI_IS_GRID_VIEW (self), 1);

  return self->min_columns;
}

/**
 * bobgui_grid_view_set_min_columns:
 * @self: a `BobguiGridView`
 * @min_columns: The minimum number of columns
 *
 * Sets the minimum number of columns to use.
 *
 * This number must be at least 1.
 *
 * If @min_columns is smaller than the minimum set via
 * [method@Bobgui.GridView.set_max_columns], that value is ignored.
 */
void
bobgui_grid_view_set_min_columns (BobguiGridView *self,
                               guint        min_columns)
{
  g_return_if_fail (BOBGUI_IS_GRID_VIEW (self));
  g_return_if_fail (min_columns > 0);

  if (self->min_columns == min_columns)
    return;

  self->min_columns = min_columns;

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self));

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MIN_COLUMNS]);
}

/**
 * bobgui_grid_view_set_single_click_activate:
 * @self: a `BobguiGridView`
 * @single_click_activate: %TRUE to activate items on single click
 *
 * Sets whether items should be activated on single click and
 * selected on hover.
 */
void
bobgui_grid_view_set_single_click_activate (BobguiGridView *self,
                                         gboolean     single_click_activate)
{
  BobguiListTile *tile;

  g_return_if_fail (BOBGUI_IS_GRID_VIEW (self));

  if (single_click_activate == self->single_click_activate)
    return;

  self->single_click_activate = single_click_activate;

  for (tile = bobgui_list_item_manager_get_first (self->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      if (tile->widget)
        bobgui_list_factory_widget_set_single_click_activate (BOBGUI_LIST_FACTORY_WIDGET (tile->widget), single_click_activate);
    }


  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SINGLE_CLICK_ACTIVATE]);
}

/**
 * bobgui_grid_view_get_single_click_activate:
 * @self: a `BobguiGridView`
 *
 * Returns whether items will be activated on single click and
 * selected on hover.
 *
 * Returns: %TRUE if items are activated on single click
 */
gboolean
bobgui_grid_view_get_single_click_activate (BobguiGridView *self)
{
  g_return_val_if_fail (BOBGUI_IS_GRID_VIEW (self), FALSE);

  return self->single_click_activate;
}

/**
 * bobgui_grid_view_set_enable_rubberband:
 * @self: a `BobguiGridView`
 * @enable_rubberband: %TRUE to enable rubberband selection
 *
 * Sets whether selections can be changed by dragging with the mouse.
 */
void
bobgui_grid_view_set_enable_rubberband (BobguiGridView *self,
                                     gboolean     enable_rubberband)
{
  g_return_if_fail (BOBGUI_IS_GRID_VIEW (self));

  if (enable_rubberband == bobgui_list_base_get_enable_rubberband (BOBGUI_LIST_BASE (self)))
    return;

  bobgui_list_base_set_enable_rubberband (BOBGUI_LIST_BASE (self), enable_rubberband);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ENABLE_RUBBERBAND]);
}

/**
 * bobgui_grid_view_get_enable_rubberband:
 * @self: a `BobguiGridView`
 *
 * Returns whether rows can be selected by dragging with the mouse.
 *
 * Returns: %TRUE if rubberband selection is enabled
 */
gboolean
bobgui_grid_view_get_enable_rubberband (BobguiGridView *self)
{
  g_return_val_if_fail (BOBGUI_IS_GRID_VIEW (self), FALSE);

  return bobgui_list_base_get_enable_rubberband (BOBGUI_LIST_BASE (self));
}

/**
 * bobgui_grid_view_set_tab_behavior:
 * @self: a `BobguiGridView`
 * @tab_behavior: The desired tab behavior
 *
 * Sets the behavior of the <kbd>Tab</kbd> and <kbd>Shift</kbd>+<kbd>Tab</kbd> keys.
 *
 * Since: 4.12
 */
void
bobgui_grid_view_set_tab_behavior (BobguiGridView        *self,
                                BobguiListTabBehavior  tab_behavior)
{
  g_return_if_fail (BOBGUI_IS_GRID_VIEW (self));

  if (tab_behavior == bobgui_list_base_get_tab_behavior (BOBGUI_LIST_BASE (self)))
    return;

  bobgui_list_base_set_tab_behavior (BOBGUI_LIST_BASE (self), tab_behavior);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TAB_BEHAVIOR]);
}

/**
 * bobgui_grid_view_get_tab_behavior:
 * @self: a `BobguiGridView`
 *
 * Gets the behavior set for the <kbd>Tab</kbd> key.
 *
 * Returns: The behavior of the <kbd>Tab</kbd> key
 *
 * Since: 4.12
 */
BobguiListTabBehavior
bobgui_grid_view_get_tab_behavior (BobguiGridView *self)
{
  g_return_val_if_fail (BOBGUI_IS_GRID_VIEW (self), FALSE);

  return bobgui_list_base_get_tab_behavior (BOBGUI_LIST_BASE (self));
}

/**
 * bobgui_grid_view_scroll_to:
 * @self: The gridview to scroll in
 * @pos: position of the item. Must be less than the number of
 *   items in the view.
 * @flags: actions to perform
 * @scroll: (nullable) (transfer full): details of how to perform
 *   the scroll operation or %NULL to scroll into view
 *
 * Scrolls to the item at the given position and performs the actions
 * specified in @flags.
 *
 * This function works no matter if the gridview is shown or focused.
 * If it isn't, then the changes will take effect once that happens.
 *
 * Since: 4.12
 */
void
bobgui_grid_view_scroll_to (BobguiGridView        *self,
                         guint               pos,
                         BobguiListScrollFlags  flags,
                         BobguiScrollInfo      *scroll)
{
  g_return_if_fail (BOBGUI_IS_GRID_VIEW (self));
  g_return_if_fail (pos < bobgui_list_base_get_n_items (BOBGUI_LIST_BASE (self)));

  bobgui_list_base_scroll_to (BOBGUI_LIST_BASE (self), pos, flags, scroll);
}
