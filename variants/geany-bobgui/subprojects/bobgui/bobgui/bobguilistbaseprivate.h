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

#pragma once

#include "bobguilistbase.h"

#include "bobguilistitemmanagerprivate.h"
#include "bobguiprivate.h"

struct _BobguiListBase
{
  BobguiWidget parent_instance;
};

struct _BobguiListBaseClass
{
  BobguiWidgetClass parent_class;

  BobguiListTile *        (* split)                                (BobguiListBase            *self,
                                                                 BobguiListTile            *tile,
                                                                 guint                   n_items);
  BobguiListItemBase *    (* create_list_widget)                   (BobguiListBase            *self);
  void                 (* prepare_section)                      (BobguiListBase            *self,
                                                                 BobguiListTile            *tile,
                                                                 guint                   position);
  BobguiListHeaderBase *  (* create_header_widget)                 (BobguiListBase            *self);

  gboolean             (* get_allocation)                       (BobguiListBase            *self,
                                                                 guint                   pos,
                                                                 GdkRectangle           *area);
  gboolean             (* get_position_from_allocation)         (BobguiListBase            *self,
                                                                 int                     across,
                                                                 int                     along,
                                                                 guint                  *pos,
                                                                 cairo_rectangle_int_t  *area);
  BobguiBitset *          (* get_items_in_rect)                    (BobguiListBase            *self,
                                                                 const cairo_rectangle_int_t *rect);
  guint                (* move_focus_along)                     (BobguiListBase            *self,
                                                                 guint                   pos,
                                                                 int                     steps);
  guint                (* move_focus_across)                    (BobguiListBase            *self,
                                                                 guint                   pos,
                                                                 int                     steps);
};

BobguiOrientation         bobgui_list_base_get_orientation            (BobguiListBase            *self);
#define bobgui_list_base_get_opposite_orientation(self) OPPOSITE_ORIENTATION(bobgui_list_base_get_orientation(self))
void                   bobgui_list_base_get_border_spacing         (BobguiListBase            *self,
                                                                 int                    *xspacing,
                                                                 int                    *yspacing);
BobguiListItemManager *   bobgui_list_base_get_manager                (BobguiListBase            *self);
BobguiScrollablePolicy    bobgui_list_base_get_scroll_policy          (BobguiListBase            *self,
                                                                 BobguiOrientation          orientation);
guint                  bobgui_list_base_get_n_items                (BobguiListBase            *self);
BobguiSelectionModel *    bobgui_list_base_get_model                  (BobguiListBase            *self);
gboolean               bobgui_list_base_set_model                  (BobguiListBase            *self,
                                                                 BobguiSelectionModel      *model);

guint                  bobgui_list_base_get_anchor                 (BobguiListBase            *self);
void                   bobgui_list_base_set_anchor                 (BobguiListBase            *self,
                                                                 guint                   anchor_pos,
                                                                 double                  anchor_align_across,
                                                                 BobguiPackType             anchor_side_across,
                                                                 double                  anchor_align_along,
                                                                 BobguiPackType             anchor_side_along);
void                   bobgui_list_base_set_anchor_max_widgets     (BobguiListBase            *self,
                                                                 guint                   n_center,
                                                                 guint                   n_above_below);

void                   bobgui_list_base_set_enable_rubberband      (BobguiListBase            *self,
                                                                 gboolean                enable);
gboolean               bobgui_list_base_get_enable_rubberband      (BobguiListBase            *self);
void                   bobgui_list_base_set_tab_behavior           (BobguiListBase            *self,
                                                                 BobguiListTabBehavior      behavior);
BobguiListTabBehavior     bobgui_list_base_get_tab_behavior           (BobguiListBase            *self);


void                   bobgui_list_base_allocate                   (BobguiListBase            *self);

void                   bobgui_list_base_scroll_to                  (BobguiListBase            *self,
                                                                 guint                   pos,
                                                                 BobguiListScrollFlags      flags,
                                                                 BobguiScrollInfo          *scroll);
