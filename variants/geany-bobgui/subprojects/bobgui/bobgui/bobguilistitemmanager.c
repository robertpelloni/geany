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

#include "bobguilistitemmanagerprivate.h"

#include "bobguilistitembaseprivate.h"
#include "bobguilistitemwidgetprivate.h"
#include "bobguisectionmodel.h"
#include "bobguiwidgetprivate.h"

typedef struct _BobguiListItemChange BobguiListItemChange;

struct _BobguiListItemManager
{
  GObject parent_instance;

  BobguiWidget *widget;
  BobguiSelectionModel *model;
  gboolean has_sections;

  BobguiRbTree *items;
  GSList *trackers;

  BobguiListTile * (* split_func) (BobguiWidget *, BobguiListTile *, guint);
  BobguiListItemBase * (* create_widget) (BobguiWidget *);
  void (* prepare_section) (BobguiWidget *, BobguiListTile *, guint);
  BobguiListHeaderBase * (* create_header_widget) (BobguiWidget *);
};

struct _BobguiListItemManagerClass
{
  GObjectClass parent_class;
};

struct _BobguiListItemTracker
{
  guint position;
  BobguiListItemBase *widget;
  guint n_before;
  guint n_after;
};

struct _BobguiListItemChange
{
  GHashTable *deleted_items;
  GQueue recycled_items;
  GQueue recycled_headers;
};

G_DEFINE_TYPE (BobguiListItemManager, bobgui_list_item_manager, G_TYPE_OBJECT)

static void
bobgui_list_item_change_init (BobguiListItemChange *change)
{
  change->deleted_items = NULL;
  g_queue_init (&change->recycled_items);
  g_queue_init (&change->recycled_headers);
}

static void
bobgui_list_item_change_finish (BobguiListItemChange *change)
{
  BobguiWidget *widget;

  g_clear_pointer (&change->deleted_items, g_hash_table_destroy);

  while ((widget = g_queue_pop_head (&change->recycled_items)))
    bobgui_widget_unparent (widget);
  while ((widget = g_queue_pop_head (&change->recycled_headers)))
    bobgui_widget_unparent (widget);
}

static void
bobgui_list_item_change_recycle (BobguiListItemChange *change,
                              BobguiListItemBase   *widget)
{
  g_queue_push_tail (&change->recycled_items, widget);
}

static void
bobgui_list_item_change_clear_header (BobguiListItemChange  *change,
                                   BobguiWidget         **widget)
{
  if (*widget == NULL)
    return;

  g_assert (BOBGUI_IS_LIST_HEADER_BASE (*widget));
  g_queue_push_tail (&change->recycled_headers, *widget);
  *widget = NULL;
}

static void
bobgui_list_item_change_release (BobguiListItemChange *change,
                              BobguiListItemBase   *widget)
{
  if (change->deleted_items == NULL)
    change->deleted_items = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) bobgui_widget_unparent);

  if (!g_hash_table_replace (change->deleted_items, bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (widget)), widget))
    {
      g_warning ("Duplicate item detected in list. Picking one randomly.");
    }
}

static BobguiListItemBase *
bobgui_list_item_change_find (BobguiListItemChange *change,
                           gpointer           item)
{
  gpointer result;

  if (change->deleted_items && g_hash_table_steal_extended (change->deleted_items, item, NULL, &result))
    return result;

  return NULL;
}

static BobguiListItemBase *
bobgui_list_item_change_get (BobguiListItemChange *change,
                          gpointer           item)
{
  BobguiListItemBase *result;

  result = bobgui_list_item_change_find (change, item);
  if (result)
    return result;

  result = g_queue_pop_head (&change->recycled_items);
  if (result)
    return result;

  return NULL;
}

static BobguiListHeaderBase *
bobgui_list_item_change_get_header (BobguiListItemChange *change)
{
  return g_queue_pop_head (&change->recycled_headers);
}

static void
potentially_empty_rectangle_union (cairo_rectangle_int_t       *self,
                                   const cairo_rectangle_int_t *area)
{
  if (area->width <= 0 || area->height <= 0)
    return;

  if (self->width <= 0 || self->height <= 0)
    {
      *self = *area;
      return;
    }

  gdk_rectangle_union (self, area, self);
}

static void
bobgui_list_item_manager_augment_node (BobguiRbTree *tree,
                                    gpointer   node_augment,
                                    gpointer   node,
                                    gpointer   left,
                                    gpointer   right)
{
  BobguiListTile *tile = node;
  BobguiListTileAugment *aug = node_augment;

  aug->n_items = tile->n_items;
  aug->area = tile->area;

  switch (tile->type)
  {
    case BOBGUI_LIST_TILE_HEADER:
    case BOBGUI_LIST_TILE_UNMATCHED_HEADER:
      aug->has_header = TRUE;
      aug->has_footer = FALSE;
      break;
    case BOBGUI_LIST_TILE_FOOTER:
    case BOBGUI_LIST_TILE_UNMATCHED_FOOTER:
      aug->has_header = FALSE;
      aug->has_footer = TRUE;
      break;
    case BOBGUI_LIST_TILE_ITEM:
    case BOBGUI_LIST_TILE_REMOVED:
      aug->has_header = FALSE;
      aug->has_footer = FALSE;
      break;
    default:
      g_assert_not_reached ();
      break;
  }

  if (left)
    {
      BobguiListTileAugment *left_aug = bobgui_rb_tree_get_augment (tree, left);

      aug->n_items += left_aug->n_items;
      aug->has_header |= left_aug->has_header;
      aug->has_footer |= left_aug->has_footer;
      potentially_empty_rectangle_union (&aug->area, &left_aug->area);
    }

  if (right)
    {
      BobguiListTileAugment *right_aug = bobgui_rb_tree_get_augment (tree, right);

      aug->n_items += right_aug->n_items;
      aug->has_header |= right_aug->has_header;
      aug->has_footer |= right_aug->has_footer;
      potentially_empty_rectangle_union (&aug->area, &right_aug->area);
    }
}

static void
bobgui_list_item_manager_clear_node (gpointer _tile)
{
  BobguiListTile *tile G_GNUC_UNUSED = _tile;

  g_assert (tile->widget == NULL);
}

BobguiListItemManager *
bobgui_list_item_manager_new (BobguiWidget          *widget,
                           BobguiListTile *       (* split_func) (BobguiWidget *, BobguiListTile *, guint),
                           BobguiListItemBase *   (* create_widget) (BobguiWidget *),
                           void                (* prepare_section) (BobguiWidget *, BobguiListTile *, guint),
                           BobguiListHeaderBase * (* create_header_widget) (BobguiWidget *))
{
  BobguiListItemManager *self;

  g_return_val_if_fail (BOBGUI_IS_WIDGET (widget), NULL);

  self = g_object_new (BOBGUI_TYPE_LIST_ITEM_MANAGER, NULL);

  /* not taking a ref because the widget refs us */
  self->widget = widget;
  self->split_func = split_func;
  self->create_widget = create_widget;
  self->prepare_section = prepare_section;
  self->create_header_widget = create_header_widget;

  self->items = bobgui_rb_tree_new_for_size (sizeof (BobguiListTile),
                                          sizeof (BobguiListTileAugment),
                                          bobgui_list_item_manager_augment_node,
                                          bobgui_list_item_manager_clear_node,
                                          NULL);

  return self;
}

static gboolean
bobgui_list_item_manager_has_sections (BobguiListItemManager *self)
{
  if (self->model == NULL || !self->has_sections)
    return FALSE;

  return BOBGUI_IS_SECTION_MODEL (self->model);
}

