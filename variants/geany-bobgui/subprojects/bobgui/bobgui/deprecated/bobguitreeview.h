/* bobguitreeview.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
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
#include <bobgui/deprecated/bobguitreemodel.h>
#include <bobgui/deprecated/bobguitreeviewcolumn.h>
#include <bobgui/bobguientry.h>

G_BEGIN_DECLS

/**
 * BobguiTreeViewDropPosition:
 * @BOBGUI_TREE_VIEW_DROP_BEFORE: dropped row is inserted before
 * @BOBGUI_TREE_VIEW_DROP_AFTER: dropped row is inserted after
 * @BOBGUI_TREE_VIEW_DROP_INTO_OR_BEFORE: dropped row becomes a child or is inserted before
 * @BOBGUI_TREE_VIEW_DROP_INTO_OR_AFTER: dropped row becomes a child or is inserted after
 *
 * An enum for determining where a dropped row goes.
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef enum
{
  /* drop before/after this row */
  BOBGUI_TREE_VIEW_DROP_BEFORE,
  BOBGUI_TREE_VIEW_DROP_AFTER,
  /* drop as a child of this row (with fallback to before or after
   * if into is not possible)
   */
  BOBGUI_TREE_VIEW_DROP_INTO_OR_BEFORE,
  BOBGUI_TREE_VIEW_DROP_INTO_OR_AFTER
} BobguiTreeViewDropPosition;

#define BOBGUI_TYPE_TREE_VIEW            (bobgui_tree_view_get_type ())
#define BOBGUI_TREE_VIEW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TREE_VIEW, BobguiTreeView))
#define BOBGUI_IS_TREE_VIEW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TREE_VIEW))
#define BOBGUI_TREE_VIEW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TREE_VIEW, BobguiTreeViewClass))
#define BOBGUI_IS_TREE_VIEW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TREE_VIEW))
#define BOBGUI_TREE_VIEW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TREE_VIEW, BobguiTreeViewClass))

typedef struct _BobguiTreeView           BobguiTreeView;
typedef struct _BobguiTreeViewClass      BobguiTreeViewClass;
typedef struct _BobguiTreeSelection      BobguiTreeSelection;

