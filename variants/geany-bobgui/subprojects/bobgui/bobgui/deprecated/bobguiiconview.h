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

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>
#include <bobgui/bobguitooltip.h>
#include <bobgui/deprecated/bobguitreemodel.h>
#include <bobgui/deprecated/bobguicellrenderer.h>
#include <bobgui/deprecated/bobguicellarea.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_ICON_VIEW            (bobgui_icon_view_get_type ())
#define BOBGUI_ICON_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_ICON_VIEW, BobguiIconView))
#define BOBGUI_IS_ICON_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_ICON_VIEW))

typedef struct _BobguiIconView           BobguiIconView;

/**
 * BobguiIconViewForeachFunc:
 * @icon_view: a `BobguiIconView`
 * @path: The `BobguiTreePath` of a selected row
 * @data: (closure): user data
 *
 * A function used by bobgui_icon_view_selected_foreach() to map all
 * selected rows.
 *
 * It will be called on every selected row in the view.
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef void (* BobguiIconViewForeachFunc)     (BobguiIconView      *icon_view,
                                             BobguiTreePath      *path,
                                             gpointer          data);

/**
 * BobguiIconViewDropPosition:
 * @BOBGUI_ICON_VIEW_NO_DROP: no drop possible
 * @BOBGUI_ICON_VIEW_DROP_INTO: dropped item replaces the item
 * @BOBGUI_ICON_VIEW_DROP_LEFT: dropped item is inserted to the left
 * @BOBGUI_ICON_VIEW_DROP_RIGHT: dropped item is inserted to the right
 * @BOBGUI_ICON_VIEW_DROP_ABOVE: dropped item is inserted above
 * @BOBGUI_ICON_VIEW_DROP_BELOW: dropped item is inserted below
 *
 * An enum for determining where a dropped item goes.
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef enum
{
  BOBGUI_ICON_VIEW_NO_DROP,
  BOBGUI_ICON_VIEW_DROP_INTO,
  BOBGUI_ICON_VIEW_DROP_LEFT,
  BOBGUI_ICON_VIEW_DROP_RIGHT,
  BOBGUI_ICON_VIEW_DROP_ABOVE,
  BOBGUI_ICON_VIEW_DROP_BELOW
} BobguiIconViewDropPosition;

GDK_AVAILABLE_IN_ALL
GType          bobgui_icon_view_get_type          (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
BobguiWidget *    bobgui_icon_view_new               (void);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
BobguiWidget *    bobgui_icon_view_new_with_area     (BobguiCellArea    *area);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
BobguiWidget *    bobgui_icon_view_new_with_model    (BobguiTreeModel   *model);

GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_model         (BobguiIconView    *icon_view,
                                                BobguiTreeModel   *model);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
BobguiTreeModel * bobgui_icon_view_get_model         (BobguiIconView    *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_text_column   (BobguiIconView    *icon_view,
                                                int             column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int            bobgui_icon_view_get_text_column   (BobguiIconView    *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_markup_column (BobguiIconView    *icon_view,
                                                int             column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int            bobgui_icon_view_get_markup_column (BobguiIconView    *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_pixbuf_column (BobguiIconView    *icon_view,
                                                int             column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int            bobgui_icon_view_get_pixbuf_column (BobguiIconView    *icon_view);

GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_item_orientation (BobguiIconView    *icon_view,
                                                   BobguiOrientation  orientation);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
BobguiOrientation bobgui_icon_view_get_item_orientation (BobguiIconView    *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_columns       (BobguiIconView    *icon_view,
                                                int             columns);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int            bobgui_icon_view_get_columns       (BobguiIconView    *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_item_width    (BobguiIconView    *icon_view,
                                                int             item_width);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int            bobgui_icon_view_get_item_width    (BobguiIconView    *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_spacing       (BobguiIconView    *icon_view,
                                                int             spacing);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int            bobgui_icon_view_get_spacing       (BobguiIconView    *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_row_spacing   (BobguiIconView    *icon_view,
                                                int             row_spacing);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int            bobgui_icon_view_get_row_spacing   (BobguiIconView    *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_column_spacing (BobguiIconView    *icon_view,
                                                int             column_spacing);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int            bobgui_icon_view_get_column_spacing (BobguiIconView    *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_margin        (BobguiIconView    *icon_view,
                                                int             margin);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int            bobgui_icon_view_get_margin        (BobguiIconView    *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_item_padding  (BobguiIconView    *icon_view,
                                                int             item_padding);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int            bobgui_icon_view_get_item_padding  (BobguiIconView    *icon_view);

GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
BobguiTreePath *  bobgui_icon_view_get_path_at_pos   (BobguiIconView     *icon_view,
                                                int              x,
                                                int              y);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
gboolean       bobgui_icon_view_get_item_at_pos   (BobguiIconView     *icon_view,
                                                int               x,
                                                int               y,
                                                BobguiTreePath     **path,
                                                BobguiCellRenderer **cell);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
gboolean       bobgui_icon_view_get_visible_range (BobguiIconView      *icon_view,
                                                BobguiTreePath     **start_path,
                                                BobguiTreePath     **end_path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_activate_on_single_click (BobguiIconView  *icon_view,
                                                           gboolean      single);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
gboolean       bobgui_icon_view_get_activate_on_single_click (BobguiIconView  *icon_view);

GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_selected_foreach   (BobguiIconView            *icon_view,
                                                 BobguiIconViewForeachFunc  func,
                                                 gpointer                data);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void           bobgui_icon_view_set_selection_mode (BobguiIconView            *icon_view,
                                                 BobguiSelectionMode        mode);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
BobguiSelectionMode bobgui_icon_view_get_selection_mode (BobguiIconView            *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void             bobgui_icon_view_select_path        (BobguiIconView            *icon_view,
                                                   BobguiTreePath            *path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void             bobgui_icon_view_unselect_path      (BobguiIconView            *icon_view,
                                                   BobguiTreePath            *path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
gboolean         bobgui_icon_view_path_is_selected   (BobguiIconView            *icon_view,
                                                   BobguiTreePath            *path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int              bobgui_icon_view_get_item_row       (BobguiIconView            *icon_view,
                                                   BobguiTreePath            *path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int              bobgui_icon_view_get_item_column    (BobguiIconView            *icon_view,
                                                   BobguiTreePath            *path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
GList           *bobgui_icon_view_get_selected_items (BobguiIconView            *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void             bobgui_icon_view_select_all         (BobguiIconView            *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void             bobgui_icon_view_unselect_all       (BobguiIconView            *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void             bobgui_icon_view_item_activated     (BobguiIconView            *icon_view,
                                                   BobguiTreePath            *path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void             bobgui_icon_view_set_cursor         (BobguiIconView            *icon_view,
                                                   BobguiTreePath            *path,
                                                   BobguiCellRenderer        *cell,
                                                   gboolean                start_editing);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
gboolean         bobgui_icon_view_get_cursor         (BobguiIconView            *icon_view,
                                                   BobguiTreePath           **path,
                                                   BobguiCellRenderer       **cell);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void             bobgui_icon_view_scroll_to_path     (BobguiIconView            *icon_view,
                                                   BobguiTreePath            *path,
                                                   gboolean                use_align,
                                                   float                   row_align,
                                                   float                   col_align);

/* Drag-and-Drop support */
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void                   bobgui_icon_view_enable_model_drag_source (BobguiIconView              *icon_view,
                                                               GdkModifierType           start_button_mask,
                                                               GdkContentFormats        *formats,
                                                               GdkDragAction             actions);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void                   bobgui_icon_view_enable_model_drag_dest   (BobguiIconView              *icon_view,
                                                               GdkContentFormats        *formats,
                                                               GdkDragAction             actions);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void                   bobgui_icon_view_unset_model_drag_source  (BobguiIconView              *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void                   bobgui_icon_view_unset_model_drag_dest    (BobguiIconView              *icon_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void                   bobgui_icon_view_set_reorderable          (BobguiIconView              *icon_view,
                                                               gboolean                  reorderable);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
gboolean               bobgui_icon_view_get_reorderable          (BobguiIconView              *icon_view);


/* These are useful to implement your own custom stuff. */
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void                   bobgui_icon_view_set_drag_dest_item       (BobguiIconView              *icon_view,
                                                               BobguiTreePath              *path,
                                                               BobguiIconViewDropPosition   pos);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void                   bobgui_icon_view_get_drag_dest_item       (BobguiIconView              *icon_view,
                                                               BobguiTreePath             **path,
                                                               BobguiIconViewDropPosition  *pos);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
gboolean               bobgui_icon_view_get_dest_item_at_pos     (BobguiIconView              *icon_view,
                                                               int                       drag_x,
                                                               int                       drag_y,
                                                               BobguiTreePath             **path,
                                                               BobguiIconViewDropPosition  *pos);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
GdkPaintable          *bobgui_icon_view_create_drag_icon         (BobguiIconView              *icon_view,
                                                               BobguiTreePath              *path);

GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
gboolean bobgui_icon_view_get_cell_rect                          (BobguiIconView     *icon_view,
                                                               BobguiTreePath     *path,
                                                               BobguiCellRenderer *cell,
                                                               GdkRectangle    *rect);


GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void    bobgui_icon_view_set_tooltip_item                        (BobguiIconView     *icon_view,
                                                               BobguiTooltip      *tooltip,
                                                               BobguiTreePath     *path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void    bobgui_icon_view_set_tooltip_cell                        (BobguiIconView     *icon_view,
                                                               BobguiTooltip      *tooltip,
                                                               BobguiTreePath     *path,
                                                               BobguiCellRenderer *cell);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
gboolean bobgui_icon_view_get_tooltip_context                    (BobguiIconView       *icon_view,
                                                               int                x,
                                                               int                y,
                                                               gboolean           keyboard_tip,
                                                               BobguiTreeModel     **model,
                                                               BobguiTreePath      **path,
                                                               BobguiTreeIter       *iter);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
void     bobgui_icon_view_set_tooltip_column                     (BobguiIconView       *icon_view,
                                                               int                column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiGridView)
int      bobgui_icon_view_get_tooltip_column                     (BobguiIconView       *icon_view);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiIconView, g_object_unref)

G_END_DECLS
