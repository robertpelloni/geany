/* bobguitreemodel.h
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

#include <glib-object.h>
#include <gdk/gdk.h>

G_BEGIN_DECLS

#define BOBGUI_TYPE_TREE_MODEL            (bobgui_tree_model_get_type ())
#define BOBGUI_TREE_MODEL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), BOBGUI_TYPE_TREE_MODEL, BobguiTreeModel))
#define BOBGUI_IS_TREE_MODEL(obj)	       (G_TYPE_CHECK_INSTANCE_TYPE ((obj), BOBGUI_TYPE_TREE_MODEL))
#define BOBGUI_TREE_MODEL_GET_IFACE(obj)  (G_TYPE_INSTANCE_GET_INTERFACE ((obj), BOBGUI_TYPE_TREE_MODEL, BobguiTreeModelIface))

#define BOBGUI_TYPE_TREE_ITER             (bobgui_tree_iter_get_type ())
#define BOBGUI_TYPE_TREE_PATH             (bobgui_tree_path_get_type ())
#define BOBGUI_TYPE_TREE_ROW_REFERENCE    (bobgui_tree_row_reference_get_type ())

typedef struct _BobguiTreeIter         BobguiTreeIter;
typedef struct _BobguiTreePath         BobguiTreePath;
typedef struct _BobguiTreeRowReference BobguiTreeRowReference;
typedef struct _BobguiTreeModel        BobguiTreeModel; /* Dummy typedef */
typedef struct _BobguiTreeModelIface   BobguiTreeModelIface;

/**
 * BobguiTreeModelForeachFunc:
 * @model: the `BobguiTreeModel` being iterated
 * @path: the current `BobguiTreePath`
 * @iter: the current `BobguiTreeIter`
 * @data: (closure): The user data passed to bobgui_tree_model_foreach()
 *
 * Type of the callback passed to bobgui_tree_model_foreach() to
 * iterate over the rows in a tree model.
 *
 * Returns: %TRUE to stop iterating, %FALSE to continue
 *
 * Deprecated: 4.20: There is no replacement.
 */
typedef gboolean (* BobguiTreeModelForeachFunc) (BobguiTreeModel *model, BobguiTreePath *path, BobguiTreeIter *iter, gpointer data);

/**
 * BobguiTreeModelFlags:
 * @BOBGUI_TREE_MODEL_ITERS_PERSIST: iterators survive all signals
 *   emitted by the tree
 * @BOBGUI_TREE_MODEL_LIST_ONLY: the model is a list only, and never
 *   has children
 *
 * These flags indicate various properties of a `BobguiTreeModel`.
 *
 * They are returned by [method@Bobgui.TreeModel.get_flags], and must be
 * static for the lifetime of the object. A more complete description
 * of %BOBGUI_TREE_MODEL_ITERS_PERSIST can be found in the overview of
 * this section.
 *
 * Deprecated: 4.10: There is no replacement
 */
typedef enum
{
  BOBGUI_TREE_MODEL_ITERS_PERSIST = 1 << 0,
  BOBGUI_TREE_MODEL_LIST_ONLY = 1 << 1
} BobguiTreeModelFlags;

/**
 * BobguiTreeIter:
 * @stamp: a unique stamp to catch invalid iterators
 * @user_data: model-specific data
 * @user_data2: model-specific data
 * @user_data3: model-specific data
 *
 * The `BobguiTreeIter` is the primary structure
 * for accessing a `BobguiTreeModel`. Models are expected to put a unique
 * integer in the @stamp member, and put
 * model-specific data in the three @user_data
 * members.
 *
 * Deprecated: 4.10
 */
struct _BobguiTreeIter
{
  int stamp;
  gpointer user_data;
  gpointer user_data2;
  gpointer user_data3;
};

/**
 * BobguiTreeModelIface:
 * @row_changed: Signal emitted when a row in the model has changed.
 * @row_inserted: Signal emitted when a new row has been inserted in
 *    the model.
 * @row_has_child_toggled: Signal emitted when a row has gotten the
 *    first child row or lost its last child row.
 * @row_deleted: Signal emitted when a row has been deleted.
 * @rows_reordered: Signal emitted when the children of a node in the
 *    BobguiTreeModel have been reordered.
 * @get_flags: Get `BobguiTreeModelFlags` supported by this interface.
 * @get_n_columns: Get the number of columns supported by the model.
 * @get_column_type: Get the type of the column.
 * @get_iter: Sets iter to a valid iterator pointing to path.
 * @get_path: Gets a newly-created `BobguiTreePath` referenced by iter.
 * @get_value: Initializes and sets value to that at column.
 * @iter_next: Sets iter to point to the node following it at the
 *    current level.
 * @iter_previous: Sets iter to point to the previous node at the
 *    current level.
 * @iter_children: Sets iter to point to the first child of parent.
 * @iter_has_child: %TRUE if iter has children, %FALSE otherwise.
 * @iter_n_children: Gets the number of children that iter has.
 * @iter_nth_child: Sets iter to be the child of parent, using the
 *    given index.
 * @iter_parent: Sets iter to be the parent of child.
 * @ref_node: Lets the tree ref the node.
 * @unref_node: Lets the tree unref the node.
 */
