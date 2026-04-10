/* bobguiiconview.h
 * Copyright (C) 2002, 2004  Anders Carlsson <andersca@gnome.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include "bobgui/deprecated/bobguiiconview.h"
#include "bobgui/bobguicssnodeprivate.h"
#include "bobgui/bobguieventcontrollermotion.h"
#include "bobgui/bobguidragsource.h"
#include "bobgui/bobguidroptargetasync.h"
#include "bobgui/bobguigestureclick.h"

#pragma once

typedef struct _BobguiIconViewItem BobguiIconViewItem;
struct _BobguiIconViewItem
{
  GdkRectangle cell_area;

  int index;

  int row, col;

  guint selected : 1;
  guint selected_before_rubberbanding : 1;

};

typedef struct _BobguiIconViewClass      BobguiIconViewClass;
typedef struct _BobguiIconViewPrivate    BobguiIconViewPrivate;

struct _BobguiIconView
{
  BobguiWidget parent;

  BobguiIconViewPrivate *priv;
};

struct _BobguiIconViewClass
{
  BobguiWidgetClass parent_class;

  void    (* item_activated)         (BobguiIconView      *icon_view,
                                      BobguiTreePath      *path);
  void    (* selection_changed)      (BobguiIconView      *icon_view);

  void    (* select_all)             (BobguiIconView      *icon_view);
  void    (* unselect_all)           (BobguiIconView      *icon_view);
  void    (* select_cursor_item)     (BobguiIconView      *icon_view);
  void    (* toggle_cursor_item)     (BobguiIconView      *icon_view);
  gboolean (* move_cursor)           (BobguiIconView      *icon_view,
                                      BobguiMovementStep   step,
                                      int               count,
                                      gboolean          extend,
                                      gboolean          modify);
  gboolean (* activate_cursor_item)  (BobguiIconView      *icon_view);
};

struct _BobguiIconViewPrivate
{
  BobguiCellArea        *cell_area;
  BobguiCellAreaContext *cell_area_context;

  gulong              add_editable_id;
  gulong              remove_editable_id;
  gulong              context_changed_id;

  GPtrArray          *row_contexts;

  int width, height;
  double mouse_x;
  double mouse_y;

  BobguiSelectionMode selection_mode;

  GList *children;

  BobguiTreeModel *model;

  GList *items;

  BobguiEventController *key_controller;

  BobguiAdjustment *hadjustment;
  BobguiAdjustment *vadjustment;

  int rubberband_x1, rubberband_y1;
  int rubberband_x2, rubberband_y2;
  GdkDevice *rubberband_device;
  BobguiCssNode *rubberband_node;

  guint scroll_timeout_id;
  int scroll_value_diff;
  int event_last_x, event_last_y;

  BobguiIconViewItem *anchor_item;
  BobguiIconViewItem *cursor_item;

  BobguiIconViewItem *last_single_clicked;
  BobguiIconViewItem *last_prelight;

  BobguiOrientation item_orientation;

  int columns;
  int item_width;
  int spacing;
  int row_spacing;
  int column_spacing;
  int margin;
  int item_padding;

  int text_column;
  int markup_column;
  int pixbuf_column;
  int tooltip_column;

  BobguiCellRenderer *pixbuf_cell;
  BobguiCellRenderer *text_cell;

  /* Drag-and-drop. */
  GdkModifierType start_button_mask;
  int pressed_button;
  double press_start_x;
  double press_start_y;

  GdkContentFormats *source_formats;
  BobguiDropTargetAsync *dest;
  BobguiCssNode *dndnode;

  GdkDrag *drag;

  GdkDragAction source_actions;
  GdkDragAction dest_actions;

  BobguiTreeRowReference *source_item;
  BobguiTreeRowReference *dest_item;
  BobguiIconViewDropPosition dest_pos;

  /* scroll to */
  BobguiTreeRowReference *scroll_to_path;
  float scroll_to_row_align;
  float scroll_to_col_align;
  guint scroll_to_use_align : 1;

  guint source_set : 1;
  guint dest_set : 1;
  guint reorderable : 1;
  guint empty_view_drop :1;
  guint activate_on_single_click : 1;

  guint modify_selection_pressed : 1;
  guint extend_selection_pressed : 1;

  guint draw_focus : 1;

  /* BobguiScrollablePolicy needs to be checked when
   * driving the scrollable adjustment values */
  guint hscroll_policy : 1;
  guint vscroll_policy : 1;

  guint doing_rubberband : 1;

};

void                 _bobgui_icon_view_set_cell_data                  (BobguiIconView            *icon_view,
                                                                    BobguiIconViewItem        *item);
void                 _bobgui_icon_view_set_cursor_item                (BobguiIconView            *icon_view,
                                                                    BobguiIconViewItem        *item,
                                                                    BobguiCellRenderer        *cursor_cell);
BobguiIconViewItem *    _bobgui_icon_view_get_item_at_coords             (BobguiIconView            *icon_view,
                                                                    int                     x,
                                                                    int                     y,
                                                                    gboolean                only_in_cell,
                                                                    BobguiCellRenderer       **cell_at_pos);
void                 _bobgui_icon_view_select_item                    (BobguiIconView            *icon_view,
                                                                    BobguiIconViewItem        *item);
void                 _bobgui_icon_view_unselect_item                  (BobguiIconView            *icon_view,
                                                                    BobguiIconViewItem        *item);

G_END_DECLS

