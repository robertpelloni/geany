/* bobguitreeselection.h
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

#include <bobgui/deprecated/bobguitreeview.h>

G_BEGIN_DECLS


#define BOBGUI_TYPE_TREE_SELECTION			(bobgui_tree_selection_get_type ())
#define BOBGUI_TREE_SELECTION(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TREE_SELECTION, BobguiTreeSelection))
#define BOBGUI_IS_TREE_SELECTION(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TREE_SELECTION))

/**
 * BobguiTreeSelectionFunc:
 * @selection: A `BobguiTreeSelection`
 * @model: A `BobguiTreeModel` being viewed
 * @path: The `BobguiTreePath` of the row in question
 * @path_currently_selected: %TRUE, if the path is currently selected
 * @data: (closure): user data
 *
 * A function used by bobgui_tree_selection_set_select_function() to filter
 * whether or not a row may be selected. It is called whenever a row's
 * state might change.
 *
 * A return value of %TRUE indicates to @selection that it is okay to
 * change the selection.
 *
 * Returns: %TRUE, if the selection state of the row can be toggled
 *
 * Deprecated: 4.20: There is no replacement
 */
typedef gboolean (* BobguiTreeSelectionFunc)    (BobguiTreeSelection  *selection,
					      BobguiTreeModel      *model,
					      BobguiTreePath       *path,
                                              gboolean           path_currently_selected,
					      gpointer           data);

/**
 * BobguiTreeSelectionForeachFunc:
 * @model: The `BobguiTreeModel` being viewed
 * @path: The `BobguiTreePath` of a selected row
 * @iter: A `BobguiTreeIter` pointing to a selected row
 * @data: (closure): user data
 *
 * A function used by bobgui_tree_selection_selected_foreach() to map all
 * selected rows.  It will be called on every selected row in the view.
 *
 * Deprecated: 4.20: There is no replacement
 */
typedef void (* BobguiTreeSelectionForeachFunc) (BobguiTreeModel      *model,
					      BobguiTreePath       *path,
					      BobguiTreeIter       *iter,
					      gpointer           data);


GDK_AVAILABLE_IN_ALL
GType            bobgui_tree_selection_get_type            (void) G_GNUC_CONST;

GDK_DEPRECATED_IN_4_10
void             bobgui_tree_selection_set_mode            (BobguiTreeSelection            *selection,
							 BobguiSelectionMode             type);
GDK_DEPRECATED_IN_4_10
BobguiSelectionMode bobgui_tree_selection_get_mode        (BobguiTreeSelection            *selection);
GDK_DEPRECATED_IN_4_10
void             bobgui_tree_selection_set_select_function (BobguiTreeSelection            *selection,
							 BobguiTreeSelectionFunc         func,
							 gpointer                     data,
							 GDestroyNotify               destroy);
GDK_DEPRECATED_IN_4_10
gpointer         bobgui_tree_selection_get_user_data       (BobguiTreeSelection            *selection);
GDK_DEPRECATED_IN_4_10
BobguiTreeView*     bobgui_tree_selection_get_tree_view       (BobguiTreeSelection            *selection);

GDK_DEPRECATED_IN_4_10
BobguiTreeSelectionFunc bobgui_tree_selection_get_select_function (BobguiTreeSelection        *selection);

/* Only meaningful if BOBGUI_SELECTION_SINGLE or BOBGUI_SELECTION_BROWSE is set */
/* Use selected_foreach or get_selected_rows for BOBGUI_SELECTION_MULTIPLE */
GDK_DEPRECATED_IN_4_10
gboolean         bobgui_tree_selection_get_selected        (BobguiTreeSelection            *selection,
							 BobguiTreeModel               **model,
							 BobguiTreeIter                 *iter);
GDK_DEPRECATED_IN_4_10
GList *          bobgui_tree_selection_get_selected_rows   (BobguiTreeSelection            *selection,
                                                         BobguiTreeModel               **model);
GDK_DEPRECATED_IN_4_10
int              bobgui_tree_selection_count_selected_rows (BobguiTreeSelection            *selection);
GDK_DEPRECATED_IN_4_10
void             bobgui_tree_selection_selected_foreach    (BobguiTreeSelection            *selection,
							 BobguiTreeSelectionForeachFunc  func,
							 gpointer                     data);
GDK_DEPRECATED_IN_4_10
void             bobgui_tree_selection_select_path         (BobguiTreeSelection            *selection,
							 BobguiTreePath                 *path);
GDK_DEPRECATED_IN_4_10
void             bobgui_tree_selection_unselect_path       (BobguiTreeSelection            *selection,
							 BobguiTreePath                 *path);
GDK_DEPRECATED_IN_4_10
void             bobgui_tree_selection_select_iter         (BobguiTreeSelection            *selection,
							 BobguiTreeIter                 *iter);
GDK_DEPRECATED_IN_4_10
void             bobgui_tree_selection_unselect_iter       (BobguiTreeSelection            *selection,
							 BobguiTreeIter                 *iter);
GDK_DEPRECATED_IN_4_10
gboolean         bobgui_tree_selection_path_is_selected    (BobguiTreeSelection            *selection,
							 BobguiTreePath                 *path);
GDK_DEPRECATED_IN_4_10
gboolean         bobgui_tree_selection_iter_is_selected    (BobguiTreeSelection            *selection,
							 BobguiTreeIter                 *iter);
GDK_DEPRECATED_IN_4_10
void             bobgui_tree_selection_select_all          (BobguiTreeSelection            *selection);
GDK_DEPRECATED_IN_4_10
void             bobgui_tree_selection_unselect_all        (BobguiTreeSelection            *selection);
GDK_DEPRECATED_IN_4_10
void             bobgui_tree_selection_select_range        (BobguiTreeSelection            *selection,
							 BobguiTreePath                 *start_path,
							 BobguiTreePath                 *end_path);
GDK_DEPRECATED_IN_4_10
void             bobgui_tree_selection_unselect_range      (BobguiTreeSelection            *selection,
                                                         BobguiTreePath                 *start_path,
							 BobguiTreePath                 *end_path);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreeSelection, g_object_unref)

G_END_DECLS