void
bobgui_list_item_manager_get_tile_bounds (BobguiListItemManager *self,
                                       GdkRectangle       *out_bounds)
{
  BobguiListTile *tile;
  BobguiListTileAugment *aug;

  tile = bobgui_rb_tree_get_root (self->items);
  if (tile == NULL)
    {
      *out_bounds = (GdkRectangle) { 0, 0, 0, 0 };
      return;
    }

  aug = bobgui_rb_tree_get_augment (self->items, tile);
  *out_bounds = aug->area;
}

gpointer
bobgui_list_item_manager_get_first (BobguiListItemManager *self)
{
  return bobgui_rb_tree_get_first (self->items);
}

gpointer
bobgui_list_item_manager_get_last (BobguiListItemManager *self)
{
  return bobgui_rb_tree_get_last (self->items);
}

gpointer
bobgui_list_item_manager_get_root (BobguiListItemManager *self)
{
  return bobgui_rb_tree_get_root (self->items);
}

/*
 * bobgui_list_item_manager_get_nth:
 * @self: a `BobguiListItemManager`
 * @position: position of the item
 * @offset: (out): offset into the returned tile
 *
 * Looks up the BobguiListTile that represents @position.
 *
 * If a the returned tile represents multiple rows, the @offset into
 * the returned tile for @position will be set. If the returned tile
 * represents a row with an existing widget, @offset will always be 0.
 *
 * Returns: (type BobguiListTile): the tile for @position or
 *   %NULL if position is out of range
 **/
gpointer
bobgui_list_item_manager_get_nth (BobguiListItemManager *self,
                               guint               position,
                               guint              *offset)
{
  BobguiListTile *tile, *tmp;

  tile = bobgui_rb_tree_get_root (self->items);

  while (tile)
    {
      tmp = bobgui_rb_tree_node_get_left (tile);
      if (tmp)
        {
          BobguiListTileAugment *aug = bobgui_rb_tree_get_augment (self->items, tmp);
          if (position < aug->n_items)
            {
              tile = tmp;
              continue;
            }
          position -= aug->n_items;
        }

      if (position < tile->n_items)
        break;
      position -= tile->n_items;

      tile = bobgui_rb_tree_node_get_right (tile);
    }

  if (offset)
    *offset = tile ? position : 0;

  return tile;
}

static BobguiListTile *
bobgui_list_tile_get_header (BobguiListItemManager *self,
                          BobguiListTile        *tile)
{
  BobguiListTileAugment *aug;
  BobguiListTile *other;
  gboolean check_right = FALSE;

  while (TRUE)
    {
      if (check_right)
        {
          other = bobgui_rb_tree_node_get_right (tile);
          if (other)
            {
              aug = bobgui_rb_tree_get_augment (self->items, other);
              if (aug->has_header)
                {
                  check_right = TRUE;
                  tile = other;
                  continue;
                }
            }
        }

      if (tile->type == BOBGUI_LIST_TILE_HEADER ||
          tile->type == BOBGUI_LIST_TILE_UNMATCHED_HEADER)
        return tile;

      other = bobgui_rb_tree_node_get_left (tile);
      if (other)
        {
          aug = bobgui_rb_tree_get_augment (self->items, other);
          if (aug->has_header)
            {
              check_right = TRUE;
              tile = other;
              continue;
            }
        }

      while ((other = bobgui_rb_tree_node_get_parent (tile)))
        {
          if (bobgui_rb_tree_node_get_right (other) == tile)
            break;
          tile = other;
        }
      tile = other;
      check_right = FALSE;
    }
}

static BobguiListTile *
bobgui_list_tile_get_footer (BobguiListItemManager *self,
                          BobguiListTile        *tile)
{
  BobguiListTileAugment *aug;
  BobguiListTile *other;
  gboolean check_left = FALSE;

  while (TRUE)
    {
      if (check_left)
        {
          other = bobgui_rb_tree_node_get_left (tile);
          if (other)
            {
              aug = bobgui_rb_tree_get_augment (self->items, other);
              if (aug->has_footer)
                {
                  check_left = TRUE;
                  tile = other;
                  continue;
                }
            }
        }

      if (tile->type == BOBGUI_LIST_TILE_FOOTER ||
          tile->type == BOBGUI_LIST_TILE_UNMATCHED_FOOTER)
        return tile;

      other = bobgui_rb_tree_node_get_right (tile);
      if (other)
        {
          aug = bobgui_rb_tree_get_augment (self->items, other);
          if (aug->has_footer)
            {
              check_left = TRUE;
              tile = other;
              continue;
            }
        }

      while ((other = bobgui_rb_tree_node_get_parent (tile)))
        {
          if (bobgui_rb_tree_node_get_left (other) == tile)
            break;
          tile = other;
        }
      tile = other;
      check_left = FALSE;
    }
}

/* This computes Manhattan distance */
static int
rectangle_distance (const cairo_rectangle_int_t *rect,
                    int                          x,
                    int                          y)
{
  int x_dist, y_dist;

  if (rect->x > x)
    x_dist = rect->x - x;
  else if (rect->x + rect->width < x)
    x_dist = x - (rect->x + rect->width);
  else
    x_dist = 0;

  if (rect->y > y)
    y_dist = rect->y - y;
  else if (rect->y + rect->height < y)
    y_dist = y - (rect->y + rect->height);
  else
    y_dist = 0;

  return x_dist + y_dist;
}

static BobguiListTile *
bobgui_list_tile_get_tile_at (BobguiListItemManager *self,
                           BobguiListTile        *tile,
                           int                 x,
                           int                 y,
                           int                *distance)
{
  BobguiListTileAugment *aug;
  BobguiListTile *left, *right, *result;
  int dist, left_dist, right_dist;

  left = bobgui_rb_tree_node_get_left (tile);
  if (left)
    {
      aug = bobgui_list_tile_get_augment (self, left);
      left_dist = rectangle_distance (&aug->area, x, y);
    }
  else
    left_dist = *distance;
  right = bobgui_rb_tree_node_get_right (tile);
  if (right)
    {
      aug = bobgui_list_tile_get_augment (self, right);
      right_dist = rectangle_distance (&aug->area, x, y);
    }
  else
    right_dist = *distance;

  dist = rectangle_distance (&tile->area, x, y);
  result = NULL;

  while (TRUE)
    {
      if (dist < left_dist && dist < right_dist)
        {
          if (dist >= *distance)
            return result;

          *distance = dist;
          return tile;
        }

      if (left_dist < right_dist)
        {
          if (left_dist >= *distance)
            return result;

          left = bobgui_list_tile_get_tile_at (self, left, x, y, distance);
          if (left)
            result = left;
          left_dist = G_MAXINT;
        }
      else
        {
          if (right_dist >= *distance)
            return result;

          right = bobgui_list_tile_get_tile_at (self, right, x, y, distance);
          if (right)
            result = right;
          right_dist = G_MAXINT;
        }
    }
}

/*
 * bobgui_list_item_manager_get_nearest_tile:
 * @self: a BobguiListItemManager
 * @x: x coordinate of tile
 * @y: y coordinate of tile
 *
 * Finds the tile closest to the coordinates at (x, y). If no
 * tile occupies the coordinates (for example, if the tile is out of bounds),
 * Manhattan distance is used to find the nearest tile.
 *
 * If multiple tiles have the same distance, the one closest to the start
 * will be returned.
 *
 * Returns: (nullable): The tile nearest to (x, y) or NULL if there are no tiles
 **/
