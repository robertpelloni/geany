/*
 * Copyright © 2018 Benjamin Otte
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

#include "bobguilistviewprivate.h"

#include "bobguibitset.h"
#include "bobguilistbaseprivate.h"
#include "bobguilistheaderwidgetprivate.h"
#include "bobguilistitemmanagerprivate.h"
#include "bobguilistitemwidgetprivate.h"
#include "bobguimultiselection.h"
#include "bobguitypebuiltins.h"
#include "bobguiwidgetprivate.h"

/* Maximum number of list items created by the listview.
 * For debugging, you can set this to G_MAXUINT to ensure
 * there's always a list item for every row.
 */
#define BOBGUI_LIST_VIEW_MAX_LIST_ITEMS 200

/* Extra items to keep above + below every tracker */
#define BOBGUI_LIST_VIEW_EXTRA_ITEMS 2

/**
 * BobguiListView:
 *
 * Presents a large dynamic list of items.
 *
 * `BobguiListView` uses its factory to generate one row widget for each visible
 * item and shows them in a linear display, either vertically or horizontally.
 *
 * The [property@Bobgui.ListView:show-separators] property offers a simple way to
 * display separators between the rows.
 *
 * `BobguiListView` allows the user to select items according to the selection
 * characteristics of the model. For models that allow multiple selected items,
 * it is possible to turn on _rubberband selection_, using
 * [property@Bobgui.ListView:enable-rubberband].
 *
 * If you need multiple columns with headers, see [class@Bobgui.ColumnView].
 *
 * To learn more about the list widget framework, see the
 * [overview](section-list-widget.html).
 *
 * An example of using `BobguiListView`:
 * ```c
 * static void
 * setup_listitem_cb (BobguiListItemFactory *factory,
 *                    BobguiListItem        *list_item)
 * {
 *   BobguiWidget *image;
 *
 *   image = bobgui_image_new ();
 *   bobgui_image_set_icon_size (BOBGUI_IMAGE (image), BOBGUI_ICON_SIZE_LARGE);
 *   bobgui_list_item_set_child (list_item, image);
 * }
 *
 * static void
 * bind_listitem_cb (BobguiListItemFactory *factory,
 *                   BobguiListItem        *list_item)
 * {
 *   BobguiWidget *image;
 *   GAppInfo *app_info;
 *
 *   image = bobgui_list_item_get_child (list_item);
 *   app_info = bobgui_list_item_get_item (list_item);
 *   bobgui_image_set_from_gicon (BOBGUI_IMAGE (image), g_app_info_get_icon (app_info));
 * }
 *
 * static void
 * activate_cb (BobguiListView  *list,
 *              guint         position,
 *              gpointer      unused)
 * {
 *   GAppInfo *app_info;
 *
 *   app_info = g_list_model_get_item (G_LIST_MODEL (bobgui_list_view_get_model (list)), position);
 *   g_app_info_launch (app_info, NULL, NULL, NULL);
 *   g_object_unref (app_info);
 * }
 *
 * ...
 *
 *   model = create_application_list ();
 *
 *   factory = bobgui_signal_list_item_factory_new ();
 *   g_signal_connect (factory, "setup", G_CALLBACK (setup_listitem_cb), NULL);
 *   g_signal_connect (factory, "bind", G_CALLBACK (bind_listitem_cb), NULL);
 *
 *   list = bobgui_list_view_new (BOBGUI_SELECTION_MODEL (bobgui_single_selection_new (model)), factory);
 *
 *   g_signal_connect (list, "activate", G_CALLBACK (activate_cb), NULL);
 *
 *   bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), list);
 * ```
 *
 * # Actions
 *
 * `BobguiListView` defines a set of built-in actions:
 *
 * - `list.activate-item` activates the item at given position by emitting
 *   the [signal@Bobgui.ListView::activate] signal.
 *
 * # CSS nodes
 *
 * ```
 * listview[.separators][.rich-list][.navigation-sidebar][.data-table]
 * ├── row[.activatable]
 * │
 * ├── row[.activatable]
 * │
 * ┊
 * ╰── [rubberband]
 * ```
 *
 * `BobguiListView` uses a single CSS node named `listview`. It may carry the
 * `.separators` style class, when [property@Bobgui.ListView:show-separators]
 * property is set. Each child widget uses a single CSS node named `row`.
 * If the [property@Bobgui.ListItem:activatable] property is set, the
 * corresponding row will have the `.activatable` style class. For
 * rubberband selection, a node with name `rubberband` is used.
 *
 * The main listview node may also carry style classes to select
 * the style of [list presentation](section-list-widget.html#list-styles):
 * .rich-list, .navigation-sidebar or .data-table.
 *
 * # Accessibility
 *
 * `BobguiListView` uses the [enum@Bobgui.AccessibleRole.list] role, and the list
 * items use the [enum@Bobgui.AccessibleRole.list_item] role.
 */

