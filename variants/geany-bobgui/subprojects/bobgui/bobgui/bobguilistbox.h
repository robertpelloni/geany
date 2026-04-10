/*
 * Copyright (c) 2013 Red Hat, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 *
 */

#pragma once

#if !defined (__BOBGUI_H_INSIDE__) && !defined (BOBGUI_COMPILATION)
#error "Only <bobgui/bobgui.h> can be included directly."
#endif

#include <bobgui/bobguiwidget.h>

G_BEGIN_DECLS


#define BOBGUI_TYPE_LIST_BOX (bobgui_list_box_get_type ())
#define BOBGUI_LIST_BOX(obj) (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_LIST_BOX, BobguiListBox))
#define BOBGUI_IS_LIST_BOX(obj) (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_LIST_BOX))

typedef struct _BobguiListBox        BobguiListBox;
typedef struct _BobguiListBoxRow        BobguiListBoxRow;
typedef struct _BobguiListBoxRowClass   BobguiListBoxRowClass;

#define BOBGUI_TYPE_LIST_BOX_ROW            (bobgui_list_box_row_get_type ())
#define BOBGUI_LIST_BOX_ROW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_LIST_BOX_ROW, BobguiListBoxRow))
#define BOBGUI_LIST_BOX_ROW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_LIST_BOX_ROW, BobguiListBoxRowClass))
#define BOBGUI_IS_LIST_BOX_ROW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_LIST_BOX_ROW))
#define BOBGUI_IS_LIST_BOX_ROW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_LIST_BOX_ROW))
#define BOBGUI_LIST_BOX_ROW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_LIST_BOX_ROW, BobguiListBoxRowClass))

struct _BobguiListBoxRow
{
  BobguiWidget parent_instance;
};

/**
 * BobguiListBoxRowClass:
 * @parent_class: The parent class.
 * @activate:
 */
struct _BobguiListBoxRowClass
{
  BobguiWidgetClass parent_class;

  /*< public >*/

  void (* activate) (BobguiListBoxRow *row);

  /*< private >*/

  gpointer padding[8];
};

/**
 * BobguiListBoxFilterFunc:
 * @row: the row that may be filtered
 * @user_data: (closure): user data
 *
 * Will be called whenever the row changes or is added and lets you control
 * if the row should be visible or not.
 *
 * Returns: %TRUE if the row should be visible, %FALSE otherwise
 */
typedef gboolean (*BobguiListBoxFilterFunc) (BobguiListBoxRow *row,
                                          gpointer       user_data);

/**
 * BobguiListBoxSortFunc:
 * @row1: the first row
 * @row2: the second row
 * @user_data: (closure): user data
 *
 * Compare two rows to determine which should be first.
 *
 * Returns: < 0 if @row1 should be before @row2, 0 if they are
 *   equal and > 0 otherwise
 */
typedef int (*BobguiListBoxSortFunc) (BobguiListBoxRow *row1,
                                   BobguiListBoxRow *row2,
                                   gpointer       user_data);

/**
 * BobguiListBoxUpdateHeaderFunc:
 * @row: the row to update
 * @before: (nullable): the row before @row, or %NULL if it is first
 * @user_data: (closure): user data
 *
 * Whenever @row changes or which row is before @row changes this
 * is called, which lets you update the header on @row.
 *
 * You may remove or set a new one via [method@Bobgui.ListBoxRow.set_header]
 * or just change the state of the current header widget.
 */
typedef void (*BobguiListBoxUpdateHeaderFunc) (BobguiListBoxRow *row,
                                            BobguiListBoxRow *before,
                                            gpointer       user_data);

/**
 * BobguiListBoxCreateWidgetFunc:
 * @item: (type GObject): the item from the model for which to create a widget for
 * @user_data: (closure): user data
 *
 * Called for list boxes that are bound to a `GListModel` with
 * bobgui_list_box_bind_model() for each item that gets added to the model.
 *
 * If the widget returned is not a #BobguiListBoxRow widget, then the widget
 * will be inserted as the child of an intermediate #BobguiListBoxRow.
 *
 * Returns: (transfer full): a `BobguiWidget` that represents @item
 */
typedef BobguiWidget * (*BobguiListBoxCreateWidgetFunc) (gpointer item,
                                                   gpointer user_data);