BobguiListTile *
bobgui_list_item_manager_get_nearest_tile (BobguiListItemManager *self,
                                        int                 x,
                                        int                 y)
{
  BobguiListTile *root;
  int distance = G_MAXINT;

  root = bobgui_list_item_manager_get_root (self);
  if (root == NULL)
    return NULL;

  return bobgui_list_tile_get_tile_at (self, root, x, y, &distance);
}

guint
bobgui_list_tile_get_position (BobguiListItemManager *self,
                            BobguiListTile        *tile)
{
  BobguiListTile *parent, *left;
  int pos;

  left = bobgui_rb_tree_node_get_left (tile);
  if (left)
    {
      BobguiListTileAugment *aug = bobgui_rb_tree_get_augment (self->items, left);
      pos = aug->n_items;
    }
  else
    {
      pos = 0;
    }

  for (parent = bobgui_rb_tree_node_get_parent (tile);
       parent != NULL;
       parent = bobgui_rb_tree_node_get_parent (tile))
    {
      left = bobgui_rb_tree_node_get_left (parent);

      if (left != tile)
        {
          if (left)
            {
              BobguiListTileAugment *aug = bobgui_rb_tree_get_augment (self->items, left);
              pos += aug->n_items;
            }
          pos += parent->n_items;
        }

      tile = parent;
    }

  return pos;
}

gpointer
bobgui_list_tile_get_augment (BobguiListItemManager *self,
                           BobguiListTile        *tile)
{
  return bobgui_rb_tree_get_augment (self->items, tile);
}

static BobguiListTile *
bobgui_list_tile_get_next_skip (BobguiListTile *tile)
{
  for (tile = bobgui_rb_tree_node_get_next (tile);
       tile && tile->type == BOBGUI_LIST_TILE_REMOVED;
       tile = bobgui_rb_tree_node_get_next (tile))
    { }

  return tile;
}

static BobguiListTile *
bobgui_list_tile_get_previous_skip (BobguiListTile *tile)
{
  for (tile = bobgui_rb_tree_node_get_previous (tile);
       tile && tile->type == BOBGUI_LIST_TILE_REMOVED;
       tile = bobgui_rb_tree_node_get_previous (tile))
    { }

  return tile;
}

/*
 * bobgui_list_tile_set_area:
 * @self: the list item manager
 * @tile: tile to set area for
 * @area: (nullable): area to set or NULL to clear
 *     the area
 *
 * Updates the area of the tile.
 *
 * The area is given in the internal coordinate system,
 * so the x/y flip due to orientation and the left/right
 * flip for RTL languages will happen later.
 *
 * This function should only be called from inside size_allocate().
 **/
void
bobgui_list_tile_set_area (BobguiListItemManager          *self,
                        BobguiListTile                 *tile,
                        const cairo_rectangle_int_t *area)
{
  cairo_rectangle_int_t empty_area = { 0, 0, 0, 0 };

  if (!area)
    area = &empty_area;

  if (gdk_rectangle_equal (&tile->area, area))
    return;

  tile->area = *area;
  bobgui_rb_tree_node_mark_dirty (tile);
}

void
bobgui_list_tile_set_area_position (BobguiListItemManager *self,
                                 BobguiListTile        *tile,
                                 int                 x,
                                 int                 y)
{
  if (tile->area.x == x && tile->area.y == y)
    return;

  tile->area.x = x;
  tile->area.y = y;
  bobgui_rb_tree_node_mark_dirty (tile);
}

void
bobgui_list_tile_set_area_size (BobguiListItemManager *self,
                             BobguiListTile        *tile,
                             int                 width,
                             int                 height)
{
  if (tile->area.width == width && tile->area.height == height)
    return;

  tile->area.width = width;
  tile->area.height = height;
  bobgui_rb_tree_node_mark_dirty (tile);
}

static void
bobgui_list_tile_set_type (BobguiListTile     *tile,
                        BobguiListTileType  type)
{
  g_assert (tile != NULL);
  if (tile->type == type)
    return;

  g_assert (tile->widget == NULL);
  tile->type = type;
  bobgui_rb_tree_node_mark_dirty (tile);
}

static void
bobgui_list_item_tracker_unset_position (BobguiListItemManager *self,
                                      BobguiListItemTracker *tracker)
{
  tracker->widget = NULL;
  tracker->position = BOBGUI_INVALID_LIST_POSITION;
}

static gboolean
bobgui_list_item_tracker_query_range (BobguiListItemManager *self,
                                   BobguiListItemTracker *tracker,
                                   guint               n_items,
                                   guint              *out_start,
                                   guint              *out_n_items)
{
  /* We can't look at tracker->widget here because we might not
   * have set it yet.
   */
  if (tracker->position == BOBGUI_INVALID_LIST_POSITION)
    return FALSE;

  /* This is magic I made up that is meant to be both
   * correct and doesn't overflow when start and/or end are close to 0 or
   * close to max.
   * But beware, I didn't test it.
   */
  *out_n_items = tracker->n_before + tracker->n_after + 1;
  *out_n_items = MIN (*out_n_items, n_items);

  *out_start = MAX (tracker->position, tracker->n_before) - tracker->n_before;
  *out_start = MIN (*out_start, n_items - *out_n_items);

  return TRUE;
}

static void
bobgui_list_item_query_tracked_range (BobguiListItemManager *self,
                                   guint               n_items,
                                   guint               position,
                                   guint              *out_n_items,
                                   gboolean           *out_tracked)
{
  GSList *l;
  guint tracker_start, tracker_n_items;

  g_assert (position < n_items);

  *out_tracked = FALSE;
  *out_n_items = n_items - position;

  /* step 1: Check if position is tracked */

  for (l = self->trackers; l; l = l->next)
    {
      if (!bobgui_list_item_tracker_query_range (self, l->data, n_items, &tracker_start, &tracker_n_items))
        continue;

      if (tracker_start > position)
        {
          *out_n_items = MIN (*out_n_items, tracker_start - position);
        }
      else if (tracker_start + tracker_n_items <= position)
        {
          /* do nothing */
        }
      else
        {
          *out_tracked = TRUE;
          *out_n_items = tracker_start + tracker_n_items - position;
          break;
        }
    }

  /* If nothing's tracked, we're done */
  if (!*out_tracked)
    return;

  /* step 2: make the tracked range as large as possible
   * NB: This is O(N_TRACKERS^2), but the number of trackers should be <5 */
restart:
  for (l = self->trackers; l; l = l->next)
    {
      if (!bobgui_list_item_tracker_query_range (self, l->data, n_items, &tracker_start, &tracker_n_items))
        continue;

      if (tracker_start + tracker_n_items <= position + *out_n_items)
        continue;
      if (tracker_start > position + *out_n_items)
        continue;

      if (*out_n_items + position < tracker_start + tracker_n_items)
        {
          *out_n_items = tracker_start + tracker_n_items - position;
          goto restart;
        }
    }
}

static BobguiListTile *
bobgui_list_item_manager_ensure_split (BobguiListItemManager *self,
                                    BobguiListTile        *tile,
                                    guint               n_items)
{
  return self->split_func (self->widget, tile, n_items);
}

