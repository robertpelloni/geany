/* bobguitreestore.h
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
#include <stdarg.h>


G_BEGIN_DECLS


#define BOBGUI_TYPE_TREE_STORE			(bobgui_tree_store_get_type ())
#define BOBGUI_TREE_STORE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TREE_STORE, BobguiTreeStore))
#define BOBGUI_TREE_STORE_CLASS(klass)		(G_TYPE_CHECK_CLASS_CAST ((klass), BOBGUI_TYPE_TREE_STORE, BobguiTreeStoreClass))
#define BOBGUI_IS_TREE_STORE(obj)			(G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TREE_STORE))
#define BOBGUI_IS_TREE_STORE_CLASS(klass)		(G_TYPE_CHECK_CLASS_TYPE ((klass), BOBGUI_TYPE_TREE_STORE))
#define BOBGUI_TREE_STORE_GET_CLASS(obj)		(G_TYPE_INSTANCE_GET_CLASS ((obj), BOBGUI_TYPE_TREE_STORE, BobguiTreeStoreClass))

typedef struct _BobguiTreeStore        BobguiTreeStore;
typedef struct _BobguiTreeStoreClass   BobguiTreeStoreClass;
typedef struct _BobguiTreeStorePrivate BobguiTreeStorePrivate;

struct _BobguiTreeStore
{
  GObject parent;

  BobguiTreeStorePrivate *priv;
};

struct _BobguiTreeStoreClass
{
  GObjectClass parent_class;

  /*< private >*/
  gpointer padding[8];
};


GDK_AVAILABLE_IN_ALL
GType         bobgui_tree_store_get_type         (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
BobguiTreeStore *bobgui_tree_store_new              (int           n_columns,
					       ...);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
BobguiTreeStore *bobgui_tree_store_newv             (int           n_columns,
					       GType        *types);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_set_column_types (BobguiTreeStore *tree_store,
					       int           n_columns,
					       GType        *types);

/* NOTE: use bobgui_tree_model_get to get values from a BobguiTreeStore */

GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_set_value        (BobguiTreeStore *tree_store,
					       BobguiTreeIter  *iter,
					       int           column,
					       GValue       *value);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_set              (BobguiTreeStore *tree_store,
					       BobguiTreeIter  *iter,
					       ...);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_set_valuesv      (BobguiTreeStore *tree_store,
					       BobguiTreeIter  *iter,
					       int          *columns,
					       GValue       *values,
					       int           n_values);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_set_valist       (BobguiTreeStore *tree_store,
					       BobguiTreeIter  *iter,
					       va_list       var_args);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
gboolean      bobgui_tree_store_remove           (BobguiTreeStore *tree_store,
					       BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_insert           (BobguiTreeStore *tree_store,
					       BobguiTreeIter  *iter,
					       BobguiTreeIter  *parent,
					       int           position);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_insert_before    (BobguiTreeStore *tree_store,
					       BobguiTreeIter  *iter,
					       BobguiTreeIter  *parent,
					       BobguiTreeIter  *sibling);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_insert_after     (BobguiTreeStore *tree_store,
					       BobguiTreeIter  *iter,
					       BobguiTreeIter  *parent,
					       BobguiTreeIter  *sibling);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_insert_with_values (BobguiTreeStore *tree_store,
						 BobguiTreeIter  *iter,
						 BobguiTreeIter  *parent,
						 int           position,
						 ...);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_insert_with_valuesv (BobguiTreeStore *tree_store,
						  BobguiTreeIter  *iter,
						  BobguiTreeIter  *parent,
						  int           position,
						  int          *columns,
						  GValue       *values,
						  int           n_values);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_prepend          (BobguiTreeStore *tree_store,
					       BobguiTreeIter  *iter,
					       BobguiTreeIter  *parent);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_append           (BobguiTreeStore *tree_store,
					       BobguiTreeIter  *iter,
					       BobguiTreeIter  *parent);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
gboolean      bobgui_tree_store_is_ancestor      (BobguiTreeStore *tree_store,
					       BobguiTreeIter  *iter,
					       BobguiTreeIter  *descendant);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
int           bobgui_tree_store_iter_depth       (BobguiTreeStore *tree_store,
					       BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_clear            (BobguiTreeStore *tree_store);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
gboolean      bobgui_tree_store_iter_is_valid    (BobguiTreeStore *tree_store,
                                               BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_reorder          (BobguiTreeStore *tree_store,
                                               BobguiTreeIter  *parent,
                                               int          *new_order);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_swap             (BobguiTreeStore *tree_store,
                                               BobguiTreeIter  *a,
                                               BobguiTreeIter  *b);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_move_before      (BobguiTreeStore *tree_store,
                                               BobguiTreeIter  *iter,
                                               BobguiTreeIter  *position);
GDK_DEPRECATED_IN_4_10_FOR(BobguiTreeListModel)
void          bobgui_tree_store_move_after       (BobguiTreeStore *tree_store,
                                               BobguiTreeIter  *iter,
                                               BobguiTreeIter  *position);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreeStore, g_object_unref)

G_END_DECLS