/**
 * BobguiTreeViewColumnDropFunc:
 * @tree_view: A `BobguiTreeView`
 * @column: The `BobguiTreeViewColumn` being dragged
 * @prev_column: A `BobguiTreeViewColumn` on one side of @column
 * @next_column: A `BobguiTreeViewColumn` on the other side of @column
 * @data: (closure): user data
 *
 * Function type for determining whether @column can be dropped in a
 * particular spot (as determined by @prev_column and @next_column).  In
 * left to right locales, @prev_column is on the left of the potential drop
 * spot, and @next_column is on the right.  In right to left mode, this is
 * reversed.  This function should return %TRUE if the spot is a valid drop
 * spot.  Please note that returning %TRUE does not actually indicate that
 * the column drop was made, but is meant only to indicate a possible drop
 * spot to the user.
 *
 * Returns: %TRUE, if @column can be dropped in this spot
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef gboolean (* BobguiTreeViewColumnDropFunc) (BobguiTreeView             *tree_view,
						BobguiTreeViewColumn       *column,
						BobguiTreeViewColumn       *prev_column,
						BobguiTreeViewColumn       *next_column,
						gpointer                 data);

/**
 * BobguiTreeViewMappingFunc:
 * @tree_view: A `BobguiTreeView`
 * @path: The path that’s expanded
 * @user_data: user data
 *
 * Function used for bobgui_tree_view_map_expanded_rows().
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef void     (* BobguiTreeViewMappingFunc)    (BobguiTreeView             *tree_view,
						BobguiTreePath             *path,
						gpointer                 user_data);

/**
 * BobguiTreeViewSearchEqualFunc:
 * @model: the `BobguiTreeModel` being searched
 * @column: the search column set by bobgui_tree_view_set_search_column()
 * @key: the key string to compare with
 * @iter: a `BobguiTreeIter` pointing the row of @model that should be compared
 *  with @key.
 * @search_data: (closure): user data from bobgui_tree_view_set_search_equal_func()
 *
 * A function used for checking whether a row in @model matches
 * a search key string entered by the user. Note the return value
 * is reversed from what you would normally expect, though it
 * has some similarity to strcmp() returning 0 for equal strings.
 *
 * Returns: %FALSE if the row matches, %TRUE otherwise.
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef gboolean (*BobguiTreeViewSearchEqualFunc) (BobguiTreeModel            *model,
						int                      column,
						const char              *key,
						BobguiTreeIter             *iter,
						gpointer                 search_data);

/**
 * BobguiTreeViewRowSeparatorFunc:
 * @model: the `BobguiTreeModel`
 * @iter: a `BobguiTreeIter` pointing at a row in @model
 * @data: (closure): user data
 *
 * Function type for determining whether the row pointed to by @iter should
 * be rendered as a separator. A common way to implement this is to have a
 * boolean column in the model, whose values the `BobguiTreeViewRowSeparatorFunc`
 * returns.
 *
 * Returns: %TRUE if the row is a separator
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef gboolean (*BobguiTreeViewRowSeparatorFunc) (BobguiTreeModel      *model,
						 BobguiTreeIter       *iter,
						 gpointer           data);

struct _BobguiTreeView
{
  BobguiWidget parent_instance;
};

struct _BobguiTreeViewClass
{
  BobguiWidgetClass parent_class;

  void     (* row_activated)              (BobguiTreeView       *tree_view,
                                           BobguiTreePath       *path,
                                           BobguiTreeViewColumn *column);
  gboolean (* test_expand_row)            (BobguiTreeView       *tree_view,
                                           BobguiTreeIter       *iter,
                                           BobguiTreePath       *path);
  gboolean (* test_collapse_row)          (BobguiTreeView       *tree_view,
                                           BobguiTreeIter       *iter,
                                           BobguiTreePath       *path);
  void     (* row_expanded)               (BobguiTreeView       *tree_view,
                                           BobguiTreeIter       *iter,
                                           BobguiTreePath       *path);
  void     (* row_collapsed)              (BobguiTreeView       *tree_view,
                                           BobguiTreeIter       *iter,
                                           BobguiTreePath       *path);
  void     (* columns_changed)            (BobguiTreeView       *tree_view);
  void     (* cursor_changed)             (BobguiTreeView       *tree_view);

  /* Key Binding signals */
  gboolean (* move_cursor)                (BobguiTreeView       *tree_view,
                                           BobguiMovementStep    step,
                                           int                count,
                                           gboolean           extend,
                                           gboolean           modify);
  gboolean (* select_all)                 (BobguiTreeView       *tree_view);
  gboolean (* unselect_all)               (BobguiTreeView       *tree_view);
  gboolean (* select_cursor_row)          (BobguiTreeView       *tree_view,
                                           gboolean           start_editing);
  gboolean (* toggle_cursor_row)          (BobguiTreeView       *tree_view);
  gboolean (* expand_collapse_cursor_row) (BobguiTreeView       *tree_view,
                                           gboolean           logical,
                                           gboolean           expand,
                                           gboolean           open_all);
  gboolean (* select_cursor_parent)       (BobguiTreeView       *tree_view);
  gboolean (* start_interactive_search)   (BobguiTreeView       *tree_view);

  /*< private >*/
  gpointer _reserved[16];
};

GDK_AVAILABLE_IN_ALL
GType                  bobgui_tree_view_get_type                      (void) G_GNUC_CONST;

/* Creators */
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
BobguiWidget             *bobgui_tree_view_new                           (void);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
BobguiWidget             *bobgui_tree_view_new_with_model                (BobguiTreeModel              *model);