enum
{
  PROP_0,
  PROP_ENABLE_RUBBERBAND,
  PROP_FACTORY,
  PROP_HEADER_FACTORY,
  PROP_MODEL,
  PROP_SHOW_SEPARATORS,
  PROP_SINGLE_CLICK_ACTIVATE,
  PROP_TAB_BEHAVIOR,

  N_PROPS
};

enum {
  ACTIVATE,
  LAST_SIGNAL
};

G_DEFINE_TYPE (BobguiListView, bobgui_list_view, BOBGUI_TYPE_LIST_BASE)

static GParamSpec *properties[N_PROPS] = { NULL, };
static guint signals[LAST_SIGNAL] = { 0 };

static BobguiListTile *
bobgui_list_view_split (BobguiListBase *base,
                     BobguiListTile *tile,
                     guint        n_items)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (base);
  BobguiListTile *new_tile;
  int spacing, row_height;

  bobgui_list_base_get_border_spacing (BOBGUI_LIST_BASE (self), NULL, &spacing);
  row_height = (tile->area.height - (tile->n_items - 1) * spacing) / tile->n_items;

  new_tile = bobgui_list_tile_split (self->item_manager, tile, n_items);
  bobgui_list_tile_set_area_size (self->item_manager,
                               tile,
                               tile->area.width,
                               row_height * tile->n_items + spacing * (tile->n_items - 1));
  bobgui_list_tile_set_area (self->item_manager,
                          new_tile,
                          &(GdkRectangle) {
                            tile->area.x,
                            tile->area.y + tile->area.height + spacing,
                            tile->area.width,
                            row_height * new_tile->n_items + spacing * (new_tile->n_items - 1)
                          });

  return new_tile;
}

static void
bobgui_list_view_prepare_section (BobguiListBase *base,
                               BobguiListTile *tile,
                               guint        position)
{
}

/* We define the listview as **inert** when the factory isn't used. */
static gboolean
bobgui_list_view_is_inert (BobguiListView *self)
{
  BobguiWidget *widget = BOBGUI_WIDGET (self);

  return !bobgui_widget_get_visible (widget) ||
         bobgui_widget_get_root (widget) == NULL;
}

static void
bobgui_list_view_update_factories_with (BobguiListView        *self,
                                     BobguiListItemFactory *factory,
                                     BobguiListItemFactory *header_factory)
{
  BobguiListTile *tile;

  for (tile = bobgui_list_item_manager_get_first (self->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      switch (tile->type)
        {
        case BOBGUI_LIST_TILE_ITEM:
          if (tile->widget)
            bobgui_list_factory_widget_set_factory (BOBGUI_LIST_FACTORY_WIDGET (tile->widget), factory);
          break;
        case BOBGUI_LIST_TILE_HEADER:
          if (tile->widget)
            bobgui_list_header_widget_set_factory (BOBGUI_LIST_HEADER_WIDGET (tile->widget), header_factory);
          break;
        case BOBGUI_LIST_TILE_UNMATCHED_HEADER:
        case BOBGUI_LIST_TILE_FOOTER:
        case BOBGUI_LIST_TILE_UNMATCHED_FOOTER:
        case BOBGUI_LIST_TILE_REMOVED:
          g_assert (tile->widget == NULL);
          break;
        default:
          g_assert_not_reached();
          break;
        }
    }
}

static void
bobgui_list_view_update_factories (BobguiListView *self)
{
  bobgui_list_view_update_factories_with (self,
                                       bobgui_list_view_is_inert (self) ? NULL : self->factory,
                                       bobgui_list_view_is_inert (self) ? NULL : self->header_factory);
}

static void
bobgui_list_view_clear_factories (BobguiListView *self)
{
  bobgui_list_view_update_factories_with (self, NULL, NULL);
}

static BobguiListItemBase *
bobgui_list_view_create_list_widget (BobguiListBase *base)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (base);
  BobguiListItemFactory *factory;
  BobguiWidget *result;

  if (bobgui_list_view_is_inert (self))
    factory = NULL;
  else
    factory = self->factory;

  result = bobgui_list_item_widget_new (factory,
                                     "row",
                                     BOBGUI_ACCESSIBLE_ROLE_LIST_ITEM);

  bobgui_list_factory_widget_set_single_click_activate (BOBGUI_LIST_FACTORY_WIDGET (result), self->single_click_activate);

  return BOBGUI_LIST_ITEM_BASE (result);
}

static BobguiListHeaderBase *
bobgui_list_view_create_header_widget (BobguiListBase *base)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (base);
  BobguiListItemFactory *factory;

  if (bobgui_list_view_is_inert (self))
    factory = NULL;
  else
    factory = self->header_factory;

  return BOBGUI_LIST_HEADER_BASE (bobgui_list_header_widget_new (factory));
}