static void
bobgui_list_item_manager_remove_items (BobguiListItemManager *self,
                                    BobguiListItemChange  *change,
                                    guint               position,
                                    guint               n_items)
{
  BobguiListTile *tile, *header;
  guint offset;

  if (n_items == 0)
    return;

  tile = bobgui_list_item_manager_get_nth (self, position, &offset);
  if (offset)
    tile = bobgui_list_item_manager_ensure_split (self, tile, offset);
  header = bobgui_list_tile_get_previous_skip (tile);
  if (header != NULL &&
      (header->type != BOBGUI_LIST_TILE_HEADER && header->type != BOBGUI_LIST_TILE_UNMATCHED_HEADER))
    header = NULL;

  while (n_items > 0)
    {
      switch (tile->type)
        {
        case BOBGUI_LIST_TILE_HEADER:
        case BOBGUI_LIST_TILE_UNMATCHED_HEADER:
          g_assert (header == NULL);
          header = tile;
          break;

        case BOBGUI_LIST_TILE_FOOTER:
        case BOBGUI_LIST_TILE_UNMATCHED_FOOTER:
          if (header)
            {
              bobgui_list_item_change_clear_header (change, &header->widget);
              bobgui_list_tile_set_type (header, BOBGUI_LIST_TILE_REMOVED);
              bobgui_list_tile_set_type (tile, BOBGUI_LIST_TILE_REMOVED);
              header = NULL;
            }
          break;

        case BOBGUI_LIST_TILE_ITEM:
          if (tile->n_items > n_items)
            {
              bobgui_list_item_manager_ensure_split (self, tile, n_items);
              g_assert (tile->n_items <= n_items);
            }
          if (tile->widget)
            bobgui_list_item_change_release (change, BOBGUI_LIST_ITEM_BASE (tile->widget));
          tile->widget = NULL;
          n_items -= tile->n_items;
          tile->n_items = 0;
          bobgui_list_tile_set_type (tile, BOBGUI_LIST_TILE_REMOVED);
          break;

        case BOBGUI_LIST_TILE_REMOVED:
        default:
          g_assert_not_reached ();
          break;
        }

      tile = bobgui_list_tile_get_next_skip (tile);
    }

  if (header)
    {
      if (tile->type == BOBGUI_LIST_TILE_FOOTER || tile->type == BOBGUI_LIST_TILE_UNMATCHED_FOOTER)
        {
          bobgui_list_item_change_clear_header (change, &header->widget);
          bobgui_list_tile_set_type (header, BOBGUI_LIST_TILE_REMOVED);
          bobgui_list_tile_set_type (tile, BOBGUI_LIST_TILE_REMOVED);
        }
    }

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self->widget));
}

static void
bobgui_list_item_manager_add_items (BobguiListItemManager *self,
                                 BobguiListItemChange  *change,
                                 guint               position,
                                 guint               n_items)
{
  BobguiListTile *tile;
  guint offset;
  gboolean has_sections;

  if (n_items == 0)
    return;

  has_sections = bobgui_list_item_manager_has_sections (self);

  tile = bobgui_list_item_manager_get_nth (self, position, &offset);
  if (tile == NULL)
    {
      /* at end of list, pick the footer */
      for (tile = bobgui_rb_tree_get_last (self->items);
           tile && tile->type == BOBGUI_LIST_TILE_REMOVED;
           tile = bobgui_rb_tree_node_get_previous (tile))
        { }

      if (tile == NULL)
        {
          /* empty list, there isn't even a footer yet */
          tile = bobgui_rb_tree_insert_after (self->items, NULL);
          tile->type = BOBGUI_LIST_TILE_UNMATCHED_HEADER;

          tile = bobgui_rb_tree_insert_after (self->items, tile);
          tile->type = BOBGUI_LIST_TILE_UNMATCHED_FOOTER;
        }
      else if (has_sections && tile->type == BOBGUI_LIST_TILE_FOOTER)
        {
          BobguiListTile *header;

          bobgui_list_tile_set_type (tile, BOBGUI_LIST_TILE_UNMATCHED_FOOTER);

          header = bobgui_list_tile_get_header (self, tile);
          bobgui_list_item_change_clear_header (change, &header->widget);
          bobgui_list_tile_set_type (header, BOBGUI_LIST_TILE_UNMATCHED_HEADER);
        }
    }
  if (offset)
    tile = bobgui_list_item_manager_ensure_split (self, tile, offset);

  tile = bobgui_rb_tree_insert_before (self->items, tile);
  tile->type = BOBGUI_LIST_TILE_ITEM;
  tile->n_items = n_items;
  bobgui_rb_tree_node_mark_dirty (tile);

  if (has_sections)
    {
      BobguiListTile *section = bobgui_list_tile_get_previous_skip (tile);

      if (section != NULL && section->type == BOBGUI_LIST_TILE_HEADER)
        {
          guint start, end;
          BobguiListTile *footer = bobgui_list_tile_get_footer (self, section);
          BobguiListTile *previous_footer = bobgui_list_tile_get_previous_skip (section);

          bobgui_section_model_get_section (BOBGUI_SECTION_MODEL (self->model), position, &start, &end);

          if (previous_footer != NULL && previous_footer->type == BOBGUI_LIST_TILE_FOOTER &&
              position > start && position < end)
          {
            bobgui_list_item_change_clear_header (change, &section->widget);
            bobgui_list_tile_set_type (section, BOBGUI_LIST_TILE_REMOVED);
            bobgui_list_tile_set_type (previous_footer, BOBGUI_LIST_TILE_REMOVED);

            section = bobgui_list_tile_get_header (self, previous_footer);
          }

          bobgui_list_item_change_clear_header (change, &section->widget);
          bobgui_list_tile_set_type (section,
                                  BOBGUI_LIST_TILE_UNMATCHED_HEADER);
          bobgui_list_tile_set_type (footer,
                                  BOBGUI_LIST_TILE_UNMATCHED_FOOTER);
        }
    }

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self->widget));
}

static gboolean
bobgui_list_item_manager_merge_list_items (BobguiListItemManager *self,
                                        BobguiListTile        *first,
                                        BobguiListTile        *second)
{
  if (first->widget || second->widget ||
      first->type != BOBGUI_LIST_TILE_ITEM || second->type != BOBGUI_LIST_TILE_ITEM)
    return FALSE;

  first->n_items += second->n_items;
  bobgui_rb_tree_node_mark_dirty (first);
  bobgui_rb_tree_remove (self->items, second);

  return TRUE;
}

/*
 * bobgui_list_tile_split:
 * @self: the listitemmanager
 * @tile: a tile to split into two
 * @n_items: number of items to keep in tile
 *
 * Splits the given tile into two tiles. The original
 * tile will remain with @n_items items, the remaining
 * items will be given to the new tile, which will be
 * inserted after the tile.
 *
 * It is not valid for either tile to have 0 items after
 * the split.
 *
 * This function does not update the tiles' areas.
 *
 * Returns: The new tile
 **/
BobguiListTile *
bobgui_list_tile_split (BobguiListItemManager *self,
                     BobguiListTile        *tile,
                     guint               n_items)
{
  BobguiListTile *result;

  g_assert (n_items > 0);
  g_assert (n_items < tile->n_items);
  g_assert (tile->type == BOBGUI_LIST_TILE_ITEM);

  result = bobgui_rb_tree_insert_after (self->items, tile);
  result->type = BOBGUI_LIST_TILE_ITEM;
  result->n_items = tile->n_items - n_items;
  tile->n_items = n_items;
  bobgui_rb_tree_node_mark_dirty (tile);

  return result;
}

/*
 * bobgui_list_tile_gc:
 * @self: the listitemmanager
 * @tile: a tile or NULL
 *
 * Tries to get rid of tiles when they aren't needed anymore,
 * either because their referenced listitems were deleted or
 * because they can be merged with the next item(s).
 *
 * Note that this only looks forward, but never backward.
 *
 * Returns: The next tile or NULL if everything was gc'ed
 **/