struct _BobguiTreeModelIface
{
  /*< private >*/
  GTypeInterface g_iface;

  /*< public >*/

  /* Signals */
  void         (* row_changed)           (BobguiTreeModel *tree_model,
					  BobguiTreePath  *path,
					  BobguiTreeIter  *iter);
  void         (* row_inserted)          (BobguiTreeModel *tree_model,
					  BobguiTreePath  *path,
					  BobguiTreeIter  *iter);
  void         (* row_has_child_toggled) (BobguiTreeModel *tree_model,
					  BobguiTreePath  *path,
					  BobguiTreeIter  *iter);
  void         (* row_deleted)           (BobguiTreeModel *tree_model,
					  BobguiTreePath  *path);
  void         (* rows_reordered)        (BobguiTreeModel *tree_model,
					  BobguiTreePath  *path,
					  BobguiTreeIter  *iter,
					  int          *new_order);

  /* Virtual Table */
  BobguiTreeModelFlags (* get_flags)  (BobguiTreeModel *tree_model);

  int          (* get_n_columns)   (BobguiTreeModel *tree_model);
  GType        (* get_column_type) (BobguiTreeModel *tree_model,
				    int           index_);
  gboolean     (* get_iter)        (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter,
				    BobguiTreePath  *path);
  BobguiTreePath *(* get_path)        (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);
  void         (* get_value)       (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter,
				    int           column,
				    GValue       *value);
  gboolean     (* iter_next)       (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);
  gboolean     (* iter_previous)   (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);
  gboolean     (* iter_children)   (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter,
				    BobguiTreeIter  *parent);
  gboolean     (* iter_has_child)  (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);
  int          (* iter_n_children) (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);
  gboolean     (* iter_nth_child)  (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter,
				    BobguiTreeIter  *parent,
				    int           n);
  gboolean     (* iter_parent)     (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter,
				    BobguiTreeIter  *child);
  void         (* ref_node)        (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);
  void         (* unref_node)      (BobguiTreeModel *tree_model,
				    BobguiTreeIter  *iter);
};