static gboolean
bobgui_list_view_get_allocation (BobguiListBase  *base,
                              guint         pos,
                              GdkRectangle *area)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (base);
  BobguiListTile *tile;
  guint offset;

  tile = bobgui_list_item_manager_get_nth (self->item_manager, pos, &offset);
  if (tile == NULL)
    return FALSE;

  *area = tile->area;
  if (area->width || area->height)
    {
      if (tile->n_items)
        area->height /= tile->n_items;
      if (offset)
        area->y += offset * area->height;
    }
  else
    {
      /* item is not allocated yet */
      BobguiListTile *other;
      int spacing;

      bobgui_list_base_get_border_spacing (BOBGUI_LIST_BASE (self), NULL, &spacing);

      for (other = bobgui_rb_tree_node_get_previous (tile);
           other;
           other = bobgui_rb_tree_node_get_previous (other))
        {
          if (other->area.width || other->area.height)
            {
              area->x = other->area.x;
              area->width = other->area.width;
              area->y = other->area.y + other->area.height + spacing;
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
                  area->width = other->area.width;
                  area->y = MAX (0, other->area.y - spacing);
                  break;
                }
            }
        }
    }

  return TRUE;
}

static guint
bobgui_list_view_move_focus_along (BobguiListBase *base,
                                guint        pos,
                                int          steps)
{
  if (steps < 0)
    return pos - MIN (pos, -steps);
  else
    {
      pos += MIN (bobgui_list_base_get_n_items (base) - pos - 1, steps);
    }

  return pos;
}

static gboolean
bobgui_list_view_get_position_from_allocation (BobguiListBase           *base,
                                            int                    x,
                                            int                    y,
                                            guint                 *pos,
                                            cairo_rectangle_int_t *area)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (base);
  BobguiListTile *tile;

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

  *pos = bobgui_list_tile_get_position (self->item_manager, tile);
  if (area)
    *area = tile->area;

  if (tile->n_items > 1)
    {
      int row_height, tile_pos, spacing;

      bobgui_list_base_get_border_spacing (BOBGUI_LIST_BASE (self), NULL, &spacing);
      row_height = (tile->area.height - (tile->n_items - 1) * spacing) / tile->n_items;
      if (y >= tile->area.y + tile->area.height)
        tile_pos = tile->n_items - 1;
      else
        tile_pos = (y - tile->area.y) / (row_height + spacing);

      *pos += tile_pos;
      if (area)
        {
          area->y = tile->area.y + tile_pos * (row_height + spacing);
          area->height = row_height;
        }
    }

  return TRUE;
}

static BobguiBitset *
bobgui_list_view_get_items_in_rect (BobguiListBase                 *base,
                                 const cairo_rectangle_int_t *rect)
{
  guint first, last;
  cairo_rectangle_int_t area;
  BobguiBitset *result;

  result = bobgui_bitset_new_empty ();

  if (!bobgui_list_view_get_position_from_allocation (base, rect->x, rect->y, &first, &area))
    return result;
  if (area.y + area.height < rect->y)
    first++;

  if (!bobgui_list_view_get_position_from_allocation (base,
                                                   rect->x + rect->width - 1,
                                                   rect->y + rect->height - 1,
                                                   &last, &area))
    return result;
  if (area.y >= rect->y + rect->height)
    last--;

  if (last >= first)
    bobgui_bitset_add_range_closed (result, first, last);

  return result;
}

static guint
bobgui_list_view_move_focus_across (BobguiListBase *base,
                                 guint        pos,
                                 int          steps)
{
  return pos;
}

static int
compare_ints (gconstpointer first,
               gconstpointer second)
{
  return *(int *) first - *(int *) second;
}

static guint
bobgui_list_view_get_unknown_row_height (BobguiListView *self,
                                      GArray      *heights)
{
  g_return_val_if_fail (heights->len > 0, 0);

  /* return the median and hope rows are generally uniform with few outliers */
  g_array_sort (heights, compare_ints);

  return g_array_index (heights, int, heights->len / 2);
}

static void
bobgui_list_view_measure_across (BobguiWidget      *widget,
                              BobguiOrientation  orientation,
                              int             for_size,
                              int            *minimum,
                              int            *natural)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (widget);
  BobguiListTile *tile;
  int min, nat, child_min, child_nat;
  /* XXX: Figure out how to split a given height into per-row heights.
   * Good luck! */
  for_size = -1;

  min = 0;
  nat = 0;

  for (tile = bobgui_list_item_manager_get_first (self->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      /* ignore unavailable rows */
      if (tile->widget == NULL)
        continue;

      bobgui_widget_measure (tile->widget,
                          orientation, for_size,
                          &child_min, &child_nat, NULL, NULL);
      min = MAX (min, child_min);
      nat = MAX (nat, child_nat);
    }

  *minimum = min;
  *natural = nat;
}