static BobguiListTile *
bobgui_list_tile_gc (BobguiListItemManager *self,
                  BobguiListTile        *tile)
{
  BobguiListTile *next;

  if (tile == NULL)
    return NULL;

  while (tile)
    {
      next = bobgui_rb_tree_node_get_next (tile);
      while (next && next->type == BOBGUI_LIST_TILE_REMOVED)
        {
          bobgui_rb_tree_remove (self->items, next);
          next = bobgui_rb_tree_node_get_next (tile);
        }

      switch (tile->type)
        {
        case BOBGUI_LIST_TILE_ITEM:
          g_assert (tile->n_items > 0);
          if (next == NULL)
            break;
          if (bobgui_list_item_manager_merge_list_items (self, tile, next))
            continue;
          break;

        case BOBGUI_LIST_TILE_HEADER:
        case BOBGUI_LIST_TILE_FOOTER:
        case BOBGUI_LIST_TILE_UNMATCHED_HEADER:
        case BOBGUI_LIST_TILE_UNMATCHED_FOOTER:
          break;

        case BOBGUI_LIST_TILE_REMOVED:
          bobgui_rb_tree_remove (self->items, tile);
          tile = next;
          continue;

        default:
          g_assert_not_reached ();
          break;
        }

      break;
    }

  return tile;
}

/*
 * bobgui_list_item_manager_gc_tiles:
 * @self: the listitemmanager
 *
 * Removes all tiles of type BOBGUI_LIST_TILE_REMOVED
 * and merges item tiles as much as possible.
 *
 * This function does not update the tiles' areas.
 */
void
bobgui_list_item_manager_gc_tiles (BobguiListItemManager *self)
{
  BobguiListTile *tile;

  for (tile = bobgui_list_tile_gc (self, bobgui_list_item_manager_get_first (self));
       tile != NULL;
       tile = bobgui_list_tile_gc (self, bobgui_rb_tree_node_get_next (tile)))
    {
    }
}

static void
bobgui_list_item_manager_release_items (BobguiListItemManager *self,
                                     BobguiListItemChange  *change)
{
  BobguiListTile *tile, *header;
  guint position, i, n_items, query_n_items;
  gboolean tracked, deleted_section;

  n_items = g_list_model_get_n_items (G_LIST_MODEL (self->model));
  position = 0;

  while (position < n_items)
    {
      bobgui_list_item_query_tracked_range (self, n_items, position, &query_n_items, &tracked);
      if (tracked)
        {
          position += query_n_items;
          continue;
        }

      deleted_section = FALSE;
      tile = bobgui_list_item_manager_get_nth (self, position, &i);
      if (i == 0)
        {
          header = bobgui_list_tile_get_previous_skip (tile);
          if (header && !bobgui_list_tile_is_header (header))
            header = NULL;
        }
      else
        header = NULL;
      i = position - i;
      while (i < position + query_n_items)
        {
          g_assert (tile != NULL);
          switch (tile->type)
            {
            case BOBGUI_LIST_TILE_ITEM:
              if (tile->widget)
                {
                  bobgui_list_item_change_recycle (change, BOBGUI_LIST_ITEM_BASE (tile->widget));
                  tile->widget = NULL;
                }
              i += tile->n_items;
              break;

            case BOBGUI_LIST_TILE_HEADER:
            case BOBGUI_LIST_TILE_UNMATCHED_HEADER:
              g_assert (deleted_section);
              bobgui_list_item_change_clear_header (change, &tile->widget);
              G_GNUC_FALLTHROUGH;
            case BOBGUI_LIST_TILE_FOOTER:
            case BOBGUI_LIST_TILE_UNMATCHED_FOOTER:
              bobgui_list_tile_set_type (tile, BOBGUI_LIST_TILE_REMOVED);
              deleted_section = TRUE;
              header = NULL;
              break;

            case BOBGUI_LIST_TILE_REMOVED:
            default:
              g_assert_not_reached ();
              break;
            }
          tile = bobgui_list_tile_get_next_skip (tile);
        }
      if (header && bobgui_list_tile_is_footer (tile))
        deleted_section = TRUE;
      if (deleted_section)
        {
          if (header == NULL)
            header = bobgui_list_tile_get_header (self, tile);
          bobgui_list_item_change_clear_header (change, &header->widget);
          bobgui_list_tile_set_type (header, BOBGUI_LIST_TILE_UNMATCHED_HEADER);

          tile = bobgui_list_tile_get_footer (self, tile);
          bobgui_list_tile_set_type (tile, BOBGUI_LIST_TILE_UNMATCHED_FOOTER);
        }
      position += query_n_items;
    }
}

static BobguiListTile *
bobgui_list_item_manager_insert_section (BobguiListItemManager *self,
                                      guint               pos,
                                      BobguiListTileType     footer_type,
                                      BobguiListTileType     header_type)
{
  BobguiListTile *tile, *footer, *header;
  guint offset;

  tile = bobgui_list_item_manager_get_nth (self, pos, &offset);
  if (tile == NULL)
    {
      if (footer_type == BOBGUI_LIST_TILE_FOOTER)
        {
          footer = bobgui_rb_tree_get_last (self->items);
          if (footer->type != BOBGUI_LIST_TILE_FOOTER && footer->type != BOBGUI_LIST_TILE_UNMATCHED_FOOTER)
            footer = bobgui_list_tile_get_previous_skip (footer);
          bobgui_list_tile_set_type (footer, footer_type);
        }
      return NULL;
    }

  if (offset)
    tile = bobgui_list_item_manager_ensure_split (self, tile, offset);

  header = bobgui_list_tile_get_previous_skip (tile);
  if (header != NULL &&
      (header->type == BOBGUI_LIST_TILE_HEADER || header->type == BOBGUI_LIST_TILE_UNMATCHED_HEADER))
    {
      if (header_type == BOBGUI_LIST_TILE_HEADER)
        bobgui_list_tile_set_type (header, header_type);
      if (footer_type == BOBGUI_LIST_TILE_FOOTER)
        {
          footer = bobgui_list_tile_get_previous_skip (header);
          if (footer)
            bobgui_list_tile_set_type (footer, footer_type);
        }
    }
  else
    {
      self->prepare_section (self->widget, tile, pos);

      header = bobgui_rb_tree_insert_before (self->items, tile);
      bobgui_list_tile_set_type (header, header_type);
      footer = bobgui_rb_tree_insert_before (self->items, header);
      bobgui_list_tile_set_type (footer, footer_type);
    }

  return header;
}

static BobguiWidget *
bobgui_list_tile_find_widget_before (BobguiListTile *tile)
{
  BobguiListTile *other;

  for (other = bobgui_rb_tree_node_get_previous (tile);
       other;
       other = bobgui_rb_tree_node_get_previous (other))
     {
       if (other->widget)
         return other->widget;
     }

  return NULL;
}