/* Accessors */
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
BobguiTreeModel          *bobgui_tree_view_get_model                     (BobguiTreeView               *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_set_model                     (BobguiTreeView               *tree_view,
								    BobguiTreeModel              *model);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
BobguiTreeSelection      *bobgui_tree_view_get_selection                 (BobguiTreeView               *tree_view);

GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean               bobgui_tree_view_get_headers_visible           (BobguiTreeView               *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_set_headers_visible           (BobguiTreeView               *tree_view,
								    gboolean                   headers_visible);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_columns_autosize              (BobguiTreeView               *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean               bobgui_tree_view_get_headers_clickable         (BobguiTreeView *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_set_headers_clickable         (BobguiTreeView               *tree_view,
								    gboolean                   setting);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean               bobgui_tree_view_get_activate_on_single_click  (BobguiTreeView               *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_set_activate_on_single_click  (BobguiTreeView               *tree_view,
								    gboolean                   single);

/* Column functions */
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
int                    bobgui_tree_view_append_column                 (BobguiTreeView               *tree_view,
								    BobguiTreeViewColumn         *column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
int                    bobgui_tree_view_remove_column                 (BobguiTreeView               *tree_view,
								    BobguiTreeViewColumn         *column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
int                    bobgui_tree_view_insert_column                 (BobguiTreeView               *tree_view,
								    BobguiTreeViewColumn         *column,
								    int                        position);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
int                    bobgui_tree_view_insert_column_with_attributes (BobguiTreeView               *tree_view,
								    int                        position,
								    const char                *title,
								    BobguiCellRenderer           *cell,
								    ...) G_GNUC_NULL_TERMINATED;
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
int                    bobgui_tree_view_insert_column_with_data_func  (BobguiTreeView               *tree_view,
								    int                        position,
								    const char                *title,
								    BobguiCellRenderer           *cell,
                                                                    BobguiTreeCellDataFunc        func,
                                                                    gpointer                   data,
                                                                    GDestroyNotify             dnotify);

GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
guint                  bobgui_tree_view_get_n_columns                 (BobguiTreeView               *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
BobguiTreeViewColumn     *bobgui_tree_view_get_column                    (BobguiTreeView               *tree_view,
								    int                        n);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
GList                 *bobgui_tree_view_get_columns                   (BobguiTreeView               *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_move_column_after             (BobguiTreeView               *tree_view,
								    BobguiTreeViewColumn         *column,
								    BobguiTreeViewColumn         *base_column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_set_expander_column           (BobguiTreeView               *tree_view,
								    BobguiTreeViewColumn         *column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
BobguiTreeViewColumn     *bobgui_tree_view_get_expander_column           (BobguiTreeView               *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_set_column_drag_function      (BobguiTreeView               *tree_view,
								    BobguiTreeViewColumnDropFunc  func,
								    gpointer                   user_data,
								    GDestroyNotify             destroy);

/* Actions */
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_scroll_to_point               (BobguiTreeView               *tree_view,
								    int                        tree_x,
								    int                        tree_y);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_scroll_to_cell                (BobguiTreeView               *tree_view,
								    BobguiTreePath               *path,
								    BobguiTreeViewColumn         *column,
								    gboolean                   use_align,
								    float                      row_align,
								    float                      col_align);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_row_activated                 (BobguiTreeView               *tree_view,
								    BobguiTreePath               *path,
								    BobguiTreeViewColumn         *column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_expand_all                    (BobguiTreeView               *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_collapse_all                  (BobguiTreeView               *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_expand_to_path                (BobguiTreeView               *tree_view,
								    BobguiTreePath               *path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean               bobgui_tree_view_expand_row                    (BobguiTreeView               *tree_view,
								    BobguiTreePath               *path,
								    gboolean                   open_all);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean               bobgui_tree_view_collapse_row                  (BobguiTreeView               *tree_view,
								    BobguiTreePath               *path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_map_expanded_rows             (BobguiTreeView               *tree_view,
								    BobguiTreeViewMappingFunc     func,
								    gpointer                   data);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean               bobgui_tree_view_row_expanded                  (BobguiTreeView               *tree_view,
								    BobguiTreePath               *path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_set_reorderable               (BobguiTreeView               *tree_view,
								    gboolean                   reorderable);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean               bobgui_tree_view_get_reorderable               (BobguiTreeView               *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_set_cursor                    (BobguiTreeView               *tree_view,
								    BobguiTreePath               *path,
								    BobguiTreeViewColumn         *focus_column,
								    gboolean                   start_editing);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_set_cursor_on_cell            (BobguiTreeView               *tree_view,
								    BobguiTreePath               *path,
								    BobguiTreeViewColumn         *focus_column,
								    BobguiCellRenderer           *focus_cell,
								    gboolean                   start_editing);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_get_cursor                    (BobguiTreeView               *tree_view,
								    BobguiTreePath              **path,
								    BobguiTreeViewColumn        **focus_column);


/* Layout information */
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean               bobgui_tree_view_get_path_at_pos               (BobguiTreeView               *tree_view,
								    int                        x,
								    int                        y,
								    BobguiTreePath              **path,
								    BobguiTreeViewColumn        **column,
								    int                       *cell_x,
								    int                       *cell_y);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_get_cell_area                 (BobguiTreeView               *tree_view,
								    BobguiTreePath               *path,
								    BobguiTreeViewColumn         *column,
								    GdkRectangle              *rect);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_get_background_area           (BobguiTreeView               *tree_view,
								    BobguiTreePath               *path,
								    BobguiTreeViewColumn         *column,
								    GdkRectangle              *rect);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_get_visible_rect              (BobguiTreeView               *tree_view,
								    GdkRectangle              *visible_rect);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean               bobgui_tree_view_get_visible_range             (BobguiTreeView               *tree_view,
								    BobguiTreePath              **start_path,
								    BobguiTreePath              **end_path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean               bobgui_tree_view_is_blank_at_pos               (BobguiTreeView               *tree_view,
                                                                    int                        x,
                                                                    int                        y,
                                                                    BobguiTreePath              **path,
                                                                    BobguiTreeViewColumn        **column,
                                                                    int                       *cell_x,
                                                                    int                       *cell_y);

/* Drag-and-Drop support */
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_enable_model_drag_source      (BobguiTreeView               *tree_view,
								    GdkModifierType            start_button_mask,
								    GdkContentFormats         *formats,
								    GdkDragAction              actions);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_enable_model_drag_dest        (BobguiTreeView               *tree_view,
								    GdkContentFormats         *formats,
								    GdkDragAction              actions);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_unset_rows_drag_source        (BobguiTreeView               *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_unset_rows_drag_dest          (BobguiTreeView               *tree_view);


/* These are useful to implement your own custom stuff. */
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_set_drag_dest_row             (BobguiTreeView               *tree_view,
								    BobguiTreePath               *path,
								    BobguiTreeViewDropPosition    pos);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                   bobgui_tree_view_get_drag_dest_row             (BobguiTreeView               *tree_view,
								    BobguiTreePath              **path,
								    BobguiTreeViewDropPosition   *pos);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean               bobgui_tree_view_get_dest_row_at_pos           (BobguiTreeView               *tree_view,
								    int                        drag_x,
								    int                        drag_y,
								    BobguiTreePath              **path,
								    BobguiTreeViewDropPosition   *pos);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
GdkPaintable          *bobgui_tree_view_create_row_drag_icon          (BobguiTreeView               *tree_view,
								    BobguiTreePath               *path);

/* Interactive search */
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                       bobgui_tree_view_set_enable_search     (BobguiTreeView                *tree_view,
								gboolean                    enable_search);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean                   bobgui_tree_view_get_enable_search     (BobguiTreeView                *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
int                        bobgui_tree_view_get_search_column     (BobguiTreeView                *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                       bobgui_tree_view_set_search_column     (BobguiTreeView                *tree_view,
								int                         column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
BobguiTreeViewSearchEqualFunc bobgui_tree_view_get_search_equal_func (BobguiTreeView                *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                       bobgui_tree_view_set_search_equal_func (BobguiTreeView                *tree_view,
								BobguiTreeViewSearchEqualFunc  search_equal_func,
								gpointer                    search_user_data,
								GDestroyNotify              search_destroy);

GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
BobguiEditable                  *bobgui_tree_view_get_search_entry         (BobguiTreeView                   *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                          bobgui_tree_view_set_search_entry         (BobguiTreeView                   *tree_view,
								      BobguiEditable                   *entry);

/* Convert between the different coordinate systems */
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void bobgui_tree_view_convert_widget_to_tree_coords       (BobguiTreeView *tree_view,
							int          wx,
							int          wy,
							int         *tx,
							int         *ty);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void bobgui_tree_view_convert_tree_to_widget_coords       (BobguiTreeView *tree_view,
							int          tx,
							int          ty,
							int         *wx,
							int         *wy);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void bobgui_tree_view_convert_widget_to_bin_window_coords (BobguiTreeView *tree_view,
							int          wx,
							int          wy,
							int         *bx,
							int         *by);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void bobgui_tree_view_convert_bin_window_to_widget_coords (BobguiTreeView *tree_view,
							int          bx,
							int          by,
							int         *wx,
							int         *wy);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void bobgui_tree_view_convert_tree_to_bin_window_coords   (BobguiTreeView *tree_view,
							int          tx,
							int          ty,
							int         *bx,
							int         *by);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void bobgui_tree_view_convert_bin_window_to_tree_coords   (BobguiTreeView *tree_view,
							int          bx,
							int          by,
							int         *tx,
							int         *ty);

GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void     bobgui_tree_view_set_fixed_height_mode (BobguiTreeView          *tree_view,
					      gboolean              enable);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean bobgui_tree_view_get_fixed_height_mode (BobguiTreeView          *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void     bobgui_tree_view_set_hover_selection   (BobguiTreeView          *tree_view,
					      gboolean              hover);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean bobgui_tree_view_get_hover_selection   (BobguiTreeView          *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void     bobgui_tree_view_set_hover_expand      (BobguiTreeView          *tree_view,
					      gboolean              expand);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean bobgui_tree_view_get_hover_expand      (BobguiTreeView          *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void     bobgui_tree_view_set_rubber_banding    (BobguiTreeView          *tree_view,
					      gboolean              enable);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean bobgui_tree_view_get_rubber_banding    (BobguiTreeView          *tree_view);

GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean bobgui_tree_view_is_rubber_banding_active (BobguiTreeView       *tree_view);

GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
BobguiTreeViewRowSeparatorFunc bobgui_tree_view_get_row_separator_func (BobguiTreeView               *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                        bobgui_tree_view_set_row_separator_func (BobguiTreeView                *tree_view,
								  BobguiTreeViewRowSeparatorFunc func,
								  gpointer                    data,
								  GDestroyNotify              destroy);

GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
BobguiTreeViewGridLines        bobgui_tree_view_get_grid_lines         (BobguiTreeView                *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                        bobgui_tree_view_set_grid_lines         (BobguiTreeView                *tree_view,
								  BobguiTreeViewGridLines        grid_lines);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean                    bobgui_tree_view_get_enable_tree_lines  (BobguiTreeView                *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                        bobgui_tree_view_set_enable_tree_lines  (BobguiTreeView                *tree_view,
								  gboolean                    enabled);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                        bobgui_tree_view_set_show_expanders     (BobguiTreeView                *tree_view,
								  gboolean                    enabled);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean                    bobgui_tree_view_get_show_expanders     (BobguiTreeView                *tree_view);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void                        bobgui_tree_view_set_level_indentation  (BobguiTreeView                *tree_view,
								  int                         indentation);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
int                         bobgui_tree_view_get_level_indentation  (BobguiTreeView                *tree_view);

/* Convenience functions for setting tooltips */
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void          bobgui_tree_view_set_tooltip_row    (BobguiTreeView       *tree_view,
						BobguiTooltip        *tooltip,
						BobguiTreePath       *path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void          bobgui_tree_view_set_tooltip_cell   (BobguiTreeView       *tree_view,
						BobguiTooltip        *tooltip,
						BobguiTreePath       *path,
						BobguiTreeViewColumn *column,
						BobguiCellRenderer   *cell);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
gboolean      bobgui_tree_view_get_tooltip_context(BobguiTreeView       *tree_view,
						int                x,
						int                y,
						gboolean           keyboard_tip,
						BobguiTreeModel     **model,
						BobguiTreePath      **path,
						BobguiTreeIter       *iter);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
void          bobgui_tree_view_set_tooltip_column (BobguiTreeView       *tree_view,
					        int                column);
GDK_DEPRECATED_IN_4_10_FOR(BobguiListView)
int           bobgui_tree_view_get_tooltip_column (BobguiTreeView       *tree_view);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreeView, g_object_unref)

G_END_DECLS