static void
bobgui_list_view_measure_list (BobguiWidget      *widget,
                            BobguiOrientation  orientation,
                            int             for_size,
                            int            *minimum,
                            int            *natural)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (widget);
  BobguiListTile *tile;
  int min, nat, child_min, child_nat, spacing;
  GArray *min_heights, *nat_heights;
  guint n_unknown, n_items;

  n_items = bobgui_list_base_get_n_items (BOBGUI_LIST_BASE (self));
  if (n_items == 0)
    return;
  bobgui_list_base_get_border_spacing (BOBGUI_LIST_BASE (self), NULL, &spacing);

  min_heights = g_array_new (FALSE, FALSE, sizeof (int));
  nat_heights = g_array_new (FALSE, FALSE, sizeof (int));
  n_unknown = 0;
  min = 0;
  nat = 0;

  for (tile = bobgui_list_item_manager_get_first (self->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      if (tile->widget)
        {
          bobgui_widget_measure (tile->widget,
                              orientation, for_size,
                              &child_min, &child_nat, NULL, NULL);
          if (tile->type == BOBGUI_LIST_TILE_ITEM)
            {
              g_array_append_val (min_heights, child_min);
              g_array_append_val (nat_heights, child_nat);
            }
          min += child_min;
          nat += child_nat;
        }
      else
        {
          n_unknown += tile->n_items;
        }
    }

  if (n_unknown)
    {
      min += n_unknown * bobgui_list_view_get_unknown_row_height (self, min_heights);
      nat += n_unknown * bobgui_list_view_get_unknown_row_height (self, nat_heights);
    }
  g_array_free (min_heights, TRUE);
  g_array_free (nat_heights, TRUE);

  *minimum = min + spacing * (n_items - 1);
  *natural = nat + spacing * (n_items - 1);
}

static void
bobgui_list_view_measure (BobguiWidget      *widget,
                       BobguiOrientation  orientation,
                       int             for_size,
                       int            *minimum,
                       int            *natural,
                       int            *minimum_baseline,
                       int            *natural_baseline)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (widget);

  if (orientation == bobgui_list_base_get_orientation (BOBGUI_LIST_BASE (self)))
    bobgui_list_view_measure_list (widget, orientation, for_size, minimum, natural);
  else
    bobgui_list_view_measure_across (widget, orientation, for_size, minimum, natural);
}

static void
bobgui_list_view_size_allocate (BobguiWidget *widget,
                             int        width,
                             int        height,
                             int        baseline)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (widget);
  BobguiListTile *tile;
  GArray *heights;
  int min, nat, row_height, y, list_width, spacing;
  BobguiOrientation orientation, opposite_orientation;
  BobguiScrollablePolicy scroll_policy, opposite_scroll_policy;

  orientation = bobgui_list_base_get_orientation (BOBGUI_LIST_BASE (self));
  opposite_orientation = OPPOSITE_ORIENTATION (orientation);
  scroll_policy = bobgui_list_base_get_scroll_policy (BOBGUI_LIST_BASE (self), orientation);
  opposite_scroll_policy = bobgui_list_base_get_scroll_policy (BOBGUI_LIST_BASE (self), opposite_orientation);
  bobgui_list_base_get_border_spacing (BOBGUI_LIST_BASE (self), NULL, &spacing);

  bobgui_list_item_manager_gc_tiles (self->item_manager);

  /* step 0: exit early if list is empty */
  tile = bobgui_list_item_manager_get_first (self->item_manager);
  if (tile == NULL)
    {
      bobgui_list_base_allocate (BOBGUI_LIST_BASE (self));
      return;
    }

  /* step 1: determine width of the list */
  bobgui_list_view_measure_across (widget, opposite_orientation,
                                -1,
                                &min, &nat);
  list_width = orientation == BOBGUI_ORIENTATION_VERTICAL ? width : height;
  if (opposite_scroll_policy == BOBGUI_SCROLL_MINIMUM)
    list_width = MAX (min, list_width);
  else
    list_width = MAX (nat, list_width);

  /* step 2: determine height of known list items and gc the list */
  heights = g_array_new (FALSE, FALSE, sizeof (int));

  for (;
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      if (tile->widget == NULL)
        continue;

      bobgui_widget_measure (tile->widget, orientation,
                          list_width,
                          &min, &nat, NULL, NULL);
      if (scroll_policy == BOBGUI_SCROLL_MINIMUM)
        row_height = min;
      else
        row_height = nat;
      bobgui_list_tile_set_area_size (self->item_manager, tile, list_width, row_height);
      if (tile->type == BOBGUI_LIST_TILE_ITEM)
        g_array_append_val (heights, row_height);
    }

  /* step 3: determine height of unknown items and set the positions */
  row_height = bobgui_list_view_get_unknown_row_height (self, heights);
  g_array_free (heights, TRUE);

  y = 0;
  for (tile = bobgui_list_item_manager_get_first (self->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      bobgui_list_tile_set_area_position (self->item_manager, tile, 0, y);
      if (tile->widget == NULL)
        {
          bobgui_list_tile_set_area_size (self->item_manager,
                                       tile,
                                       list_width,
                                       row_height * tile->n_items
                                       + spacing * (tile->n_items - 1));
        }

      y += tile->area.height + spacing;
    }

  /* step 4: allocate the rest */
  bobgui_list_base_allocate (BOBGUI_LIST_BASE (self));
}