static void
bobgui_list_item_manager_ensure_items (BobguiListItemManager *self,
                                    BobguiListItemChange  *change,
                                    guint               update_start,
                                    int                 update_diff)
{
  BobguiListTile *tile, *header;
  BobguiWidget *insert_after;
  guint position, i, n_items, query_n_items, offset;
  gboolean tracked, has_sections;

  if (self->model == NULL)
    return;

  n_items = g_list_model_get_n_items (G_LIST_MODEL (self->model));
  position = 0;
  has_sections = bobgui_list_item_manager_has_sections (self);

  bobgui_list_item_manager_release_items (self, change);

  while (position < n_items)
    {
      bobgui_list_item_query_tracked_range (self, n_items, position, &query_n_items, &tracked);
      if (!tracked)
        {
          position += query_n_items;
          continue;
        }

      tile = bobgui_list_item_manager_get_nth (self, position, &offset);
      if (offset > 0)
        tile = bobgui_list_item_manager_ensure_split (self, tile, offset);

      if (has_sections)
        {
          header = bobgui_list_tile_get_header (self, tile);
          if (header->type == BOBGUI_LIST_TILE_UNMATCHED_HEADER)
            {
              guint start, end;
              gpointer item;

              bobgui_section_model_get_section (BOBGUI_SECTION_MODEL (self->model), position, &start, &end);
              header = bobgui_list_item_manager_insert_section (self,
                                                             start,
                                                             BOBGUI_LIST_TILE_UNMATCHED_FOOTER,
                                                             BOBGUI_LIST_TILE_HEADER);
              g_assert (header != NULL && header->widget == NULL);
              header->widget = BOBGUI_WIDGET (bobgui_list_item_change_get_header (change));
              if (header->widget == NULL)
                header->widget = BOBGUI_WIDGET (self->create_header_widget (self->widget));
              item = g_list_model_get_item (G_LIST_MODEL (self->model), start);
              bobgui_list_header_base_update (BOBGUI_LIST_HEADER_BASE (header->widget),
                                           item,
                                           start, end);
              g_object_unref (item);
              bobgui_widget_insert_after (header->widget,
                                       self->widget,
                                       bobgui_list_tile_find_widget_before (header));

              bobgui_list_item_manager_insert_section (self,
                                                    end,
                                                    BOBGUI_LIST_TILE_FOOTER,
                                                    BOBGUI_LIST_TILE_UNMATCHED_HEADER);
            }
          else if (bobgui_list_header_base_get_end (BOBGUI_LIST_HEADER_BASE (header->widget)) > update_start)
            {
              BobguiListHeaderBase *base = BOBGUI_LIST_HEADER_BASE (header->widget);
              guint start = bobgui_list_header_base_get_start (base);
              bobgui_list_header_base_update (base,
                                           bobgui_list_header_base_get_item (base),
                                           start > update_start ? start + update_diff : start,
                                           bobgui_list_header_base_get_end (base) + update_diff);
            }
        }

      insert_after = bobgui_list_tile_find_widget_before (tile);

      for (i = 0; i < query_n_items;)
        {
          g_assert (tile != NULL);

          switch (tile->type)
          {
            case BOBGUI_LIST_TILE_ITEM:
              if (tile->n_items > 1)
                bobgui_list_item_manager_ensure_split (self, tile, 1);

              if (tile->widget == NULL)
                {
                  gpointer item = g_list_model_get_item (G_LIST_MODEL (self->model), position + i);
                  tile->widget = BOBGUI_WIDGET (bobgui_list_item_change_get (change, item));
                  if (tile->widget == NULL)
                    tile->widget = BOBGUI_WIDGET (self->create_widget (self->widget));
                  bobgui_list_item_base_update (BOBGUI_LIST_ITEM_BASE (tile->widget),
                                             position + i,
                                             item,
                                             bobgui_selection_model_is_selected (self->model, position + i));
                  bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (tile->widget),
                                                  BOBGUI_ACCESSIBLE_RELATION_POS_IN_SET, position + i + 1,
                                                  BOBGUI_ACCESSIBLE_RELATION_SET_SIZE, g_list_model_get_n_items (G_LIST_MODEL (self->model)),
                                                  -1);

                  g_object_unref (item);
                  bobgui_widget_insert_after (tile->widget, self->widget, insert_after);
                }
              else
                {
                  if (update_start <= position + i)
                    {
                      bobgui_list_item_base_update (BOBGUI_LIST_ITEM_BASE (tile->widget),
                                                 position + i,
                                                 bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (tile->widget)),
                                                 bobgui_selection_model_is_selected (self->model, position + i));
                    }
                }
              insert_after = tile->widget;
              i++;
              break;

            case BOBGUI_LIST_TILE_UNMATCHED_HEADER:
              if (has_sections)
                {
                  guint start, end;
                  gpointer item;

                  bobgui_section_model_get_section (BOBGUI_SECTION_MODEL (self->model), position + i, &start, &end);

                  bobgui_list_tile_set_type (tile, BOBGUI_LIST_TILE_HEADER);
                  g_assert (tile->widget == NULL);
                  tile->widget = BOBGUI_WIDGET (bobgui_list_item_change_get_header (change));
                  if (tile->widget == NULL)
                    tile->widget = BOBGUI_WIDGET (self->create_header_widget (self->widget));
                  item = g_list_model_get_item (G_LIST_MODEL (self->model), start);
                  bobgui_list_header_base_update (BOBGUI_LIST_HEADER_BASE (tile->widget),
                                               item,
                                               start, end);
                  g_object_unref (item);
                  bobgui_widget_insert_after (tile->widget, self->widget, insert_after);
                  insert_after = tile->widget;

                  bobgui_list_item_manager_insert_section (self,
                                                        end,
                                                        BOBGUI_LIST_TILE_FOOTER,
                                                        BOBGUI_LIST_TILE_UNMATCHED_HEADER);
                }
              break;

            case BOBGUI_LIST_TILE_HEADER:
            case BOBGUI_LIST_TILE_FOOTER:
              break;

            case BOBGUI_LIST_TILE_UNMATCHED_FOOTER:
            case BOBGUI_LIST_TILE_REMOVED:
            default:
              g_assert_not_reached ();
              break;
            }
          tile = bobgui_list_tile_get_next_skip (tile);
        }

      position += query_n_items;
    }
}

