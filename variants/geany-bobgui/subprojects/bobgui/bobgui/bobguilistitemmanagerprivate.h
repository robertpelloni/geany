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


#pragma once

#include "bobgui/bobguitypes.h"
#include "bobgui/bobguienums.h"

#include "bobgui/bobguilistitembaseprivate.h"
#include "bobgui/bobguilistheaderbaseprivate.h"
#include "bobgui/bobguilistitemfactory.h"
#include "bobgui/bobguirbtreeprivate.h"
#include "bobgui/bobguiselectionmodel.h"

G_BEGIN_DECLS

#define BOBGUI_TYPE_LIST_ITEM_MANAGER         (bobgui_list_item_manager_get_type ())
#define BOBGUI_LIST_ITEM_MANAGER(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), BOBGUI_TYPE_LIST_ITEM_MANAGER, BobguiListItemManager))
#define BOBGUI_LIST_ITEM_MANAGER_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST ((k), BOBGUI_TYPE_LIST_ITEM_MANAGER, BobguiListItemManagerClass))
#define BOBGUI_IS_LIST_ITEM_MANAGER(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), BOBGUI_TYPE_LIST_ITEM_MANAGER))
#define BOBGUI_IS_LIST_ITEM_MANAGER_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), BOBGUI_TYPE_LIST_ITEM_MANAGER))
#define BOBGUI_LIST_ITEM_MANAGER_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), BOBGUI_TYPE_LIST_ITEM_MANAGER, BobguiListItemManagerClass))

typedef struct _BobguiListItemManager BobguiListItemManager;
typedef struct _BobguiListItemManagerClass BobguiListItemManagerClass;
typedef struct _BobguiListTile BobguiListTile;
typedef struct _BobguiListTileAugment BobguiListTileAugment;
typedef struct _BobguiListItemTracker BobguiListItemTracker;

typedef enum
{
  BOBGUI_LIST_TILE_ITEM,
  BOBGUI_LIST_TILE_HEADER,
  BOBGUI_LIST_TILE_FOOTER,
  BOBGUI_LIST_TILE_UNMATCHED_HEADER,
  BOBGUI_LIST_TILE_UNMATCHED_FOOTER,
  BOBGUI_LIST_TILE_REMOVED,
} BobguiListTileType;

struct _BobguiListTile
{
  BobguiListTileType type;
  BobguiWidget *widget;
  guint n_items;
  /* area occupied by tile. May be empty if tile has no allocation */
  cairo_rectangle_int_t area;
};

struct _BobguiListTileAugment
{
  guint n_items;

  guint has_header :1;
  guint has_footer :1;

  /* union of all areas of tile and children */
  cairo_rectangle_int_t area;
};


GType                   bobgui_list_item_manager_get_type          (void) G_GNUC_CONST;

BobguiListItemManager *    bobgui_list_item_manager_new               (BobguiWidget              *widget,
                                                                 BobguiListTile *           (* split_func) (BobguiWidget *, BobguiListTile *, guint),
                                                                 BobguiListItemBase *       (* create_widget) (BobguiWidget *),
                                                                 void                    (* prepare_section) (BobguiWidget *, BobguiListTile *, guint),
                                                                 BobguiListHeaderBase *     (* create_header_widget) (BobguiWidget *));

void                    bobgui_list_item_manager_get_tile_bounds   (BobguiListItemManager     *self,
                                                                 GdkRectangle           *out_bounds);
gpointer                bobgui_list_item_manager_get_root          (BobguiListItemManager     *self);
gpointer                bobgui_list_item_manager_get_first         (BobguiListItemManager     *self);
gpointer                bobgui_list_item_manager_get_last          (BobguiListItemManager     *self);
gpointer                bobgui_list_item_manager_get_nth           (BobguiListItemManager     *self,
                                                                 guint                   position,
                                                                 guint                  *offset);
BobguiListTile *           bobgui_list_item_manager_get_nearest_tile  (BobguiListItemManager     *self,
                                                                 int                     x,
                                                                 int                     y);
void                    bobgui_list_item_manager_gc_tiles          (BobguiListItemManager     *self);

static inline gboolean
bobgui_list_tile_is_header (BobguiListTile *tile)
{
  return tile->type == BOBGUI_LIST_TILE_HEADER || tile->type == BOBGUI_LIST_TILE_UNMATCHED_HEADER;
}

static inline gboolean
bobgui_list_tile_is_footer (BobguiListTile *tile)
{
  return tile->type == BOBGUI_LIST_TILE_FOOTER || tile->type == BOBGUI_LIST_TILE_UNMATCHED_FOOTER;
}

guint                   bobgui_list_tile_get_position              (BobguiListItemManager     *self,
                                                                 BobguiListTile            *tile);
gpointer                bobgui_list_tile_get_augment               (BobguiListItemManager     *self,
                                                                 BobguiListTile            *tile);
void                    bobgui_list_tile_set_area                  (BobguiListItemManager     *self,
                                                                 BobguiListTile            *tile,
                                                                 const cairo_rectangle_int_t *area);
void                    bobgui_list_tile_set_area_position         (BobguiListItemManager     *self,
                                                                 BobguiListTile            *tile,
                                                                 int                     x,
                                                                 int                     y);
void                    bobgui_list_tile_set_area_size             (BobguiListItemManager     *self,
                                                                 BobguiListTile            *tile,
                                                                 int                     width,
                                                                 int                     height);

BobguiListTile *           bobgui_list_tile_split                     (BobguiListItemManager     *self,
                                                                 BobguiListTile            *tile,
                                                                 guint                   n_items);

void                    bobgui_list_item_manager_set_model         (BobguiListItemManager     *self,
                                                                 BobguiSelectionModel      *model);
BobguiSelectionModel *     bobgui_list_item_manager_get_model         (BobguiListItemManager     *self);
void                    bobgui_list_item_manager_set_has_sections  (BobguiListItemManager     *self,
                                                                 gboolean                has_sections);
gboolean                bobgui_list_item_manager_get_has_sections  (BobguiListItemManager     *self);

BobguiListItemTracker *    bobgui_list_item_tracker_new               (BobguiListItemManager     *self);
void                    bobgui_list_item_tracker_free              (BobguiListItemManager     *self,
                                                                 BobguiListItemTracker     *tracker);
void                    bobgui_list_item_tracker_set_position      (BobguiListItemManager     *self,
                                                                 BobguiListItemTracker     *tracker,
                                                                 guint                   position,
                                                                 guint                   n_before,
                                                                 guint                   n_after);
guint                   bobgui_list_item_tracker_get_position      (BobguiListItemManager     *self,
                                                                 BobguiListItemTracker     *tracker);


G_END_DECLS