static void
bobgui_list_view_root (BobguiWidget *widget)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (widget);

  BOBGUI_WIDGET_CLASS (bobgui_list_view_parent_class)->root (widget);

  if (!bobgui_list_view_is_inert (self))
    bobgui_list_view_update_factories (self);
}

static void
bobgui_list_view_unroot (BobguiWidget *widget)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (widget);

  if (!bobgui_list_view_is_inert (self))
    bobgui_list_view_clear_factories (self);

  BOBGUI_WIDGET_CLASS (bobgui_list_view_parent_class)->unroot (widget);
}

static void
bobgui_list_view_show (BobguiWidget *widget)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (widget);

  BOBGUI_WIDGET_CLASS (bobgui_list_view_parent_class)->show (widget);

  if (!bobgui_list_view_is_inert (self))
    bobgui_list_view_update_factories (self);
}

static void
bobgui_list_view_hide (BobguiWidget *widget)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (widget);

  if (!bobgui_list_view_is_inert (self))
    bobgui_list_view_clear_factories (self);

  BOBGUI_WIDGET_CLASS (bobgui_list_view_parent_class)->hide (widget);
}

static void
bobgui_list_view_dispose (GObject *object)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (object);

  self->item_manager = NULL;

  g_clear_object (&self->factory);
  g_clear_object (&self->header_factory);

  G_OBJECT_CLASS (bobgui_list_view_parent_class)->dispose (object);
}

static void
bobgui_list_view_get_property (GObject    *object,
                            guint       property_id,
                            GValue     *value,
                            GParamSpec *pspec)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (object);

  switch (property_id)
    {
    case PROP_ENABLE_RUBBERBAND:
      g_value_set_boolean (value, bobgui_list_base_get_enable_rubberband (BOBGUI_LIST_BASE (self)));
      break;

    case PROP_FACTORY:
      g_value_set_object (value, self->factory);
      break;

    case PROP_HEADER_FACTORY:
      g_value_set_object (value, self->header_factory);
      break;

    case PROP_MODEL:
      g_value_set_object (value, bobgui_list_base_get_model (BOBGUI_LIST_BASE (self)));
      break;

    case PROP_SHOW_SEPARATORS:
      g_value_set_boolean (value, self->show_separators);
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
bobgui_list_view_set_property (GObject      *object,
                            guint         property_id,
                            const GValue *value,
                            GParamSpec   *pspec)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (object);

  switch (property_id)
    {
    case PROP_ENABLE_RUBBERBAND:
      bobgui_list_view_set_enable_rubberband (self, g_value_get_boolean (value));
      break;

    case PROP_FACTORY:
      bobgui_list_view_set_factory (self, g_value_get_object (value));
      break;

    case PROP_HEADER_FACTORY:
      bobgui_list_view_set_header_factory (self, g_value_get_object (value));
      break;

    case PROP_MODEL:
      bobgui_list_view_set_model (self, g_value_get_object (value));
      break;

    case PROP_SHOW_SEPARATORS:
      bobgui_list_view_set_show_separators (self, g_value_get_boolean (value));
      break;

    case PROP_SINGLE_CLICK_ACTIVATE:
      bobgui_list_view_set_single_click_activate (self, g_value_get_boolean (value));
      break;

    case PROP_TAB_BEHAVIOR:
      bobgui_list_view_set_tab_behavior (self, g_value_get_enum (value));
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
      break;
    }
}

static void
bobgui_list_view_activate_item (BobguiWidget  *widget,
                             const char *action_name,
                             GVariant   *parameter)
{
  BobguiListView *self = BOBGUI_LIST_VIEW (widget);
  guint pos;

  if (!g_variant_check_format_string (parameter, "u", FALSE))
    return;

  g_variant_get (parameter, "u", &pos);
  if (pos >= bobgui_list_base_get_n_items (BOBGUI_LIST_BASE (self)))
    return;

  g_signal_emit (widget, signals[ACTIVATE], 0, pos);
}