static void
bobgui_list_item_manager_model_items_changed_cb (GListModel         *model,
                                              guint               position,
                                              guint               removed,
                                              guint               added,
                                              BobguiListItemManager *self)
{
  BobguiListItemChange change;
  GSList *l;
  guint n_items;

  bobgui_list_item_change_init (&change);
  n_items = g_list_model_get_n_items (G_LIST_MODEL (self->model));

  bobgui_list_item_manager_remove_items (self, &change, position, removed);
  bobgui_list_item_manager_add_items (self, &change, position, added);

  /* Check if any tracked item was removed */
  for (l = self->trackers; l; l = l->next)
    {
      BobguiListItemTracker *tracker = l->data;

      if (tracker->widget == NULL)
        continue;

      if (tracker->position >= position && tracker->position < position + removed)
        break;
    }

  /* At least one tracked item was removed, do a more expensive rebuild
   * trying to find where it moved */
  if (l)
    {
      BobguiListTile *tile, *new_tile;
      BobguiWidget *insert_after;
      guint i, offset;

      tile = bobgui_list_item_manager_get_nth (self, position, &offset);
      for (new_tile = tile ? bobgui_rb_tree_node_get_previous (tile) : bobgui_rb_tree_get_last (self->items);
           new_tile && new_tile->widget == NULL;
           new_tile = bobgui_rb_tree_node_get_previous (new_tile))
        { }
      if (new_tile)
        insert_after = new_tile->widget;
      else
        insert_after = NULL; /* we're at the start */

      for (i = 0; i < added; i++)
        {
          BobguiListItemBase *widget;
          gpointer item;

          /* XXX: can we avoid temporarily allocating items on failure? */
          item = g_list_model_get_item (G_LIST_MODEL (self->model), position + i);
          widget = bobgui_list_item_change_find (&change, item);
          g_object_unref (item);

          if (widget == NULL)
            {
              offset++;
              continue;
            }

          while (offset >= tile->n_items)
            {
              offset -= tile->n_items;
              tile = bobgui_rb_tree_node_get_next (tile);
            }
          if (offset > 0)
            {
              tile = bobgui_list_item_manager_ensure_split (self, tile, offset);
              offset = 0;
            }

          new_tile = tile;
          if (tile->n_items == 1)
            tile = bobgui_rb_tree_node_get_next (tile);
          else
            tile = bobgui_list_item_manager_ensure_split (self, tile, 1);

          new_tile->widget = BOBGUI_WIDGET (widget);
          bobgui_list_item_base_update (widget,
                                     position + i,
                                     item,
                                     bobgui_selection_model_is_selected (self->model, position + i));

          bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (widget),
                                          BOBGUI_ACCESSIBLE_RELATION_POS_IN_SET, position + i + 1,
                                          BOBGUI_ACCESSIBLE_RELATION_SET_SIZE, g_list_model_get_n_items (G_LIST_MODEL (self->model)),
                                          -1);

          bobgui_widget_insert_after (new_tile->widget, self->widget, insert_after);
          insert_after = new_tile->widget;
        }
    }

  /* Update tracker positions if necessary, they need to have correct
   * positions for bobgui_list_item_manager_ensure_items().
   * We don't update the items, they will be updated by ensure_items()
   * and then we can update them. */
  for (l = self->trackers; l; l = l->next)
    {
      BobguiListItemTracker *tracker = l->data;

      if (tracker->position == BOBGUI_INVALID_LIST_POSITION)
        {
          /* if the list is no longer empty, set the tracker to a valid position. */
          if (n_items > 0 && n_items == added && removed == 0)
            tracker->position = 0;
        }
      else if (tracker->position >= position + removed)
        {
          tracker->position += added - removed;
        }
      else if (tracker->position >= position)
        {
          BobguiListItemBase *widget = bobgui_list_item_change_find (&change, bobgui_list_item_base_get_item (tracker->widget));
          if (widget)
            {
              /* The item is still in the recycling pool, which means it got deleted.
               * Put the widget back and then guess a good new position */
              bobgui_list_item_change_release (&change, widget);

              tracker->position = position + (tracker->position - position) * added / removed;
              if (tracker->position >= n_items)
                {
                  if (n_items == 0)
                    tracker->position = BOBGUI_INVALID_LIST_POSITION;
                  else
                    tracker->position--;
                }
              tracker->widget = NULL;
            }
          else
            {
              /* item was put in its right place in the expensive loop above,
               * and we updated its position while at it. So grab it from there.
               */
              tracker->position = bobgui_list_item_base_get_position (tracker->widget);
            }
        }
      else
        {
          /* nothing changed for items before position */
        }
    }

  bobgui_list_item_manager_ensure_items (self, &change, position + added, added - removed);

  /* final loop through the trackers: Grab the missing widgets.
   * For items that had been removed and a new position was set, grab
   * their item now that we ensured it exists.
   */
  for (l = self->trackers; l; l = l->next)
    {
      BobguiListItemTracker *tracker = l->data;
      BobguiListTile *tile;

      if (tracker->widget != NULL ||
          tracker->position == BOBGUI_INVALID_LIST_POSITION)
        continue;

      tile = bobgui_list_item_manager_get_nth (self, tracker->position, NULL);
      g_assert (tile != NULL);
      g_assert (tile->widget);
      tracker->widget = BOBGUI_LIST_ITEM_BASE (tile->widget);
    }

  bobgui_list_item_change_finish (&change);

  bobgui_widget_queue_resize (self->widget);
}

static void
bobgui_list_item_manager_model_n_items_changed_cb (GListModel         *model,
                                                GParamSpec         *pspec,
                                                BobguiListItemManager *self)
{
  guint n_items = g_list_model_get_n_items (model);
  BobguiListTile *tile;

  for (tile = bobgui_list_item_manager_get_first (self);
       tile != NULL;
       tile = bobgui_rb_tree_node_get_next (tile))
    {
      if (tile->widget && tile->type == BOBGUI_LIST_TILE_ITEM)
        bobgui_accessible_update_relation (BOBGUI_ACCESSIBLE (tile->widget),
                                        BOBGUI_ACCESSIBLE_RELATION_SET_SIZE, n_items,
                                       -1);
    }
}

static void
bobgui_list_item_manager_model_sections_changed_cb (GListModel         *model,
                                                 guint               position,
                                                 guint               n_items,
                                                 BobguiListItemManager *self)
{
  BobguiListItemChange change;
  BobguiListTile *tile, *header;
  guint offset;

  if (!bobgui_list_item_manager_has_sections (self))
    return;

  bobgui_list_item_change_init (&change);

  tile = bobgui_list_item_manager_get_nth (self, position, &offset);
  header = bobgui_list_tile_get_header (self, tile);
  bobgui_list_item_change_clear_header (&change, &header->widget);
  bobgui_list_tile_set_type (header, BOBGUI_LIST_TILE_UNMATCHED_HEADER);

  n_items += offset;
  while (n_items > 0)
    {
      switch (tile->type)
        {
        case BOBGUI_LIST_TILE_HEADER:
        case BOBGUI_LIST_TILE_UNMATCHED_HEADER:
          bobgui_list_item_change_clear_header (&change, &tile->widget);
          bobgui_list_tile_set_type (tile, BOBGUI_LIST_TILE_REMOVED);
          break;

        case BOBGUI_LIST_TILE_FOOTER:
        case BOBGUI_LIST_TILE_UNMATCHED_FOOTER:
          bobgui_list_tile_set_type (tile, BOBGUI_LIST_TILE_REMOVED);
          break;

        case BOBGUI_LIST_TILE_ITEM:
          n_items -= MIN (n_items, tile->n_items);
          break;

        case BOBGUI_LIST_TILE_REMOVED:
        default:
          g_assert_not_reached ();
          break;
        }

      tile = bobgui_list_tile_get_next_skip (tile);
    }

  if (!bobgui_list_tile_is_footer (tile))
    tile = bobgui_list_tile_get_footer (self, tile);

  bobgui_list_tile_set_type (tile, BOBGUI_LIST_TILE_UNMATCHED_FOOTER);

  bobgui_list_item_manager_ensure_items (self, &change, G_MAXUINT, 0);

  bobgui_list_item_change_finish (&change);

  bobgui_widget_queue_resize (BOBGUI_WIDGET (self->widget));
}

static void
bobgui_list_item_manager_model_selection_changed_cb (GListModel         *model,
                                                  guint               position,
                                                  guint               n_items,
                                                  BobguiListItemManager *self)
{
  BobguiListTile *tile;
  guint offset;

  tile = bobgui_list_item_manager_get_nth (self, position, &offset);

  if (offset)
    {
      position += tile->n_items - offset;
      if (tile->n_items - offset > n_items)
        n_items = 0;
      else
        n_items -= tile->n_items - offset;
      tile = bobgui_rb_tree_node_get_next (tile);
    }

  while (n_items > 0)
    {
      if (tile->widget && tile->type == BOBGUI_LIST_TILE_ITEM)
        {
          bobgui_list_item_base_update (BOBGUI_LIST_ITEM_BASE (tile->widget),
                                     position,
                                     bobgui_list_item_base_get_item (BOBGUI_LIST_ITEM_BASE (tile->widget)),
                                     bobgui_selection_model_is_selected (self->model, position));
        }
      position += tile->n_items;
      n_items -= MIN (n_items, tile->n_items);
      tile = bobgui_list_tile_get_next_skip (tile);
    }
}