GDK_AVAILABLE_IN_ALL
GType      bobgui_list_box_row_get_type      (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_list_box_row_new           (void);

GDK_AVAILABLE_IN_ALL
void       bobgui_list_box_row_set_child     (BobguiListBoxRow *row,
                                           BobguiWidget      *child);
GDK_AVAILABLE_IN_ALL
BobguiWidget *bobgui_list_box_row_get_child     (BobguiListBoxRow *row);

GDK_AVAILABLE_IN_ALL
BobguiWidget* bobgui_list_box_row_get_header    (BobguiListBoxRow *row);
GDK_AVAILABLE_IN_ALL
void       bobgui_list_box_row_set_header    (BobguiListBoxRow *row,
                                           BobguiWidget     *header);
GDK_AVAILABLE_IN_ALL
int        bobgui_list_box_row_get_index     (BobguiListBoxRow *row);
GDK_AVAILABLE_IN_ALL
void       bobgui_list_box_row_changed       (BobguiListBoxRow *row);

GDK_AVAILABLE_IN_ALL
gboolean   bobgui_list_box_row_is_selected   (BobguiListBoxRow *row);

GDK_AVAILABLE_IN_ALL
void       bobgui_list_box_row_set_selectable (BobguiListBoxRow *row,
                                            gboolean       selectable);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_list_box_row_get_selectable (BobguiListBoxRow *row);


GDK_AVAILABLE_IN_ALL
void       bobgui_list_box_row_set_activatable (BobguiListBoxRow *row,
                                             gboolean       activatable);
GDK_AVAILABLE_IN_ALL
gboolean   bobgui_list_box_row_get_activatable (BobguiListBoxRow *row);

GDK_AVAILABLE_IN_ALL
GType          bobgui_list_box_get_type                     (void) G_GNUC_CONST;
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_prepend                      (BobguiListBox                    *box,
                                                          BobguiWidget                     *child);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_append                       (BobguiListBox                    *box,
                                                          BobguiWidget                     *child);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_insert                       (BobguiListBox                    *box,
                                                          BobguiWidget                     *child,
                                                          int                            position);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_remove                       (BobguiListBox                    *box,
                                                          BobguiWidget                     *child);
GDK_AVAILABLE_IN_4_12
void           bobgui_list_box_remove_all                   (BobguiListBox                    *box);

GDK_AVAILABLE_IN_ALL
BobguiListBoxRow* bobgui_list_box_get_selected_row             (BobguiListBox                    *box);
GDK_AVAILABLE_IN_ALL
BobguiListBoxRow* bobgui_list_box_get_row_at_index             (BobguiListBox                    *box,
                                                          int                            index_);
GDK_AVAILABLE_IN_ALL
BobguiListBoxRow* bobgui_list_box_get_row_at_y                 (BobguiListBox                    *box,
                                                          int                            y);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_select_row                   (BobguiListBox                    *box,
                                                          BobguiListBoxRow                 *row);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_set_placeholder              (BobguiListBox                    *box,
                                                          BobguiWidget                     *placeholder);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_set_adjustment               (BobguiListBox                    *box,
                                                          BobguiAdjustment                 *adjustment);
GDK_AVAILABLE_IN_ALL
BobguiAdjustment *bobgui_list_box_get_adjustment               (BobguiListBox                    *box);

typedef void (* BobguiListBoxForeachFunc) (BobguiListBox      *box,
                                        BobguiListBoxRow   *row,
                                        gpointer         user_data);

GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_selected_foreach             (BobguiListBox                    *box,
                                                          BobguiListBoxForeachFunc          func,
                                                          gpointer                       data);
GDK_AVAILABLE_IN_ALL
GList         *bobgui_list_box_get_selected_rows            (BobguiListBox                    *box);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_unselect_row                 (BobguiListBox                    *box,
                                                          BobguiListBoxRow                 *row);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_select_all                   (BobguiListBox                    *box);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_unselect_all                 (BobguiListBox                    *box);

GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_set_selection_mode           (BobguiListBox                    *box,
                                                          BobguiSelectionMode               mode);
GDK_AVAILABLE_IN_ALL
BobguiSelectionMode bobgui_list_box_get_selection_mode         (BobguiListBox                    *box);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_set_filter_func              (BobguiListBox                    *box,
                                                          BobguiListBoxFilterFunc           filter_func,
                                                          gpointer                       user_data,
                                                          GDestroyNotify                 destroy);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_set_header_func              (BobguiListBox                    *box,
                                                          BobguiListBoxUpdateHeaderFunc     update_header,
                                                          gpointer                       user_data,
                                                          GDestroyNotify                 destroy);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_invalidate_filter            (BobguiListBox                    *box);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_invalidate_sort              (BobguiListBox                    *box);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_invalidate_headers           (BobguiListBox                    *box);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_set_sort_func                (BobguiListBox                    *box,
                                                          BobguiListBoxSortFunc             sort_func,
                                                          gpointer                       user_data,
                                                          GDestroyNotify                 destroy);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_set_activate_on_single_click (BobguiListBox                    *box,
                                                          gboolean                       single);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_list_box_get_activate_on_single_click (BobguiListBox                    *box);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_drag_unhighlight_row         (BobguiListBox                    *box);
GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_drag_highlight_row           (BobguiListBox                    *box,
                                                          BobguiListBoxRow                 *row);
GDK_AVAILABLE_IN_ALL
BobguiWidget*     bobgui_list_box_new                          (void);


GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_bind_model                   (BobguiListBox                   *box,
                                                          GListModel                   *model,
                                                          BobguiListBoxCreateWidgetFunc    create_widget_func,
                                                          gpointer                      user_data,
                                                          GDestroyNotify                user_data_free_func);

GDK_AVAILABLE_IN_ALL
void           bobgui_list_box_set_show_separators          (BobguiListBox                   *box,
                                                          gboolean                      show_separators);
GDK_AVAILABLE_IN_ALL
gboolean       bobgui_list_box_get_show_separators          (BobguiListBox                   *box);

GDK_AVAILABLE_IN_4_18
void               bobgui_list_box_set_tab_behavior (BobguiListBox         *box,
                                                  BobguiListTabBehavior  behavior);
GDK_AVAILABLE_IN_4_18
BobguiListTabBehavior bobgui_list_box_get_tab_behavior (BobguiListBox         *box);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiListBox, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiListBoxRow, g_object_unref)

G_END_DECLS