static void
bobgui_list_view_class_init (BobguiListViewClass *klass)
{
  BobguiListBaseClass *list_base_class = BOBGUI_LIST_BASE_CLASS (klass);
  BobguiWidgetClass *widget_class = BOBGUI_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  list_base_class->split = bobgui_list_view_split;
  list_base_class->create_list_widget = bobgui_list_view_create_list_widget;
  list_base_class->prepare_section = bobgui_list_view_prepare_section;
  list_base_class->create_header_widget = bobgui_list_view_create_header_widget;
  list_base_class->get_allocation = bobgui_list_view_get_allocation;
  list_base_class->get_items_in_rect = bobgui_list_view_get_items_in_rect;
  list_base_class->get_position_from_allocation = bobgui_list_view_get_position_from_allocation;
  list_base_class->move_focus_along = bobgui_list_view_move_focus_along;
  list_base_class->move_focus_across = bobgui_list_view_move_focus_across;

  widget_class->measure = bobgui_list_view_measure;
  widget_class->size_allocate = bobgui_list_view_size_allocate;
  widget_class->root = bobgui_list_view_root;
  widget_class->unroot = bobgui_list_view_unroot;
  widget_class->show = bobgui_list_view_show;
  widget_class->hide = bobgui_list_view_hide;

  gobject_class->dispose = bobgui_list_view_dispose;
  gobject_class->get_property = bobgui_list_view_get_property;
  gobject_class->set_property = bobgui_list_view_set_property;

  /**
   * BobguiListView:enable-rubberband:
   *
   * Allow rubberband selection.
   */
  properties[PROP_ENABLE_RUBBERBAND] =
    g_param_spec_boolean ("enable-rubberband", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiListView:factory:
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
   * BobguiListView:header-factory:
   *
   * Factory for creating header widgets.
   *
   * The factory must be for configuring [class@Bobgui.ListHeader] objects.
   *
   * Since: 4.12
   */
  properties[PROP_HEADER_FACTORY] =
    g_param_spec_object ("header-factory", NULL, NULL,
                         BOBGUI_TYPE_LIST_ITEM_FACTORY,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListView:model:
   *
   * Model for the items displayed.
   */
  properties[PROP_MODEL] =
    g_param_spec_object ("model", NULL, NULL,
                         BOBGUI_TYPE_SELECTION_MODEL,
                         G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY | G_PARAM_STATIC_STRINGS);

  /**
   * BobguiListView:show-separators:
   *
   * Show separators between rows.
   */
  properties[PROP_SHOW_SEPARATORS] =
    g_param_spec_boolean ("show-separators", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiListView:single-click-activate:
   *
   * Activate rows on single click and select them on hover.
   */
  properties[PROP_SINGLE_CLICK_ACTIVATE] =
    g_param_spec_boolean ("single-click-activate", NULL, NULL,
                          FALSE,
                          G_PARAM_READWRITE | G_PARAM_EXPLICIT_NOTIFY);

  /**
   * BobguiListView:tab-behavior:
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
   * BobguiListView::activate:
   * @self: the listview
   * @position: position of item to activate
   *
   * Emitted when a row has been activated by the user.
   *
   * Activation usually happens via the list.activate-item action of
   * the `BobguiListView`.
   *
   * This allows for a convenient way to handle activation in a listview.
   * See [method@Bobgui.ListItem.set_activatable] for details on how to use
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
   * BobguiListView|list.activate-item:
   * @position: position of item to activate
   *
   * Activates the item given in @position by emitting the
   * [signal@Bobgui.ListView::activate] signal.
   */
  bobgui_widget_class_install_action (widget_class,
                                   "list.activate-item",
                                   "u",
                                   bobgui_list_view_activate_item);

  bobgui_widget_class_set_css_name (widget_class, I_("listview"));
  bobgui_widget_class_set_accessible_role (widget_class, BOBGUI_ACCESSIBLE_ROLE_LIST);
}

static void
bobgui_list_view_init (BobguiListView *self)
{
  self->item_manager = bobgui_list_base_get_manager (BOBGUI_LIST_BASE (self));

  bobgui_list_base_set_anchor_max_widgets (BOBGUI_LIST_BASE (self),
                                        BOBGUI_LIST_VIEW_MAX_LIST_ITEMS,
                                        BOBGUI_LIST_VIEW_EXTRA_ITEMS);

  bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "view");
}

/**
 * bobgui_list_view_new:
 * @model: (nullable) (transfer full): the model to use
 * @factory: (nullable) (transfer full): The factory to populate items with
 *
 * Creates a new `BobguiListView` that uses the given @factory for
 * mapping items to widgets.
 *
 * The function takes ownership of the
 * arguments, so you can write code like
 * ```c
 * list_view = bobgui_list_view_new (create_model (),
 *   bobgui_builder_list_item_factory_new_from_resource ("/resource.ui"));
 * ```
 *
 * Returns: a new `BobguiListView` using the given @model and @factory
 */
BobguiWidget *
bobgui_list_view_new (BobguiSelectionModel  *model,
                   BobguiListItemFactory *factory)
{
  BobguiWidget *result;

  g_return_val_if_fail (model == NULL || BOBGUI_IS_SELECTION_MODEL (model), NULL);
  g_return_val_if_fail (factory == NULL || BOBGUI_IS_LIST_ITEM_FACTORY (factory), NULL);

  result = g_object_new (BOBGUI_TYPE_LIST_VIEW,
                         "model", model,
                         "factory", factory,
                         NULL);

  /* consume the references */
  g_clear_object (&model);
  g_clear_object (&factory);

  return result;
}

/**
 * bobgui_list_view_get_model:
 * @self: a listview
 *
 * Gets the model that's currently used to read the items displayed.
 *
 * Returns: (nullable) (transfer none): The model in use
 */
BobguiSelectionModel *
bobgui_list_view_get_model (BobguiListView *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_VIEW (self), NULL);

  return bobgui_list_base_get_model (BOBGUI_LIST_BASE (self));
}

/**
 * bobgui_list_view_set_model:
 * @self: a listview
 * @model: (nullable) (transfer none): the model to use
 *
 * Sets the model to use.
 *
 * This must be a [iface@Bobgui.SelectionModel] to use.
 */
void
bobgui_list_view_set_model (BobguiListView       *self,
                         BobguiSelectionModel *model)
{
  g_return_if_fail (BOBGUI_IS_LIST_VIEW (self));
  g_return_if_fail (model == NULL || BOBGUI_IS_SELECTION_MODEL (model));

  if (!bobgui_list_base_set_model (BOBGUI_LIST_BASE (self), model))
    return;

  bobgui_accessible_update_property (BOBGUI_ACCESSIBLE (self),
                                  BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE, BOBGUI_IS_MULTI_SELECTION (model),
                                  -1);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_MODEL]);
}

