/* bobguitreemodelsort.h
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

#include <gdk/gdk.h>
#include <bobgui/deprecated/bobguitreemodel.h>
#include <bobgui/deprecated/bobguitreesortable.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TREE_MODEL_SORT			(bobgui_tree_model_sort_get_type ())
#define BOBGUI_TREE_MODEL_SORT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TREE_MODEL_SORT, BobguiTreeModelSort))
#define BOBGUI_TREE_MODEL_SORT_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TREE_MODEL_SORT, BobguiTreeModelSortClass))
#define BOBGUI_IS_TREE_MODEL_SORT(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TREE_MODEL_SORT))
#define BOBGUI_IS_TREE_MODEL_SORT_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TREE_MODEL_SORT))
#define BOBGUI_TREE_MODEL_SORT_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TREE_MODEL_SORT, BobguiTreeModelSortClass))

typedef struct _BobguiTreeModelSort        BobguiTreeModelSort;
typedef struct _BobguiTreeModelSortClass   BobguiTreeModelSortClass;
typedef struct _BobguiTreeModelSortPrivate BobguiTreeModelSortPrivate;

struct _BobguiTreeModelSort
{
  GObject parent;

  /* < private > */
  BobguiTreeModelSortPrivate *priv;
};

struct _BobguiTreeModelSortClass
{
  GObjectClass parent_class;

  /* < private > */
  gpointer padding[8];
};


GDK_AVAILABLE_IN_ALL
GType         bobgui_tree_model_sort_get_type                   (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
BobguiTreeModel *bobgui_tree_model_sort_new_with_model             (BobguiTreeModel     *child_model);

GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
BobguiTreeModel *bobgui_tree_model_sort_get_model                  (BobguiTreeModelSort *tree_model);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
BobguiTreePath  *bobgui_tree_model_sort_convert_child_path_to_path (BobguiTreeModelSort *tree_model_sort,
							      BobguiTreePath      *child_path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
gboolean      bobgui_tree_model_sort_convert_child_iter_to_iter (BobguiTreeModelSort *tree_model_sort,
							      BobguiTreeIter      *sort_iter,
							      BobguiTreeIter      *child_iter);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
BobguiTreePath  *bobgui_tree_model_sort_convert_path_to_child_path (BobguiTreeModelSort *tree_model_sort,
							      BobguiTreePath      *sorted_path);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
void          bobgui_tree_model_sort_convert_iter_to_child_iter (BobguiTreeModelSort *tree_model_sort,
							      BobguiTreeIter      *child_iter,
							      BobguiTreeIter      *sorted_iter);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
void          bobgui_tree_model_sort_reset_default_sort_func    (BobguiTreeModelSort *tree_model_sort);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
void          bobgui_tree_model_sort_clear_cache                (BobguiTreeModelSort *tree_model_sort);
GDK_DEPRECATED_IN_4_10_FOR(BobguiFilterListModel)
gboolean      bobgui_tree_model_sort_iter_is_valid              (BobguiTreeModelSort *tree_model_sort,
                                                              BobguiTreeIter      *iter);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreeModelSort, g_object_unref)

G_END_DECLS