/* BobguiTreePath operations */
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreePath *bobgui_tree_path_new              (void);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreePath *bobgui_tree_path_new_from_string  (const char        *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreePath *bobgui_tree_path_new_from_indices (int                first_index,
					     ...);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreePath *bobgui_tree_path_new_from_indicesv (int              *indices,
					      gsize             length);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
char        *bobgui_tree_path_to_string        (BobguiTreePath       *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreePath *bobgui_tree_path_new_first        (void);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void         bobgui_tree_path_append_index     (BobguiTreePath       *path,
					     int                index_);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void         bobgui_tree_path_prepend_index    (BobguiTreePath       *path,
					     int                index_);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
int          bobgui_tree_path_get_depth        (BobguiTreePath       *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
int         *bobgui_tree_path_get_indices      (BobguiTreePath       *path);

GDK_DEPRECATED_IN_4_10_FOR(GListModel)
int         *bobgui_tree_path_get_indices_with_depth (BobguiTreePath *path,
						   int         *depth);

GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void         bobgui_tree_path_free             (BobguiTreePath       *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreePath *bobgui_tree_path_copy             (const BobguiTreePath *path);
GDK_AVAILABLE_IN_ALL
GType        bobgui_tree_path_get_type         (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
int          bobgui_tree_path_compare          (const BobguiTreePath *a,
					     const BobguiTreePath *b);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void         bobgui_tree_path_next             (BobguiTreePath       *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean     bobgui_tree_path_prev             (BobguiTreePath       *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean     bobgui_tree_path_up               (BobguiTreePath       *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void         bobgui_tree_path_down             (BobguiTreePath       *path);

GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean     bobgui_tree_path_is_ancestor      (BobguiTreePath       *path,
                                             BobguiTreePath       *descendant);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean     bobgui_tree_path_is_descendant    (BobguiTreePath       *path,
                                             BobguiTreePath       *ancestor);

/**
 * BobguiTreeRowReference:
 *
 * A BobguiTreeRowReference tracks model changes so that it always refers to the
 * same row (a `BobguiTreePath` refers to a position, not a fixed row). Create a
 * new BobguiTreeRowReference with bobgui_tree_row_reference_new().
 *
 * Deprecated: 4.10: Use [iface@Gio.ListModel] instead
 */

GDK_AVAILABLE_IN_ALL
GType                bobgui_tree_row_reference_get_type (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreeRowReference *bobgui_tree_row_reference_new       (BobguiTreeModel        *model,
						       BobguiTreePath         *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreeRowReference *bobgui_tree_row_reference_new_proxy (GObject             *proxy,
						       BobguiTreeModel        *model,
						       BobguiTreePath         *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreePath         *bobgui_tree_row_reference_get_path  (BobguiTreeRowReference *reference);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreeModel        *bobgui_tree_row_reference_get_model (BobguiTreeRowReference *reference);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean             bobgui_tree_row_reference_valid     (BobguiTreeRowReference *reference);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreeRowReference *bobgui_tree_row_reference_copy      (BobguiTreeRowReference *reference);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void                 bobgui_tree_row_reference_free      (BobguiTreeRowReference *reference);
/* These two functions are only needed if you created the row reference with a
 * proxy object */
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void                 bobgui_tree_row_reference_inserted  (GObject     *proxy,
						       BobguiTreePath *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void                 bobgui_tree_row_reference_deleted   (GObject     *proxy,
						       BobguiTreePath *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void                 bobgui_tree_row_reference_reordered (GObject     *proxy,
						       BobguiTreePath *path,
						       BobguiTreeIter *iter,
						       int         *new_order);

/* BobguiTreeIter operations */
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreeIter *     bobgui_tree_iter_copy             (BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void              bobgui_tree_iter_free             (BobguiTreeIter  *iter);
GDK_AVAILABLE_IN_ALL
GType             bobgui_tree_iter_get_type         (void) G_GNUC_CONST;

GDK_AVAILABLE_IN_ALL
GType             bobgui_tree_model_get_type        (void) G_GNUC_CONST;
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreeModelFlags bobgui_tree_model_get_flags       (BobguiTreeModel *tree_model);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
int               bobgui_tree_model_get_n_columns   (BobguiTreeModel *tree_model);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
GType             bobgui_tree_model_get_column_type (BobguiTreeModel *tree_model,
						  int           index_);


/* Iterator movement */
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean          bobgui_tree_model_get_iter        (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter,
						  BobguiTreePath  *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean          bobgui_tree_model_get_iter_from_string (BobguiTreeModel *tree_model,
						       BobguiTreeIter  *iter,
						       const char   *path_string);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
char *           bobgui_tree_model_get_string_from_iter (BobguiTreeModel *tree_model,
                                                       BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean          bobgui_tree_model_get_iter_first  (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
BobguiTreePath *     bobgui_tree_model_get_path        (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void              bobgui_tree_model_get_value       (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter,
						  int           column,
						  GValue       *value);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean          bobgui_tree_model_iter_previous   (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean          bobgui_tree_model_iter_next       (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean          bobgui_tree_model_iter_children   (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter,
						  BobguiTreeIter  *parent);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean          bobgui_tree_model_iter_has_child  (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
int               bobgui_tree_model_iter_n_children (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean          bobgui_tree_model_iter_nth_child  (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter,
						  BobguiTreeIter  *parent,
						  int           n);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
gboolean          bobgui_tree_model_iter_parent     (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter,
						  BobguiTreeIter  *child);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void              bobgui_tree_model_ref_node        (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void              bobgui_tree_model_unref_node      (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void              bobgui_tree_model_get             (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter,
						  ...);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void              bobgui_tree_model_get_valist      (BobguiTreeModel *tree_model,
						  BobguiTreeIter  *iter,
						  va_list       var_args);


GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void              bobgui_tree_model_foreach         (BobguiTreeModel            *model,
						  BobguiTreeModelForeachFunc  func,
						  gpointer                 user_data);

/* Signals */
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void bobgui_tree_model_row_changed           (BobguiTreeModel *tree_model,
					   BobguiTreePath  *path,
					   BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void bobgui_tree_model_row_inserted          (BobguiTreeModel *tree_model,
					   BobguiTreePath  *path,
					   BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void bobgui_tree_model_row_has_child_toggled (BobguiTreeModel *tree_model,
					   BobguiTreePath  *path,
					   BobguiTreeIter  *iter);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void bobgui_tree_model_row_deleted           (BobguiTreeModel *tree_model,
					   BobguiTreePath  *path);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void bobgui_tree_model_rows_reordered        (BobguiTreeModel *tree_model,
					   BobguiTreePath  *path,
					   BobguiTreeIter  *iter,
					   int          *new_order);
GDK_DEPRECATED_IN_4_10_FOR(GListModel)
void bobgui_tree_model_rows_reordered_with_length (BobguiTreeModel *tree_model,
						BobguiTreePath  *path,
						BobguiTreeIter  *iter,
						int          *new_order,
						int           length);

G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreeModel, g_object_unref)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreeIter, bobgui_tree_iter_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreePath, bobgui_tree_path_free)
G_DEFINE_AUTOPTR_CLEANUP_FUNC(BobguiTreeRowReference, bobgui_tree_row_reference_free)

G_END_DECLS