/**
 * bobgui_list_view_get_factory:
 * @self: a listview
 *
 * Gets the factory that's currently used to populate list items.
 *
 * Returns: (nullable) (transfer none): The factory in use
 */
BobguiListItemFactory *
bobgui_list_view_get_factory (BobguiListView *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_VIEW (self), NULL);

  return self->factory;
}

/**
 * bobgui_list_view_set_factory:
 * @self: a listview
 * @factory: (nullable) (transfer none): the factory to use
 *
 * Sets the `BobguiListItemFactory` to use for populating list items.
 */
void
bobgui_list_view_set_factory (BobguiListView        *self,
                           BobguiListItemFactory *factory)
{
  g_return_if_fail (BOBGUI_IS_LIST_VIEW (self));
  g_return_if_fail (factory == NULL || BOBGUI_IS_LIST_ITEM_FACTORY (factory));

  if (!g_set_object (&self->factory, factory))
    return;

  bobgui_list_view_update_factories (self);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_FACTORY]);
}

/**
 * bobgui_list_view_get_header_factory:
 * @self: a listview
 *
 * Gets the factory that's currently used to populate section headers.
 *
 * Returns: (nullable) (transfer none): The factory in use
 *
 * Since: 4.12
 */
BobguiListItemFactory *
bobgui_list_view_get_header_factory (BobguiListView *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_VIEW (self), NULL);

  return self->header_factory;
}

/**
 * bobgui_list_view_set_header_factory:
 * @self: a listview
 * @factory: (nullable) (transfer none): the factory to use
 *
 * Sets the `BobguiListItemFactory` to use for populating the
 * [class@Bobgui.ListHeader] objects used in section headers.
 *
 * If this factory is set to `NULL`, the list will not show
 * section headers.
 *
 * Since: 4.12
 */
void
bobgui_list_view_set_header_factory (BobguiListView        *self,
                                  BobguiListItemFactory *factory)
{
  gboolean had_sections;

  g_return_if_fail (BOBGUI_IS_LIST_VIEW (self));
  g_return_if_fail (factory == NULL || BOBGUI_IS_LIST_ITEM_FACTORY (factory));

  had_sections = bobgui_list_item_manager_get_has_sections (self->item_manager);

  if (!g_set_object (&self->header_factory, factory))
    return;

  bobgui_list_item_manager_set_has_sections (self->item_manager, factory != NULL);

  if (!bobgui_list_view_is_inert (self) &&
      had_sections && bobgui_list_item_manager_get_has_sections (self->item_manager))
    {
      BobguiListTile *tile;

      for (tile = bobgui_list_item_manager_get_first (self->item_manager);
           tile != NULL;
           tile = bobgui_rb_tree_node_get_next (tile))
        {
          if (tile->widget && tile->type == BOBGUI_LIST_TILE_HEADER)
            bobgui_list_header_widget_set_factory (BOBGUI_LIST_HEADER_WIDGET (tile->widget), factory);
        }
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_HEADER_FACTORY]);
}

/**
 * bobgui_list_view_set_show_separators:
 * @self: a listview
 * @show_separators: whether to show separators
 *
 * Sets whether the listview should show separators
 * between rows.
 */
void
bobgui_list_view_set_show_separators (BobguiListView *self,
                                   gboolean     show_separators)
{
  g_return_if_fail (BOBGUI_IS_LIST_VIEW (self));

  if (self->show_separators == show_separators)
    return;

  self->show_separators = show_separators;

  if (show_separators)
    bobgui_widget_add_css_class (BOBGUI_WIDGET (self), "separators");
  else
    bobgui_widget_remove_css_class (BOBGUI_WIDGET (self), "separators");

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SHOW_SEPARATORS]);
}

/**
 * bobgui_list_view_get_show_separators:
 * @self: a listview
 *
 * Returns whether the listview should show separators
 * between rows.
 *
 * Returns: true if the listview shows separators
 */
gboolean
bobgui_list_view_get_show_separators (BobguiListView *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_VIEW (self), FALSE);

  return self->show_separators;
}