static void
bobgui_list_item_manager_clear_model (BobguiListItemManager *self)
{
  BobguiListItemChange change;
  GSList *l;

  if (self->model == NULL)
    return;

  bobgui_list_item_change_init (&change);
  bobgui_list_item_manager_remove_items (self, &change, 0, g_list_model_get_n_items (G_LIST_MODEL (self->model)));
  bobgui_list_item_change_finish (&change);
  for (l = self->trackers; l; l = l->next)
    {
      bobgui_list_item_tracker_unset_position (self, l->data);
    }

  g_signal_handlers_disconnect_by_func (self->model,
                                        bobgui_list_item_manager_model_selection_changed_cb,
                                        self);
  g_signal_handlers_disconnect_by_func (self->model,
                                        bobgui_list_item_manager_model_items_changed_cb,
                                        self);
  g_signal_handlers_disconnect_by_func (self->model,
                                        bobgui_list_item_manager_model_n_items_changed_cb,
                                        self);
  g_signal_handlers_disconnect_by_func (self->model,
                                        bobgui_list_item_manager_model_sections_changed_cb,
                                        self);
  g_clear_object (&self->model);

  bobgui_list_item_manager_gc_tiles (self);

  g_assert (bobgui_rb_tree_get_root (self->items) == NULL);
}

static void
bobgui_list_item_manager_dispose (GObject *object)
{
  BobguiListItemManager *self = BOBGUI_LIST_ITEM_MANAGER (object);

  bobgui_list_item_manager_clear_model (self);

  g_clear_pointer (&self->items, bobgui_rb_tree_unref);

  G_OBJECT_CLASS (bobgui_list_item_manager_parent_class)->dispose (object);
}

static void
bobgui_list_item_manager_class_init (BobguiListItemManagerClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = bobgui_list_item_manager_dispose;
}

static void
bobgui_list_item_manager_init (BobguiListItemManager *self)
{
}

void
bobgui_list_item_manager_set_model (BobguiListItemManager *self,
                                 BobguiSelectionModel  *model)
{
  g_return_if_fail (BOBGUI_IS_LIST_ITEM_MANAGER (self));
  g_return_if_fail (model == NULL || BOBGUI_IS_SELECTION_MODEL (model));

  if (self->model == model)
    return;

  bobgui_list_item_manager_clear_model (self);

  if (model)
    {
      BobguiListItemChange change;

      self->model = g_object_ref (model);

      g_signal_connect (model,
                        "items-changed",
                        G_CALLBACK (bobgui_list_item_manager_model_items_changed_cb),
                        self);
      g_signal_connect (model,
                        "notify::n-items",
                        G_CALLBACK (bobgui_list_item_manager_model_n_items_changed_cb),
                        self);
      g_signal_connect (model,
                        "selection-changed",
                        G_CALLBACK (bobgui_list_item_manager_model_selection_changed_cb),
                        self);
      if (BOBGUI_IS_SECTION_MODEL (model))
        g_signal_connect (model,
                          "sections-changed",
                          G_CALLBACK (bobgui_list_item_manager_model_sections_changed_cb),
                          self);

      bobgui_list_item_change_init (&change);
      bobgui_list_item_manager_add_items (self, &change, 0, g_list_model_get_n_items (G_LIST_MODEL (model)));
      bobgui_list_item_manager_ensure_items (self, &change, G_MAXUINT, 0);
      bobgui_list_item_change_finish (&change);
    }
}

BobguiSelectionModel *
bobgui_list_item_manager_get_model (BobguiListItemManager *self)
{
  g_return_val_if_fail (BOBGUI_IS_LIST_ITEM_MANAGER (self), NULL);

  return self->model;
}

void
bobgui_list_item_manager_set_has_sections (BobguiListItemManager *self,
                                        gboolean            has_sections)
{
  BobguiListItemChange change;
  BobguiListTile *tile;
  gboolean had_sections;

  if (self->has_sections == has_sections)
    return;

  had_sections = bobgui_list_item_manager_has_sections (self);

  self->has_sections = has_sections;

  bobgui_list_item_change_init (&change);

  if (had_sections && !bobgui_list_item_manager_has_sections (self))
    {
      BobguiListTile *header = NULL, *footer = NULL;

      for (tile = bobgui_rb_tree_get_first (self->items);
           tile;
           tile = bobgui_list_tile_get_next_skip (tile))
        {
          switch (tile->type)
            {
            case BOBGUI_LIST_TILE_HEADER:
            case BOBGUI_LIST_TILE_UNMATCHED_HEADER:
              bobgui_list_item_change_clear_header (&change, &tile->widget);
              if (!header)
                header = tile;
              else
                bobgui_list_tile_set_type (tile, BOBGUI_LIST_TILE_REMOVED);
              break;
            case BOBGUI_LIST_TILE_FOOTER:
            case BOBGUI_LIST_TILE_UNMATCHED_FOOTER:
              if (footer)
                bobgui_list_tile_set_type (footer, BOBGUI_LIST_TILE_REMOVED);
              footer = tile;
              break;
            case BOBGUI_LIST_TILE_ITEM:
            case BOBGUI_LIST_TILE_REMOVED:
              break;
            default:
              g_assert_not_reached ();
              break;
            }
        }
      if (header)
        {
          bobgui_list_tile_set_type (header, BOBGUI_LIST_TILE_UNMATCHED_HEADER);
          bobgui_list_tile_set_type (footer, BOBGUI_LIST_TILE_UNMATCHED_FOOTER);
        }
    }

  bobgui_list_item_manager_ensure_items (self, &change, G_MAXUINT, 0);
  bobgui_list_item_change_finish (&change);

  bobgui_widget_queue_resize (self->widget);
}

gboolean
bobgui_list_item_manager_get_has_sections (BobguiListItemManager *self)
{
  return self->has_sections;
}

BobguiListItemTracker *
bobgui_list_item_tracker_new (BobguiListItemManager *self)
{
  BobguiListItemTracker *tracker;

  g_return_val_if_fail (BOBGUI_IS_LIST_ITEM_MANAGER (self), NULL);

  tracker = g_new0 (BobguiListItemTracker, 1);

  tracker->position = BOBGUI_INVALID_LIST_POSITION;

  self->trackers = g_slist_prepend (self->trackers, tracker);

  return tracker;
}

void
bobgui_list_item_tracker_free (BobguiListItemManager *self,
                            BobguiListItemTracker *tracker)
{
  BobguiListItemChange change;

  bobgui_list_item_tracker_unset_position (self, tracker);

  self->trackers = g_slist_remove (self->trackers, tracker);

  g_free (tracker);

  bobgui_list_item_change_init (&change);
  bobgui_list_item_manager_ensure_items (self, &change, G_MAXUINT, 0);
  bobgui_list_item_change_finish (&change);

  bobgui_widget_queue_resize (self->widget);
}

void
bobgui_list_item_tracker_set_position (BobguiListItemManager *self,
                                    BobguiListItemTracker *tracker,
                                    guint               position,
                                    guint               n_before,
                                    guint               n_after)
{
  BobguiListItemChange change;
  BobguiListTile *tile;
  guint n_items;

  bobgui_list_item_tracker_unset_position (self, tracker);

  if (self->model == NULL)
    return;

  n_items = g_list_model_get_n_items (G_LIST_MODEL (self->model));
  if (position >= n_items)
    position = n_items - 1; /* for n_items == 0 this underflows to BOBGUI_INVALID_LIST_POSITION */

  tracker->position = position;
  tracker->n_before = n_before;
  tracker->n_after = n_after;

  bobgui_list_item_change_init (&change);
  bobgui_list_item_manager_ensure_items (self, &change, G_MAXUINT, 0);
  bobgui_list_item_change_finish (&change);

  tile = bobgui_list_item_manager_get_nth (self, position, NULL);
  if (tile)
    tracker->widget = BOBGUI_LIST_ITEM_BASE (tile->widget);

  bobgui_widget_queue_resize (self->widget);
}

guint
bobgui_list_item_tracker_get_position (BobguiListItemManager *self,
                                    BobguiListItemTracker *tracker)
{
  return tracker->position;
}