/**
 * bobgui_list_view_set_single_click_activate:
 * @self: a listview
 * @single_click_activate: whether to activate items on single click
 *
 * Sets whether rows should be activated on single click and
 * selected on hover.
 */
void
bobgui_list_view_set_single_click_activate (BobguiListView *self,
                                         gboolean     single_click_activate)
{
  BobguiListTile *tile;

  g_return_if_fail (BOBGUI_IS_LIST_VIEW (self));

  if (single_click_activate == self->single_click_activate)
    return;

  self->single_click_activate = single_click_activate;

  for (tile = bobgui_list_item_manager_get_first (self->item_manager);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      if (tile->widget && tile->type == BOBGUI_LIST_TILE_ITEM)
        bobgui_list_factory_widget_set_single_click_activate (BOBGUI_LIST_FACTORY_WIDGET (tile->widget), single_click_activate);
    }

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_SINGLE_CLICK_ACTIVATE]);
}

/**
 * bobgui_list_view_get_single_click_activate:
 * @self: a listview
 *
 * Returns whether rows will be activated on single click and
 * selected on hover.
 *
 * Returns: true if rows are activated on single click
 */
gboolean
bobgui_list_view_get_single_click_activate (BobguiListView *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_VIEW (self), FALSE);

  return self->single_click_activate;
}

/**
 * bobgui_list_view_set_enable_rubberband:
 * @self: a listview
 * @enable_rubberband: whether to enable rubberband selection
 *
 * Sets whether selections can be changed by dragging with the mouse.
 */
void
bobgui_list_view_set_enable_rubberband (BobguiListView *self,
                                     gboolean     enable_rubberband)
{
  g_return_if_fail (BOBGUI_IS_LIST_VIEW (self));

  if (enable_rubberband == bobgui_list_base_get_enable_rubberband (BOBGUI_LIST_BASE (self)))
    return;

  bobgui_list_base_set_enable_rubberband (BOBGUI_LIST_BASE (self), enable_rubberband);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_ENABLE_RUBBERBAND]);
}

/**
 * bobgui_list_view_get_enable_rubberband:
 * @self: a listview
 *
 * Returns whether rows can be selected by dragging with the mouse.
 *
 * Returns: true if rubberband selection is enabled
 */
gboolean
bobgui_list_view_get_enable_rubberband (BobguiListView *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_VIEW (self), FALSE);

  return bobgui_list_base_get_enable_rubberband (BOBGUI_LIST_BASE (self));
}

/**
 * bobgui_list_view_set_tab_behavior:
 * @self: a listview
 * @tab_behavior: The desired tab behavior
 *
 * Sets the <kbd>Tab</kbd> key behavior.
 *
 * This influences how the <kbd>Tab</kbd> and
 * <kbd>Shift</kbd>+<kbd>Tab</kbd> keys move the
 * focus in the listview.
 *
 * Since: 4.12
 */
void
bobgui_list_view_set_tab_behavior (BobguiListView        *self,
                                BobguiListTabBehavior  tab_behavior)
{
  g_return_if_fail (BOBGUI_IS_LIST_VIEW (self));

  if (tab_behavior == bobgui_list_base_get_tab_behavior (BOBGUI_LIST_BASE (self)))
    return;

  bobgui_list_base_set_tab_behavior (BOBGUI_LIST_BASE (self), tab_behavior);

  g_object_notify_by_pspec (G_OBJECT (self), properties[PROP_TAB_BEHAVIOR]);
}

/**
 * bobgui_list_view_get_tab_behavior:
 * @self: a listview
 *
 * Gets the behavior set for the <kbd>Tab</kbd> key.
 *
 * Returns: The behavior of the <kbd>Tab</kbd> key
 *
 * Since: 4.12
 */
BobguiListTabBehavior
bobgui_list_view_get_tab_behavior (BobguiListView *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_VIEW (self), FALSE);

  return bobgui_list_base_get_tab_behavior (BOBGUI_LIST_BASE (self));
}

/**
 * bobgui_list_view_scroll_to:
 * @self: a listview
 * @pos: position of the item. Must be less than the number of
 *   items in the view.
 * @flags: actions to perform
 * @scroll: (nullable) (transfer full): details of how to perform
 *   the scroll operation or %NULL to scroll into view
 *
 * Scrolls to the item at the given position and performs the actions
 * specified in @flags.
 *
 * This function works no matter if the listview is shown or focused.
 * If it isn't, then the changes will take effect once that happens.
 *
 * Since: 4.12
 */
void
bobgui_list_view_scroll_to (BobguiListView        *self,
                         guint               pos,
                         BobguiListScrollFlags  flags,
                         BobguiScrollInfo      *scroll)
{
  g_return_if_fail (BOBGUI_IS_LIST_VIEW (self));
  g_return_if_fail (pos < bobgui_list_base_get_n_items (BOBGUI_LIST_BASE (self)));

  bobgui_list_base_scroll_to (BOBGUI_LIST_BASE (self), pos, flags, scroll);
}
