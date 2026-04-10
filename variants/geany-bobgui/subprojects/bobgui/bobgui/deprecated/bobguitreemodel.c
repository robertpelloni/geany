/* bobguitreemodel.c
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

#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <gobject/gvaluecollector.h>
#include "bobguitreemodel.h"
#include "bobguitreeview.h"
#include "bobguitreeprivate.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiTreeModel:
 *
 * The tree interface used by BobguiTreeView
 *
 * The `BobguiTreeModel` interface defines a generic tree interface for
 * use by the `BobguiTreeView` widget. It is an abstract interface, and
 * is designed to be usable with any appropriate data structure. The
 * programmer just has to implement this interface on their own data
 * type for it to be viewable by a `BobguiTreeView` widget.
 *
 * The model is represented as a hierarchical tree of strongly-typed,
 * columned data. In other words, the model can be seen as a tree where
 * every node has different values depending on which column is being
 * queried. The type of data found in a column is determined by using
 * the GType system (ie. %G_TYPE_INT, %BOBGUI_TYPE_BUTTON, %G_TYPE_POINTER,
 * etc). The types are homogeneous per column across all nodes. It is
 * important to note that this interface only provides a way of examining
 * a model and observing changes. The implementation of each individual
 * model decides how and if changes are made.
 *
 * In order to make life simpler for programmers who do not need to
 * write their own specialized model, two generic models are provided
 * — the `BobguiTreeStore` and the `BobguiListStore`. To use these, the
 * developer simply pushes data into these models as necessary. These
 * models provide the data structure as well as all appropriate tree
 * interfaces. As a result, implementing drag and drop, sorting, and
 * storing data is trivial. For the vast majority of trees and lists,
 * these two models are sufficient.
 *
 * Models are accessed on a node/column level of granularity. One can
 * query for the value of a model at a certain node and a certain
 * column on that node. There are two structures used to reference a
 * particular node in a model. They are the [struct@Bobgui.TreePath] and
 * the [struct@Bobgui.TreeIter] (“iter” is short for iterator). Most of the
 * interface consists of operations on a [struct@Bobgui.TreeIter].
 *
 * A path is essentially a potential node. It is a location on a model
 * that may or may not actually correspond to a node on a specific
 * model. A [struct@Bobgui.TreePath] can be converted into either an
 * array of unsigned integers or a string. The string form is a list
 * of numbers separated by a colon. Each number refers to the offset
 * at that level. Thus, the path `0` refers to the root
 * node and the path `2:4` refers to the fifth child of
 * the third node.
 *
 * By contrast, a [struct@Bobgui.TreeIter] is a reference to a specific node on
 * a specific model. It is a generic struct with an integer and three
 * generic pointers. These are filled in by the model in a model-specific
 * way. One can convert a path to an iterator by calling
 * bobgui_tree_model_get_iter(). These iterators are the primary way
 * of accessing a model and are similar to the iterators used by
 * `BobguiTextBuffer`. They are generally statically allocated on the
 * stack and only used for a short time. The model interface defines
 * a set of operations using them for navigating the model.
 *
 * It is expected that models fill in the iterator with private data.
 * For example, the `BobguiListStore` model, which is internally a simple
 * linked list, stores a list node in one of the pointers. The
 * `BobguiTreeModel`Sort stores an array and an offset in two of the
 * pointers. Additionally, there is an integer field. This field is
 * generally filled with a unique stamp per model. This stamp is for
 * catching errors resulting from using invalid iterators with a model.
 *
 * The lifecycle of an iterator can be a little confusing at first.
 * Iterators are expected to always be valid for as long as the model
 * is unchanged (and doesn’t emit a signal). The model is considered
 * to own all outstanding iterators and nothing needs to be done to
 * free them from the user’s point of view. Additionally, some models
 * guarantee that an iterator is valid for as long as the node it refers
 * to is valid (most notably the `BobguiTreeStore` and `BobguiListStore`).
 * Although generally uninteresting, as one always has to allow for
 * the case where iterators do not persist beyond a signal, some very
 * important performance enhancements were made in the sort model.
 * As a result, the %BOBGUI_TREE_MODEL_ITERS_PERSIST flag was added to
 * indicate this behavior.
 *
 * To help show some common operation of a model, some examples are
 * provided. The first example shows three ways of getting the iter at
 * the location `3:2:5`. While the first method shown is
 * easier, the second is much more common, as you often get paths from
 * callbacks.
 *
 * ## Acquiring a `BobguiTreeIter`
 *
 * ```c
 * // Three ways of getting the iter pointing to the location
 * BobguiTreePath *path;
 * BobguiTreeIter iter;
 * BobguiTreeIter parent_iter;
 *
 * // get the iterator from a string
 * bobgui_tree_model_get_iter_from_string (model,
 *                                      &iter,
 *                                      "3:2:5");
 *
 * // get the iterator from a path
 * path = bobgui_tree_path_new_from_string ("3:2:5");
 * bobgui_tree_model_get_iter (model, &iter, path);
 * bobgui_tree_path_free (path);
 *
 * // walk the tree to find the iterator
 * bobgui_tree_model_iter_nth_child (model, &iter,
 *                                NULL, 3);
 * parent_iter = iter;
 * bobgui_tree_model_iter_nth_child (model, &iter,
 *                                &parent_iter, 2);
 * parent_iter = iter;
 * bobgui_tree_model_iter_nth_child (model, &iter,
 *                                &parent_iter, 5);
 * ```
 *
 * This second example shows a quick way of iterating through a list
 * and getting a string and an integer from each row. The
 * populate_model() function used below is not
 * shown, as it is specific to the `BobguiListStore`. For information on
 * how to write such a function, see the `BobguiListStore` documentation.
 *
 * ## Reading data from a `BobguiTreeModel`
 *
 * ```c
 * enum
 * {
 *   STRING_COLUMN,
 *   INT_COLUMN,
 *   N_COLUMNS
 * };
 *
 * ...
 *
 * BobguiTreeModel *list_store;
 * BobguiTreeIter iter;
 * gboolean valid;
 * int row_count = 0;
 *
 * // make a new list_store
 * list_store = bobgui_list_store_new (N_COLUMNS,
 *                                  G_TYPE_STRING,
 *                                  G_TYPE_INT);
 *
 * // Fill the list store with data
 * populate_model (list_store);
 *
 * // Get the first iter in the list, check it is valid and walk
 * // through the list, reading each row.
 *
 * valid = bobgui_tree_model_get_iter_first (list_store,
 *                                        &iter);
 * while (valid)
 *  {
 *    char *str_data;
 *    int    int_data;
 *
 *    // Make sure you terminate calls to bobgui_tree_model_get() with a “-1” value
 *    bobgui_tree_model_get (list_store, &iter,
 *                        STRING_COLUMN, &str_data,
 *                        INT_COLUMN, &int_data,
 *                        -1);
 *
 *    // Do something with the data
 *    g_print ("Row %d: (%s,%d)\n",
 *             row_count, str_data, int_data);
 *    g_free (str_data);
 *
 *    valid = bobgui_tree_model_iter_next (list_store,
 *                                      &iter);
 *    row_count++;
 *  }
 * ```
 *
 * The `BobguiTreeModel` interface contains two methods for reference
 * counting: bobgui_tree_model_ref_node() and bobgui_tree_model_unref_node().
 * These two methods are optional to implement. The reference counting
 * is meant as a way for views to let models know when nodes are being
 * displayed. `BobguiTreeView` will take a reference on a node when it is
 * visible, which means the node is either in the toplevel or expanded.
 * Being displayed does not mean that the node is currently directly
 * visible to the user in the viewport. Based on this reference counting
 * scheme a caching model, for example, can decide whether or not to cache
 * a node based on the reference count. A file-system based model would
 * not want to keep the entire file hierarchy in memory, but just the
 * folders that are currently expanded in every current view.
 *
 * When working with reference counting, the following rules must be taken
 * into account:
 *
 * - Never take a reference on a node without owning a reference on its parent.
 *   This means that all parent nodes of a referenced node must be referenced
 *   as well.
 *
 * - Outstanding references on a deleted node are not released. This is not
 *   possible because the node has already been deleted by the time the
 *   row-deleted signal is received.
 *
 * - Models are not obligated to emit a signal on rows of which none of its
 *   siblings are referenced. To phrase this differently, signals are only
 *   required for levels in which nodes are referenced. For the root level
 *   however, signals must be emitted at all times (however the root level
 *   is always referenced when any view is attached).
 *
 * Deprecated: 4.10: Use [iface@Gio.ListModel] instead
 */

#define INITIALIZE_TREE_ITER(Iter) \
    G_STMT_START{ \
      (Iter)->stamp = 0; \
      (Iter)->user_data  = NULL; \
      (Iter)->user_data2 = NULL; \
      (Iter)->user_data3 = NULL; \
    }G_STMT_END

#define ROW_REF_DATA_STRING "bobgui-tree-row-refs"

enum {
  ROW_CHANGED,
  ROW_INSERTED,
  ROW_HAS_CHILD_TOGGLED,
  ROW_DELETED,
  ROWS_REORDERED,
  LAST_SIGNAL
};

static guint tree_model_signals[LAST_SIGNAL] = { 0 };

/**
 * BobguiTreePath:
 *
 * An opaque structure representing a path to a row in a model.
 *
 * Deprecated: 4.10
 */
struct _BobguiTreePath
{
  int depth;    /* Number of elements */
  int alloc;    /* Number of allocated elements */
  int *indices;
};

typedef struct
{
  GSList *list;
} RowRefList;

static void      bobgui_tree_model_base_init   (gpointer           g_class);

/* custom closures */
static void      row_inserted_marshal       (GClosure          *closure,
                                             GValue /* out */  *return_value,
                                             guint              n_param_value,
                                             const GValue      *param_values,
                                             gpointer           invocation_hint,
                                             gpointer           marshal_data);
static void      row_deleted_marshal        (GClosure          *closure,
                                             GValue /* out */  *return_value,
                                             guint              n_param_value,
                                             const GValue      *param_values,
                                             gpointer           invocation_hint,
                                             gpointer           marshal_data);
static void      rows_reordered_marshal     (GClosure          *closure,
                                             GValue /* out */  *return_value,
                                             guint              n_param_value,
                                             const GValue      *param_values,
                                             gpointer           invocation_hint,
                                             gpointer           marshal_data);

static void      bobgui_tree_row_ref_inserted  (RowRefList        *refs,
                                             BobguiTreePath       *path,
                                             BobguiTreeIter       *iter);
static void      bobgui_tree_row_ref_deleted   (RowRefList        *refs,
                                             BobguiTreePath       *path);
static void      bobgui_tree_row_ref_reordered (RowRefList        *refs,
                                             BobguiTreePath       *path,
                                             BobguiTreeIter       *iter,
                                             int               *new_order);

GType
bobgui_tree_model_get_type (void)
{
  static GType tree_model_type = 0;

  if (! tree_model_type)
    {
      const GTypeInfo tree_model_info =
      {
        sizeof (BobguiTreeModelIface), /* class_size */
        bobgui_tree_model_base_init,   /* base_init */
        NULL,           /* base_finalize */
        NULL,
        NULL,           /* class_finalize */
        NULL,           /* class_data */
        0,
        0,              /* n_preallocs */
        NULL
      };

      tree_model_type =
        g_type_register_static (G_TYPE_INTERFACE, I_("BobguiTreeModel"),
                                &tree_model_info, 0);

      g_type_interface_add_prerequisite (tree_model_type, G_TYPE_OBJECT);
    }

  return tree_model_type;
}

static void
bobgui_tree_model_base_init (gpointer g_class)
{
  static gboolean initialized = FALSE;
  GClosure *closure;

  if (! initialized)
    {
      GType row_inserted_params[2];
      GType row_deleted_params[1];
      GType rows_reordered_params[3];

      row_inserted_params[0] = BOBGUI_TYPE_TREE_PATH | G_SIGNAL_TYPE_STATIC_SCOPE;
      row_inserted_params[1] = BOBGUI_TYPE_TREE_ITER;

      row_deleted_params[0] = BOBGUI_TYPE_TREE_PATH | G_SIGNAL_TYPE_STATIC_SCOPE;

      rows_reordered_params[0] = BOBGUI_TYPE_TREE_PATH | G_SIGNAL_TYPE_STATIC_SCOPE;
      rows_reordered_params[1] = BOBGUI_TYPE_TREE_ITER;
      rows_reordered_params[2] = G_TYPE_POINTER;

      /**
       * BobguiTreeModel::row-changed:
       * @tree_model: the `BobguiTreeModel` on which the signal is emitted
       * @path: a `BobguiTreePath` identifying the changed row
       * @iter: a valid `BobguiTreeIter` pointing to the changed row
       *
       * This signal is emitted when a row in the model has changed.
       */
      tree_model_signals[ROW_CHANGED] =
        g_signal_new (I_("row-changed"),
                      BOBGUI_TYPE_TREE_MODEL,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BobguiTreeModelIface, row_changed),
                      NULL, NULL,
                      _bobgui_marshal_VOID__BOXED_BOXED,
                      G_TYPE_NONE, 2,
                      BOBGUI_TYPE_TREE_PATH | G_SIGNAL_TYPE_STATIC_SCOPE,
                      BOBGUI_TYPE_TREE_ITER);
      g_signal_set_va_marshaller (tree_model_signals[ROW_CHANGED],
                                  G_TYPE_FROM_CLASS (g_class),
                                  _bobgui_marshal_VOID__BOXED_BOXEDv);

      /* We need to get notification about structure changes
       * to update row references., so instead of using the
       * standard g_signal_new() with an offset into our interface
       * structure, we use a customs closures for the class
       * closures (default handlers) that first update row references
       * and then calls the function from the interface structure.
       *
       * The reason we don't simply update the row references from
       * the wrapper functions (bobgui_tree_model_row_inserted(), etc.)
       * is to keep proper ordering with respect to signal handlers
       * connected normally and after.
       */

      /**
       * BobguiTreeModel::row-inserted:
       * @tree_model: the `BobguiTreeModel` on which the signal is emitted
       * @path: a `BobguiTreePath` identifying the new row
       * @iter: a valid `BobguiTreeIter` pointing to the new row
       *
       * This signal is emitted when a new row has been inserted in
       * the model.
       *
       * Note that the row may still be empty at this point, since
       * it is a common pattern to first insert an empty row, and
       * then fill it with the desired values.
       */
      closure = g_closure_new_simple (sizeof (GClosure), NULL);
      g_closure_set_marshal (closure, row_inserted_marshal);
      tree_model_signals[ROW_INSERTED] =
        g_signal_newv (I_("row-inserted"),
                       BOBGUI_TYPE_TREE_MODEL,
                       G_SIGNAL_RUN_FIRST,
                       closure,
                       NULL, NULL,
                       _bobgui_marshal_VOID__BOXED_BOXED,
                       G_TYPE_NONE, 2,
                       row_inserted_params);
      g_signal_set_va_marshaller (tree_model_signals[ROW_INSERTED],
                                  G_TYPE_FROM_CLASS (g_class),
                                  _bobgui_marshal_VOID__BOXED_BOXEDv);

      /**
       * BobguiTreeModel::row-has-child-toggled:
       * @tree_model: the `BobguiTreeModel` on which the signal is emitted
       * @path: a `BobguiTreePath` identifying the row
       * @iter: a valid `BobguiTreeIter` pointing to the row
       *
       * This signal is emitted when a row has gotten the first child
       * row or lost its last child row.
       */
      tree_model_signals[ROW_HAS_CHILD_TOGGLED] =
        g_signal_new (I_("row-has-child-toggled"),
                      BOBGUI_TYPE_TREE_MODEL,
                      G_SIGNAL_RUN_LAST,
                      G_STRUCT_OFFSET (BobguiTreeModelIface, row_has_child_toggled),
                      NULL, NULL,
                      _bobgui_marshal_VOID__BOXED_BOXED,
                      G_TYPE_NONE, 2,
                      BOBGUI_TYPE_TREE_PATH | G_SIGNAL_TYPE_STATIC_SCOPE,
                      BOBGUI_TYPE_TREE_ITER);
      g_signal_set_va_marshaller (tree_model_signals[ROW_HAS_CHILD_TOGGLED],
                                  G_TYPE_FROM_CLASS (g_class),
                                  _bobgui_marshal_VOID__BOXED_BOXEDv);

      /**
       * BobguiTreeModel::row-deleted:
       * @tree_model: the `BobguiTreeModel` on which the signal is emitted
       * @path: a `BobguiTreePath` identifying the row
       *
       * This signal is emitted when a row has been deleted.
       *
       * Note that no iterator is passed to the signal handler,
       * since the row is already deleted.
       *
       * This should be called by models after a row has been removed.
       * The location pointed to by @path should be the location that
       * the row previously was at. It may not be a valid location anymore.
       */
      closure = g_closure_new_simple (sizeof (GClosure), NULL);
      g_closure_set_marshal (closure, row_deleted_marshal);
      tree_model_signals[ROW_DELETED] =
        g_signal_newv (I_("row-deleted"),
                       BOBGUI_TYPE_TREE_MODEL,
                       G_SIGNAL_RUN_FIRST,
                       closure,
                       NULL, NULL,
                       NULL,
                       G_TYPE_NONE, 1,
                       row_deleted_params);

      /**
       * BobguiTreeModel::rows-reordered: (skip)
       * @tree_model: the `BobguiTreeModel` on which the signal is emitted
       * @path: a `BobguiTreePath` identifying the tree node whose children
       *     have been reordered
       * @iter: a valid `BobguiTreeIter` pointing to the node whose children
       *     have been reordered, or %NULL if the depth of @path is 0
       * @new_order: an array of integers mapping the current position
       *     of each child to its old position before the re-ordering,
       *     i.e. @new_order`[newpos] = oldpos`
       *
       * This signal is emitted when the children of a node in the
       * `BobguiTreeModel` have been reordered.
       *
       * Note that this signal is not emitted
       * when rows are reordered by DND, since this is implemented
       * by removing and then reinserting the row.
       */
      closure = g_closure_new_simple (sizeof (GClosure), NULL);
      g_closure_set_marshal (closure, rows_reordered_marshal);
      tree_model_signals[ROWS_REORDERED] =
        g_signal_newv (I_("rows-reordered"),
                       BOBGUI_TYPE_TREE_MODEL,
                       G_SIGNAL_RUN_FIRST,
                       closure,
                       NULL, NULL,
                       _bobgui_marshal_VOID__BOXED_BOXED_POINTER,
                       G_TYPE_NONE, 3,
                       rows_reordered_params);
      g_signal_set_va_marshaller (tree_model_signals[ROWS_REORDERED],
                                  G_TYPE_FROM_CLASS (g_class),
                                  _bobgui_marshal_VOID__BOXED_BOXED_POINTERv);
      initialized = TRUE;
    }
}

static void
row_inserted_marshal (GClosure          *closure,
                      GValue /* out */  *return_value,
                      guint              n_param_values,
                      const GValue      *param_values,
                      gpointer           invocation_hint,
                      gpointer           marshal_data)
{
  BobguiTreeModelIface *iface;

  void (* row_inserted_callback) (BobguiTreeModel *tree_model,
                                  BobguiTreePath *path,
                                  BobguiTreeIter *iter) = NULL;

  GObject *model = g_value_get_object (param_values + 0);
  BobguiTreePath *path = (BobguiTreePath *)g_value_get_boxed (param_values + 1);
  BobguiTreeIter *iter = (BobguiTreeIter *)g_value_get_boxed (param_values + 2);

  /* first, we need to update internal row references */
  bobgui_tree_row_ref_inserted ((RowRefList *)g_object_get_data (model, ROW_REF_DATA_STRING),
                             path, iter);

  /* fetch the interface ->row_inserted implementation */
  iface = BOBGUI_TREE_MODEL_GET_IFACE (model);
  row_inserted_callback = G_STRUCT_MEMBER (gpointer, iface,
                              G_STRUCT_OFFSET (BobguiTreeModelIface,
                                               row_inserted));

  /* Call that default signal handler, it if has been set */
  if (row_inserted_callback)
    row_inserted_callback (BOBGUI_TREE_MODEL (model), path, iter);
}

static void
row_deleted_marshal (GClosure          *closure,
                     GValue /* out */  *return_value,
                     guint              n_param_values,
                     const GValue      *param_values,
                     gpointer           invocation_hint,
                     gpointer           marshal_data)
{
  BobguiTreeModelIface *iface;
  void (* row_deleted_callback) (BobguiTreeModel *tree_model,
                                 BobguiTreePath  *path) = NULL;
  GObject *model = g_value_get_object (param_values + 0);
  BobguiTreePath *path = (BobguiTreePath *)g_value_get_boxed (param_values + 1);

  /* first, we need to update internal row references */
  bobgui_tree_row_ref_deleted ((RowRefList *)g_object_get_data (model, ROW_REF_DATA_STRING),
                            path);

  /* fetch the interface ->row_deleted implementation */
  iface = BOBGUI_TREE_MODEL_GET_IFACE (model);
  row_deleted_callback = G_STRUCT_MEMBER (gpointer, iface,
                              G_STRUCT_OFFSET (BobguiTreeModelIface,
                                               row_deleted));

  /* Call that default signal handler, it if has been set */
  if (row_deleted_callback)
    row_deleted_callback (BOBGUI_TREE_MODEL (model), path);
}

static void
rows_reordered_marshal (GClosure          *closure,
                        GValue /* out */  *return_value,
                        guint              n_param_values,
                        const GValue      *param_values,
                        gpointer           invocation_hint,
                        gpointer           marshal_data)
{
  BobguiTreeModelIface *iface;
  void (* rows_reordered_callback) (BobguiTreeModel *tree_model,
                                    BobguiTreePath  *path,
                                    BobguiTreeIter  *iter,
                                    int          *new_order);

  GObject *model = g_value_get_object (param_values + 0);
  BobguiTreePath *path = (BobguiTreePath *)g_value_get_boxed (param_values + 1);
  BobguiTreeIter *iter = (BobguiTreeIter *)g_value_get_boxed (param_values + 2);
  int *new_order = (int *)g_value_get_pointer (param_values + 3);

  /* first, we need to update internal row references */
  bobgui_tree_row_ref_reordered ((RowRefList *)g_object_get_data (model, ROW_REF_DATA_STRING),
                              path, iter, new_order);

  /* fetch the interface ->rows_reordered implementation */
  iface = BOBGUI_TREE_MODEL_GET_IFACE (model);
  rows_reordered_callback = G_STRUCT_MEMBER (gpointer, iface,
                              G_STRUCT_OFFSET (BobguiTreeModelIface,
                                               rows_reordered));

  /* Call that default signal handler, it if has been set */
  if (rows_reordered_callback)
    rows_reordered_callback (BOBGUI_TREE_MODEL (model), path, iter, new_order);
}

/**
 * bobgui_tree_path_new:
 *
 * Creates a new `BobguiTreePath`
 * This refers to a row.
 *
 * Returns: A newly created `BobguiTreePath`.
 *
 * Deprecated: 4.10
 */
BobguiTreePath *
bobgui_tree_path_new (void)
{
  BobguiTreePath *retval;
  retval = g_slice_new (BobguiTreePath);
  retval->depth = 0;
  retval->alloc = 0;
  retval->indices = NULL;

  return retval;
}

/**
 * bobgui_tree_path_new_from_string:
 * @path: The string representation of a path
 *
 * Creates a new `BobguiTreePath` initialized to @path.
 *
 * @path is expected to be a colon separated list of numbers.
 * For example, the string “10:4:0” would create a path of depth
 * 3 pointing to the 11th child of the root node, the 5th
 * child of that 11th child, and the 1st child of that 5th child.
 * If an invalid path string is passed in, %NULL is returned.
 *
 * Returns: (nullable): A newly-created `BobguiTreePath`
 *
 * Deprecated: 4.10
 */
BobguiTreePath *
bobgui_tree_path_new_from_string (const char *path)
{
  BobguiTreePath *retval;
  const char *orig_path = path;
  char *ptr;
  int i;

  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (*path != '\000', NULL);

  retval = bobgui_tree_path_new ();

  while (1)
    {
      i = strtol (path, &ptr, 10);
      if (i < 0)
        {
          g_warning (G_STRLOC ": Negative numbers in path %s passed to bobgui_tree_path_new_from_string", orig_path);
          bobgui_tree_path_free (retval);
          return NULL;
        }

      bobgui_tree_path_append_index (retval, i);

      if (*ptr == '\000')
        break;
      if (ptr == path || *ptr != ':')
        {
          g_warning (G_STRLOC ": Invalid path %s passed to bobgui_tree_path_new_from_string", orig_path);
          bobgui_tree_path_free (retval);
          return NULL;
        }
      path = ptr + 1;
    }

  return retval;
}

/**
 * bobgui_tree_path_new_from_indices:
 * @first_index: first integer
 * @...: list of integers terminated by -1
 *
 * Creates a new path with @first_index and @varargs as indices.
 *
 * Returns: A newly created `BobguiTreePath`
 *
 * Deprecated: 4.10
 */
BobguiTreePath *
bobgui_tree_path_new_from_indices (int first_index,
                                ...)
{
  int arg;
  va_list args;
  BobguiTreePath *path;

  path = bobgui_tree_path_new ();

  va_start (args, first_index);
  arg = first_index;

  while (arg != -1)
    {
      bobgui_tree_path_append_index (path, arg);
      arg = va_arg (args, int);
    }

  va_end (args);

  return path;
}

/**
 * bobgui_tree_path_new_from_indicesv: (rename-to bobgui_tree_path_new_from_indices)
 * @indices: (array length=length): array of indices
 * @length: length of @indices array
 *
 * Creates a new path with the given @indices array of @length.
 *
 * Returns: A newly created `BobguiTreePath`
 *
 * Deprecated: 4.10
 */
BobguiTreePath *
bobgui_tree_path_new_from_indicesv (int *indices,
                                 gsize length)
{
  BobguiTreePath *path;

  g_return_val_if_fail (indices != NULL && length != 0, NULL);

  path = bobgui_tree_path_new ();
  path->alloc = length;
  path->depth = length;
  path->indices = g_new (int, length);
  memcpy (path->indices, indices, length * sizeof (int));

  return path;
}

/**
 * bobgui_tree_path_to_string:
 * @path: a `BobguiTreePath`
 *
 * Generates a string representation of the path.
 *
 * This string is a “:” separated list of numbers.
 * For example, “4:10:0:3” would be an acceptable
 * return value for this string. If the path has
 * depth 0, %NULL is returned.
 *
 * Returns: (nullable): A newly-allocated string
 *
 * Deprecated: 4.10
 */
char *
bobgui_tree_path_to_string (BobguiTreePath *path)
{
  char *retval, *ptr, *end;
  int i, n;

  g_return_val_if_fail (path != NULL, NULL);

  if (path->depth == 0)
    return NULL;

  n = path->depth * 12;
  ptr = retval = g_new0 (char, n);
  end = ptr + n;
  g_snprintf (retval, end - ptr, "%d", path->indices[0]);
  while (*ptr != '\000')
    ptr++;

  for (i = 1; i < path->depth; i++)
    {
      g_snprintf (ptr, end - ptr, ":%d", path->indices[i]);
      while (*ptr != '\000')
        ptr++;
    }

  return retval;
}

/**
 * bobgui_tree_path_new_first:
 *
 * Creates a new `BobguiTreePath`.
 *
 * The string representation of this path is “0”.
 *
 * Returns: A new `BobguiTreePath`
 *
 * Deprecated: 4.10
 */
BobguiTreePath *
bobgui_tree_path_new_first (void)
{
  BobguiTreePath *retval;

  retval = bobgui_tree_path_new ();
  bobgui_tree_path_append_index (retval, 0);

  return retval;
}

/**
 * bobgui_tree_path_append_index:
 * @path: a `BobguiTreePath`
 * @index_: the index
 *
 * Appends a new index to a path.
 *
 * As a result, the depth of the path is increased.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_path_append_index (BobguiTreePath *path,
                            int          index_)
{
  g_return_if_fail (path != NULL);
  g_return_if_fail (index_ >= 0);

  if (path->depth == path->alloc)
    {
      path->alloc = MAX (path->alloc * 2, 1);
      path->indices = g_renew (int, path->indices, path->alloc);
    }

  path->depth += 1;
  path->indices[path->depth - 1] = index_;
}

/**
 * bobgui_tree_path_prepend_index:
 * @path: a `BobguiTreePath`
 * @index_: the index
 *
 * Prepends a new index to a path.
 *
 * As a result, the depth of the path is increased.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_path_prepend_index (BobguiTreePath *path,
                             int        index)
{
  if (path->depth == path->alloc)
    {
      int *indices;
      path->alloc = MAX (path->alloc * 2, 1);
      indices = g_new (int, path->alloc);
      if (path->depth > 0)
        memcpy (indices + 1, path->indices, path->depth * sizeof (int));
      g_free (path->indices);
      path->indices = indices;
    }
  else if (path->depth > 0)
    memmove (path->indices + 1, path->indices, path->depth * sizeof (int));

  path->depth += 1;
  path->indices[0] = index;
}

/**
 * bobgui_tree_path_get_depth:
 * @path: a `BobguiTreePath`
 *
 * Returns the current depth of @path.
 *
 * Returns: The depth of @path
 *
 * Deprecated: 4.10
 */
int
bobgui_tree_path_get_depth (BobguiTreePath *path)
{
  g_return_val_if_fail (path != NULL, 0);

  return path->depth;
}

/**
 * bobgui_tree_path_get_indices: (skip)
 * @path: a `BobguiTreePath`
 *
 * Returns the current indices of @path.
 *
 * This is an array of integers, each representing a node in a tree.
 * This value should not be freed.
 *
 * The length of the array can be obtained with bobgui_tree_path_get_depth().
 *
 * Returns: (nullable) (transfer none): The current indices
 *
 * Deprecated: 4.10
 */
int *
bobgui_tree_path_get_indices (BobguiTreePath *path)
{
  g_return_val_if_fail (path != NULL, NULL);

  return path->indices;
}

/**
 * bobgui_tree_path_get_indices_with_depth: (rename-to bobgui_tree_path_get_indices)
 * @path: a `BobguiTreePath`
 * @depth: (out) (optional): return location for number of elements
 *   returned in the integer array
 *
 * Returns the current indices of @path.
 *
 * This is an array of integers, each representing a node in a tree.
 * It also returns the number of elements in the array.
 * The array should not be freed.
 *
 * Returns: (array length=depth) (transfer none) (nullable): The current
 *   indices
 *
 * Deprecated: 4.10
 */
int *
bobgui_tree_path_get_indices_with_depth (BobguiTreePath *path,
                                      int         *depth)
{
  g_return_val_if_fail (path != NULL, NULL);

  if (depth)
    *depth = path->depth;

  return path->indices;
}

/**
 * bobgui_tree_path_free:
 * @path: (nullable): a `BobguiTreePath`
 *
 * Frees @path. If @path is %NULL, it simply returns.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_path_free (BobguiTreePath *path)
{
  if (!path)
    return;

  g_free (path->indices);
  g_slice_free (BobguiTreePath, path);
}

/**
 * bobgui_tree_path_copy:
 * @path: a `BobguiTreePath`
 *
 * Creates a new `BobguiTreePath` as a copy of @path.
 *
 * Returns: a new `BobguiTreePath`
 *
 * Deprecated: 4.10
 */
BobguiTreePath *
bobgui_tree_path_copy (const BobguiTreePath *path)
{
  BobguiTreePath *retval;

  g_return_val_if_fail (path != NULL, NULL);

  retval = g_slice_new (BobguiTreePath);
  retval->depth = path->depth;
  retval->alloc = retval->depth;
  retval->indices = g_new (int, path->alloc);
  if (path->depth > 0)
    memcpy (retval->indices, path->indices, path->depth * sizeof (int));
  return retval;
}

G_DEFINE_BOXED_TYPE (BobguiTreePath, bobgui_tree_path,
                     bobgui_tree_path_copy,
                     bobgui_tree_path_free)

/**
 * bobgui_tree_path_compare:
 * @a: a `BobguiTreePath`
 * @b: a `BobguiTreePath` to compare with
 *
 * Compares two paths.
 *
 * If @a appears before @b in a tree, then -1 is returned.
 * If @b appears before @a, then 1 is returned.
 * If the two nodes are equal, then 0 is returned.
 *
 * Returns: the relative positions of @a and @b
 *
 * Deprecated: 4.10
 */
int
bobgui_tree_path_compare (const BobguiTreePath *a,
                       const BobguiTreePath *b)
{
  int p = 0, q = 0;

  g_return_val_if_fail (a != NULL, 0);
  g_return_val_if_fail (b != NULL, 0);
  g_return_val_if_fail (a->depth > 0, 0);
  g_return_val_if_fail (b->depth > 0, 0);

  do
    {
      if (a->indices[p] == b->indices[q])
        continue;
      return (a->indices[p] < b->indices[q]?-1:1);
    }
  while (++p < a->depth && ++q < b->depth);
  if (a->depth == b->depth)
    return 0;
  return (a->depth < b->depth?-1:1);
}

/**
 * bobgui_tree_path_is_ancestor:
 * @path: a `BobguiTreePath`
 * @descendant: another `BobguiTreePath`
 *
 * Returns %TRUE if @descendant is a descendant of @path.
 *
 * Returns: %TRUE if @descendant is contained inside @path
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_path_is_ancestor (BobguiTreePath *path,
                           BobguiTreePath *descendant)
{
  int i;

  g_return_val_if_fail (path != NULL, FALSE);
  g_return_val_if_fail (descendant != NULL, FALSE);

  /* can't be an ancestor if we're deeper */
  if (path->depth >= descendant->depth)
    return FALSE;

  i = 0;
  while (i < path->depth)
    {
      if (path->indices[i] != descendant->indices[i])
        return FALSE;
      ++i;
    }

  return TRUE;
}

/**
 * bobgui_tree_path_is_descendant:
 * @path: a `BobguiTreePath`
 * @ancestor: another `BobguiTreePath`
 *
 * Returns %TRUE if @path is a descendant of @ancestor.
 *
 * Returns: %TRUE if @ancestor contains @path somewhere below it
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_path_is_descendant (BobguiTreePath *path,
                             BobguiTreePath *ancestor)
{
  int i;

  g_return_val_if_fail (path != NULL, FALSE);
  g_return_val_if_fail (ancestor != NULL, FALSE);

  /* can't be a descendant if we're shallower in the tree */
  if (path->depth <= ancestor->depth)
    return FALSE;

  i = 0;
  while (i < ancestor->depth)
    {
      if (path->indices[i] != ancestor->indices[i])
        return FALSE;
      ++i;
    }

  return TRUE;
}


/**
 * bobgui_tree_path_next:
 * @path: a `BobguiTreePath`
 *
 * Moves the @path to point to the next node at the current depth.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_path_next (BobguiTreePath *path)
{
  g_return_if_fail (path != NULL);
  g_return_if_fail (path->depth > 0);

  path->indices[path->depth - 1] ++;
}

/**
 * bobgui_tree_path_prev:
 * @path: a `BobguiTreePath`
 *
 * Moves the @path to point to the previous node at the
 * current depth, if it exists.
 *
 * Returns: %TRUE if @path has a previous node, and
 *   the move was made
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_path_prev (BobguiTreePath *path)
{
  g_return_val_if_fail (path != NULL, FALSE);

  if (path->depth == 0)
    return FALSE;

  if (path->indices[path->depth - 1] == 0)
    return FALSE;

  path->indices[path->depth - 1] -= 1;

  return TRUE;
}

/**
 * bobgui_tree_path_up:
 * @path: a `BobguiTreePath`
 *
 * Moves the @path to point to its parent node, if it has a parent.
 *
 * Returns: %TRUE if @path has a parent, and the move was made
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_path_up (BobguiTreePath *path)
{
  g_return_val_if_fail (path != NULL, FALSE);

  if (path->depth == 0)
    return FALSE;

  path->depth--;

  return TRUE;
}

/**
 * bobgui_tree_path_down:
 * @path: a `BobguiTreePath`
 *
 * Moves @path to point to the first child of the current path.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_path_down (BobguiTreePath *path)
{
  g_return_if_fail (path != NULL);

  bobgui_tree_path_append_index (path, 0);
}

/**
 * bobgui_tree_iter_copy:
 * @iter: a `BobguiTreeIter`
 *
 * Creates a dynamically allocated tree iterator as a copy of @iter.
 *
 * This function is not intended for use in applications,
 * because you can just copy the structs by value
 * (`BobguiTreeIter new_iter = iter;`).
 * You must free this iter with bobgui_tree_iter_free().
 *
 * Returns: a newly-allocated copy of @iter
 *
 * Deprecated: 4.10
 */
BobguiTreeIter *
bobgui_tree_iter_copy (BobguiTreeIter *iter)
{
  BobguiTreeIter *retval;

  g_return_val_if_fail (iter != NULL, NULL);

  retval = g_slice_new (BobguiTreeIter);
  *retval = *iter;

  return retval;
}

/**
 * bobgui_tree_iter_free:
 * @iter: a dynamically allocated tree iterator
 *
 * Frees an iterator that has been allocated by bobgui_tree_iter_copy().
 *
 * This function is mainly used for language bindings.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_iter_free (BobguiTreeIter *iter)
{
  g_return_if_fail (iter != NULL);

  g_slice_free (BobguiTreeIter, iter);
}

G_DEFINE_BOXED_TYPE (BobguiTreeIter,  bobgui_tree_iter,
                     bobgui_tree_iter_copy,
                     bobgui_tree_iter_free)

/**
 * bobgui_tree_model_get_flags:
 * @tree_model: a `BobguiTreeModel`
 *
 * Returns a set of flags supported by this interface.
 *
 * The flags are a bitwise combination of `BobguiTreeModel`Flags.
 * The flags supported should not change during the lifetime
 * of the @tree_model.
 *
 * Returns: the flags supported by this interface
 *
 * Deprecated: 4.10
 */
BobguiTreeModelFlags
bobgui_tree_model_get_flags (BobguiTreeModel *tree_model)
{
  BobguiTreeModelIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), 0);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  if (iface->get_flags)
    return (* iface->get_flags) (tree_model);

  return 0;
}

/**
 * bobgui_tree_model_get_n_columns:
 * @tree_model: a `BobguiTreeModel`
 *
 * Returns the number of columns supported by @tree_model.
 *
 * Returns: the number of columns
 *
 * Deprecated: 4.10
 */
int
bobgui_tree_model_get_n_columns (BobguiTreeModel *tree_model)
{
  BobguiTreeModelIface *iface;
  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), 0);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  g_return_val_if_fail (iface->get_n_columns != NULL, 0);

  return (* iface->get_n_columns) (tree_model);
}

/**
 * bobgui_tree_model_get_column_type:
 * @tree_model: a `BobguiTreeModel`
 * @index_: the column index
 *
 * Returns the type of the column.
 *
 * Returns: the type of the column
 *
 * Deprecated: 4.10
 */
GType
bobgui_tree_model_get_column_type (BobguiTreeModel *tree_model,
                                int           index)
{
  BobguiTreeModelIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), G_TYPE_INVALID);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  g_return_val_if_fail (iface->get_column_type != NULL, G_TYPE_INVALID);
  g_return_val_if_fail (index >= 0, G_TYPE_INVALID);

  return (* iface->get_column_type) (tree_model, index);
}

/**
 * bobgui_tree_model_get_iter:
 * @tree_model: a `BobguiTreeModel`
 * @iter: (out): the uninitialized `BobguiTreeIter`
 * @path: the `BobguiTreePath`
 *
 * Sets @iter to a valid iterator pointing to @path.
 *
 * If @path does not exist, @iter is set to an invalid
 * iterator and %FALSE is returned.
 *
 * Returns: %TRUE, if @iter was set
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_model_get_iter (BobguiTreeModel *tree_model,
                         BobguiTreeIter  *iter,
                         BobguiTreePath  *path)
{
  BobguiTreeModelIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (path != NULL, FALSE);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  g_return_val_if_fail (iface->get_iter != NULL, FALSE);
  g_return_val_if_fail (path->depth > 0, FALSE);

  INITIALIZE_TREE_ITER (iter);

  return (* iface->get_iter) (tree_model, iter, path);
}

/**
 * bobgui_tree_model_get_iter_from_string:
 * @tree_model: a `BobguiTreeModel`
 * @iter: (out): an uninitialized `BobguiTreeIter`
 * @path_string: a string representation of a `BobguiTreePath`
 *
 * Sets @iter to a valid iterator pointing to @path_string, if it
 * exists.
 *
 * Otherwise, @iter is left invalid and %FALSE is returned.
 *
 * Returns: %TRUE, if @iter was set
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_model_get_iter_from_string (BobguiTreeModel *tree_model,
                                     BobguiTreeIter  *iter,
                                     const char   *path_string)
{
  gboolean retval;
  BobguiTreePath *path;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (path_string != NULL, FALSE);

  path = bobgui_tree_path_new_from_string (path_string);

  g_return_val_if_fail (path != NULL, FALSE);

  retval = bobgui_tree_model_get_iter (tree_model, iter, path);
  bobgui_tree_path_free (path);

  return retval;
}

/**
 * bobgui_tree_model_get_string_from_iter:
 * @tree_model: a `BobguiTreeModel`
 * @iter: a `BobguiTreeIter`
 *
 * Generates a string representation of the iter.
 *
 * This string is a “:” separated list of numbers.
 * For example, “4:10:0:3” would be an acceptable
 * return value for this string.
 *
 * Returns: (nullable): a newly-allocated string
 *
 * Deprecated: 4.10
 */
char *
bobgui_tree_model_get_string_from_iter (BobguiTreeModel *tree_model,
                                     BobguiTreeIter  *iter)
{
  BobguiTreePath *path;
  char *ret;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), NULL);
  g_return_val_if_fail (iter != NULL, NULL);

  path = bobgui_tree_model_get_path (tree_model, iter);

  g_return_val_if_fail (path != NULL, NULL);

  ret = bobgui_tree_path_to_string (path);
  bobgui_tree_path_free (path);

  return ret;
}

/**
 * bobgui_tree_model_get_iter_first:
 * @tree_model: a `BobguiTreeModel`
 * @iter: (out): the uninitialized `BobguiTreeIter`
 *
 * Initializes @iter with the first iterator in the tree
 * (the one at the path "0").
 *
 * Returns %FALSE if the tree is empty, %TRUE otherwise.
 *
 * Returns: %TRUE, if @iter was set
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_model_get_iter_first (BobguiTreeModel *tree_model,
                               BobguiTreeIter  *iter)
{
  BobguiTreePath *path;
  gboolean retval;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  path = bobgui_tree_path_new_first ();
  retval = bobgui_tree_model_get_iter (tree_model, iter, path);
  bobgui_tree_path_free (path);

  return retval;
}

/**
 * bobgui_tree_model_get_path:
 * @tree_model: a `BobguiTreeModel`
 * @iter: the `BobguiTreeIter`
 *
 * Returns a newly-created `BobguiTreePath` referenced by @iter.
 *
 * This path should be freed with bobgui_tree_path_free().
 *
 * Returns: a newly-created `BobguiTreePath`
 *
 * Deprecated: 4.10
 */
BobguiTreePath *
bobgui_tree_model_get_path (BobguiTreeModel *tree_model,
                         BobguiTreeIter  *iter)
{
  BobguiTreeModelIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), NULL);
  g_return_val_if_fail (iter != NULL, NULL);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  g_return_val_if_fail (iface->get_path != NULL, NULL);

  return (* iface->get_path) (tree_model, iter);
}

/**
 * bobgui_tree_model_get_value:
 * @tree_model: a `BobguiTreeModel`
 * @iter: the `BobguiTreeIter`
 * @column: the column to lookup the value at
 * @value: (out) (transfer none): an empty `GValue` to set
 *
 * Initializes and sets @value to that at @column.
 *
 * When done with @value, g_value_unset() needs to be called
 * to free any allocated memory.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_model_get_value (BobguiTreeModel *tree_model,
                          BobguiTreeIter  *iter,
                          int           column,
                          GValue       *value)
{
  BobguiTreeModelIface *iface;

  g_return_if_fail (BOBGUI_IS_TREE_MODEL (tree_model));
  g_return_if_fail (iter != NULL);
  g_return_if_fail (value != NULL);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  g_return_if_fail (iface->get_value != NULL);

  (* iface->get_value) (tree_model, iter, column, value);
}

/**
 * bobgui_tree_model_iter_next:
 * @tree_model: a `BobguiTreeModel`
 * @iter: (in): the `BobguiTreeIter`
 *
 * Sets @iter to point to the node following it at the current level.
 *
 * If there is no next @iter, %FALSE is returned and @iter is set
 * to be invalid.
 *
 * Returns: %TRUE if @iter has been changed to the next node
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_model_iter_next (BobguiTreeModel  *tree_model,
                          BobguiTreeIter   *iter)
{
  BobguiTreeModelIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  g_return_val_if_fail (iface->iter_next != NULL, FALSE);

  return (* iface->iter_next) (tree_model, iter);
}

static gboolean
bobgui_tree_model_iter_previous_default (BobguiTreeModel *tree_model,
                                      BobguiTreeIter  *iter)
{
  gboolean retval;
  BobguiTreePath *path;

  path = bobgui_tree_model_get_path (tree_model, iter);
  if (path == NULL)
    return FALSE;

  retval = bobgui_tree_path_prev (path) &&
           bobgui_tree_model_get_iter (tree_model, iter, path);
  if (retval == FALSE)
    iter->stamp = 0;

  bobgui_tree_path_free (path);

  return retval;
}

/**
 * bobgui_tree_model_iter_previous:
 * @tree_model: a `BobguiTreeModel`
 * @iter: (in): the `BobguiTreeIter`
 *
 * Sets @iter to point to the previous node at the current level.
 *
 * If there is no previous @iter, %FALSE is returned and @iter is
 * set to be invalid.
 *
 * Returns: %TRUE if @iter has been changed to the previous node
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_model_iter_previous (BobguiTreeModel *tree_model,
                              BobguiTreeIter  *iter)
{
  gboolean retval;
  BobguiTreeModelIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);

  if (iface->iter_previous)
    retval = (* iface->iter_previous) (tree_model, iter);
  else
    retval = bobgui_tree_model_iter_previous_default (tree_model, iter);

  return retval;
}

/**
 * bobgui_tree_model_iter_children:
 * @tree_model: a `BobguiTreeModel`
 * @iter: (out): the new `BobguiTreeIter` to be set to the child
 * @parent: (nullable): the `BobguiTreeIter`
 *
 * Sets @iter to point to the first child of @parent.
 *
 * If @parent has no children, %FALSE is returned and @iter is
 * set to be invalid. @parent will remain a valid node after this
 * function has been called.
 *
 * If @parent is %NULL returns the first node, equivalent to
 * `bobgui_tree_model_get_iter_first (tree_model, iter);`
 *
 * Returns: %TRUE, if @iter has been set to the first child
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_model_iter_children (BobguiTreeModel *tree_model,
                              BobguiTreeIter  *iter,
                              BobguiTreeIter  *parent)
{
  BobguiTreeModelIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  g_return_val_if_fail (iface->iter_children != NULL, FALSE);

  INITIALIZE_TREE_ITER (iter);

  return (* iface->iter_children) (tree_model, iter, parent);
}

/**
 * bobgui_tree_model_iter_has_child:
 * @tree_model: a `BobguiTreeModel`
 * @iter: the `BobguiTreeIter` to test for children
 *
 * Returns %TRUE if @iter has children, %FALSE otherwise.
 *
 * Returns: %TRUE if @iter has children
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_model_iter_has_child (BobguiTreeModel *tree_model,
                               BobguiTreeIter  *iter)
{
  BobguiTreeModelIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  g_return_val_if_fail (iface->iter_has_child != NULL, FALSE);

  return (* iface->iter_has_child) (tree_model, iter);
}

/**
 * bobgui_tree_model_iter_n_children:
 * @tree_model: a `BobguiTreeModel`
 * @iter: (nullable): the `BobguiTreeIter`
 *
 * Returns the number of children that @iter has.
 *
 * As a special case, if @iter is %NULL, then the number
 * of toplevel nodes is returned.
 *
 * Returns: the number of children of @iter
 *
 * Deprecated: 4.10
 */
int
bobgui_tree_model_iter_n_children (BobguiTreeModel *tree_model,
                                BobguiTreeIter  *iter)
{
  BobguiTreeModelIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), 0);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  g_return_val_if_fail (iface->iter_n_children != NULL, 0);

  return (* iface->iter_n_children) (tree_model, iter);
}

/**
 * bobgui_tree_model_iter_nth_child:
 * @tree_model: a `BobguiTreeModel`
 * @iter: (out): the `BobguiTreeIter` to set to the nth child
 * @parent: (nullable): the `BobguiTreeIter` to get the child from
 * @n: the index of the desired child
 *
 * Sets @iter to be the child of @parent, using the given index.
 *
 * The first index is 0. If @n is too big, or @parent has no children,
 * @iter is set to an invalid iterator and %FALSE is returned. @parent
 * will remain a valid node after this function has been called. As a
 * special case, if @parent is %NULL, then the @n-th root node
 * is set.
 *
 * Returns: %TRUE, if @parent has an @n-th child
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_model_iter_nth_child (BobguiTreeModel *tree_model,
                               BobguiTreeIter  *iter,
                               BobguiTreeIter  *parent,
                               int           n)
{
  BobguiTreeModelIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (n >= 0, FALSE);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  g_return_val_if_fail (iface->iter_nth_child != NULL, FALSE);

  INITIALIZE_TREE_ITER (iter);

  return (* iface->iter_nth_child) (tree_model, iter, parent, n);
}

/**
 * bobgui_tree_model_iter_parent:
 * @tree_model: a `BobguiTreeModel`
 * @iter: (out): the new `BobguiTreeIter` to set to the parent
 * @child: the `BobguiTreeIter`
 *
 * Sets @iter to be the parent of @child.
 *
 * If @child is at the toplevel, and doesn’t have a parent, then
 * @iter is set to an invalid iterator and %FALSE is returned.
 * @child will remain a valid node after this function has been
 * called.
 *
 * @iter will be initialized before the lookup is performed, so @child
 * and @iter cannot point to the same memory location.
 *
 * Returns: %TRUE, if @iter is set to the parent of @child
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_model_iter_parent (BobguiTreeModel *tree_model,
                            BobguiTreeIter  *iter,
                            BobguiTreeIter  *child)
{
  BobguiTreeModelIface *iface;

  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (tree_model), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (child != NULL, FALSE);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  g_return_val_if_fail (iface->iter_parent != NULL, FALSE);

  INITIALIZE_TREE_ITER (iter);

  return (* iface->iter_parent) (tree_model, iter, child);
}

/**
 * bobgui_tree_model_ref_node:
 * @tree_model: a `BobguiTreeModel`
 * @iter: the `BobguiTreeIter`
 *
 * Lets the tree ref the node.
 *
 * This is an optional method for models to implement.
 * To be more specific, models may ignore this call as it exists
 * primarily for performance reasons.
 *
 * This function is primarily meant as a way for views to let
 * caching models know when nodes are being displayed (and hence,
 * whether or not to cache that node). Being displayed means a node
 * is in an expanded branch, regardless of whether the node is currently
 * visible in the viewport. For example, a file-system based model
 * would not want to keep the entire file-hierarchy in memory,
 * just the sections that are currently being displayed by
 * every current view.
 *
 * A model should be expected to be able to get an iter independent
 * of its reffed state.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_model_ref_node (BobguiTreeModel *tree_model,
                         BobguiTreeIter  *iter)
{
  BobguiTreeModelIface *iface;

  g_return_if_fail (BOBGUI_IS_TREE_MODEL (tree_model));

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  if (iface->ref_node)
    (* iface->ref_node) (tree_model, iter);
}

/**
 * bobgui_tree_model_unref_node:
 * @tree_model: a `BobguiTreeModel`
 * @iter: the `BobguiTreeIter`
 *
 * Lets the tree unref the node.
 *
 * This is an optional method for models to implement.
 * To be more specific, models may ignore this call as it exists
 * primarily for performance reasons. For more information on what
 * this means, see bobgui_tree_model_ref_node().
 *
 * Please note that nodes that are deleted are not unreffed.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_model_unref_node (BobguiTreeModel *tree_model,
                           BobguiTreeIter  *iter)
{
  BobguiTreeModelIface *iface;

  g_return_if_fail (BOBGUI_IS_TREE_MODEL (tree_model));
  g_return_if_fail (iter != NULL);

  iface = BOBGUI_TREE_MODEL_GET_IFACE (tree_model);
  if (iface->unref_node)
    (* iface->unref_node) (tree_model, iter);
}

/**
 * bobgui_tree_model_get:
 * @tree_model: a `BobguiTreeModel`
 * @iter: a row in @tree_model
 * @...: pairs of column number and value return locations,
 *   terminated by -1
 *
 * Gets the value of one or more cells in the row referenced by @iter.
 *
 * The variable argument list should contain integer column numbers,
 * each column number followed by a place to store the value being
 * retrieved.  The list is terminated by a -1. For example, to get a
 * value from column 0 with type %G_TYPE_STRING, you would
 * write: `bobgui_tree_model_get (model, iter, 0, &place_string_here, -1)`,
 * where `place_string_here` is a #gchararray
 * to be filled with the string.
 *
 * Returned values with type %G_TYPE_OBJECT have to be unreferenced,
 * values with type %G_TYPE_STRING or %G_TYPE_BOXED have to be freed.
 * Other values are passed by value.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_model_get (BobguiTreeModel *tree_model,
                    BobguiTreeIter  *iter,
                    ...)
{
  va_list var_args;

  g_return_if_fail (BOBGUI_IS_TREE_MODEL (tree_model));
  g_return_if_fail (iter != NULL);

  va_start (var_args, iter);
  bobgui_tree_model_get_valist (tree_model, iter, var_args);
  va_end (var_args);
}

/**
 * bobgui_tree_model_get_valist:
 * @tree_model: a `BobguiTreeModel`
 * @iter: a row in @tree_model
 * @var_args: va_list of column/return location pairs
 *
 * Gets the value of one or more cells in the row referenced by @iter.
 *
 * See [method@Bobgui.TreeModel.get], this version takes a va_list
 * for language bindings to use.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_model_get_valist (BobguiTreeModel *tree_model,
                           BobguiTreeIter  *iter,
                           va_list      var_args)
{
  int column;

  g_return_if_fail (BOBGUI_IS_TREE_MODEL (tree_model));
  g_return_if_fail (iter != NULL);

  column = va_arg (var_args, int);

  while (column != -1)
    {
      GValue value = G_VALUE_INIT;
      char *error = NULL;

      if (column >= bobgui_tree_model_get_n_columns (tree_model))
        {
          g_warning ("%s: Invalid column number %d accessed (remember to end your list of columns with a -1)", G_STRLOC, column);
          break;
        }

      bobgui_tree_model_get_value (BOBGUI_TREE_MODEL (tree_model), iter, column, &value);

      G_VALUE_LCOPY (&value, var_args, 0, &error);
      if (error)
        {
          g_warning ("%s: %s", G_STRLOC, error);
          g_free (error);

          /* we purposely leak the value here, it might not be
           * in a sane state if an error condition occurred
           */
          break;
        }

      g_value_unset (&value);

      column = va_arg (var_args, int);
    }
}

/**
 * bobgui_tree_model_row_changed:
 * @tree_model: a `BobguiTreeModel`
 * @path: a `BobguiTreePath` pointing to the changed row
 * @iter: a valid `BobguiTreeIter` pointing to the changed row
 *
 * Emits the ::row-changed signal on @tree_model.
 *
 * See [signal@Bobgui.TreeModel::row-changed].
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_model_row_changed (BobguiTreeModel *tree_model,
                            BobguiTreePath  *path,
                            BobguiTreeIter  *iter)
{
  g_return_if_fail (BOBGUI_IS_TREE_MODEL (tree_model));
  g_return_if_fail (path != NULL);
  g_return_if_fail (iter != NULL);

  g_signal_emit (tree_model, tree_model_signals[ROW_CHANGED], 0, path, iter);
}

/**
 * bobgui_tree_model_row_inserted:
 * @tree_model: a `BobguiTreeModel`
 * @path: a `BobguiTreePath` pointing to the inserted row
 * @iter: a valid `BobguiTreeIter` pointing to the inserted row
 *
 * Emits the ::row-inserted signal on @tree_model.
 *
 * See [signal@Bobgui.TreeModel::row-inserted].
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_model_row_inserted (BobguiTreeModel *tree_model,
                             BobguiTreePath  *path,
                             BobguiTreeIter  *iter)
{
  g_return_if_fail (BOBGUI_IS_TREE_MODEL (tree_model));
  g_return_if_fail (path != NULL);
  g_return_if_fail (iter != NULL);

  g_signal_emit (tree_model, tree_model_signals[ROW_INSERTED], 0, path, iter);
}

/**
 * bobgui_tree_model_row_has_child_toggled:
 * @tree_model: a `BobguiTreeModel`
 * @path: a `BobguiTreePath` pointing to the changed row
 * @iter: a valid `BobguiTreeIter` pointing to the changed row
 *
 * Emits the ::row-has-child-toggled signal on @tree_model.
 *
 * See [signal@Bobgui.TreeModel::row-has-child-toggled].
 *
 * This should be called by models after the child
 * state of a node changes.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_model_row_has_child_toggled (BobguiTreeModel *tree_model,
                                      BobguiTreePath  *path,
                                      BobguiTreeIter  *iter)
{
  g_return_if_fail (BOBGUI_IS_TREE_MODEL (tree_model));
  g_return_if_fail (path != NULL);
  g_return_if_fail (iter != NULL);

  g_signal_emit (tree_model, tree_model_signals[ROW_HAS_CHILD_TOGGLED], 0, path, iter);
}

/**
 * bobgui_tree_model_row_deleted:
 * @tree_model: a `BobguiTreeModel`
 * @path: a `BobguiTreePath` pointing to the previous location of
 *   the deleted row
 *
 * Emits the ::row-deleted signal on @tree_model.
 *
 * See [signal@Bobgui.TreeModel::row-deleted].
 *
 * This should be called by models after a row has been removed.
 * The location pointed to by @path should be the location that
 * the row previously was at. It may not be a valid location anymore.
 *
 * Nodes that are deleted are not unreffed, this means that any
 * outstanding references on the deleted node should not be released.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_model_row_deleted (BobguiTreeModel *tree_model,
                            BobguiTreePath  *path)
{
  g_return_if_fail (BOBGUI_IS_TREE_MODEL (tree_model));
  g_return_if_fail (path != NULL);

  g_signal_emit (tree_model, tree_model_signals[ROW_DELETED], 0, path);
}

/**
 * bobgui_tree_model_rows_reordered: (skip)
 * @tree_model: a `BobguiTreeModel`
 * @path: a `BobguiTreePath` pointing to the tree node whose children
 *   have been reordered
 * @iter: a valid `BobguiTreeIter` pointing to the node whose children
 *   have been reordered, or %NULL if the depth of @path is 0
 * @new_order: an array of integers mapping the current position of
 *   each child to its old position before the re-ordering,
 *   i.e. @new_order`[newpos] = oldpos`
 *
 * Emits the ::rows-reordered signal on @tree_model.
 *
 * See [signal@Bobgui.TreeModel::rows-reordered].
 *
 * This should be called by models when their rows have been
 * reordered.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_model_rows_reordered (BobguiTreeModel *tree_model,
                               BobguiTreePath  *path,
                               BobguiTreeIter  *iter,
                               int          *new_order)
{
  g_return_if_fail (BOBGUI_IS_TREE_MODEL (tree_model));
  g_return_if_fail (new_order != NULL);

  g_signal_emit (tree_model, tree_model_signals[ROWS_REORDERED], 0, path, iter, new_order);
}

/**
 * bobgui_tree_model_rows_reordered_with_length: (rename-to bobgui_tree_model_rows_reordered)
 * @tree_model: a `BobguiTreeModel`
 * @path: a `BobguiTreePath` pointing to the tree node whose children
 *   have been reordered
 * @iter: (nullable): a valid `BobguiTreeIter` pointing to the node
 *   whose children have been reordered, or %NULL if the depth
 *   of @path is 0
 * @new_order: (array length=length): an array of integers
 *   mapping the current position of each child to its old
 *   position before the re-ordering,
 *   i.e. @new_order`[newpos] = oldpos`
 * @length: length of @new_order array
 *
 * Emits the ::rows-reordered signal on @tree_model.
 *
 * See [signal@Bobgui.TreeModel::rows-reordered].
 *
 * This should be called by models when their rows have been
 * reordered.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_model_rows_reordered_with_length (BobguiTreeModel *tree_model,
                                           BobguiTreePath  *path,
                                           BobguiTreeIter  *iter,
                                           int          *new_order,
                                           int           length)
{
  g_return_if_fail (BOBGUI_IS_TREE_MODEL (tree_model));
  g_return_if_fail (new_order != NULL);
  g_return_if_fail (length == bobgui_tree_model_iter_n_children (tree_model, iter));

  g_signal_emit (tree_model, tree_model_signals[ROWS_REORDERED], 0, path, iter, new_order);
}

static gboolean
bobgui_tree_model_foreach_helper (BobguiTreeModel            *model,
                               BobguiTreeIter             *iter,
                               BobguiTreePath             *path,
                               BobguiTreeModelForeachFunc  func,
                               gpointer                 user_data)
{
  gboolean iters_persist;

  iters_persist = bobgui_tree_model_get_flags (model) & BOBGUI_TREE_MODEL_ITERS_PERSIST;

  do
    {
      BobguiTreeIter child;

      if ((* func) (model, path, iter, user_data))
        return TRUE;

      if (!iters_persist)
        {
          if (!bobgui_tree_model_get_iter (model, iter, path))
            return TRUE;
        }

      if (bobgui_tree_model_iter_children (model, &child, iter))
        {
          bobgui_tree_path_down (path);
          if (bobgui_tree_model_foreach_helper (model, &child, path, func, user_data))
            return TRUE;
          bobgui_tree_path_up (path);
        }

      bobgui_tree_path_next (path);
    }
  while (bobgui_tree_model_iter_next (model, iter));

  return FALSE;
}

/**
 * bobgui_tree_model_foreach:
 * @model: a `BobguiTreeModel`
 * @func: (scope call) (closure user_data): a function to be called on each row
 * @user_data: user data to passed to @func
 *
 * Calls @func on each node in model in a depth-first fashion.
 *
 * If @func returns %TRUE, then the tree ceases to be walked,
 * and bobgui_tree_model_foreach() returns.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_model_foreach (BobguiTreeModel            *model,
                        BobguiTreeModelForeachFunc  func,
                        gpointer                 user_data)
{
  BobguiTreePath *path;
  BobguiTreeIter iter;

  g_return_if_fail (BOBGUI_IS_TREE_MODEL (model));
  g_return_if_fail (func != NULL);

  path = bobgui_tree_path_new_first ();
  if (!bobgui_tree_model_get_iter (model, &iter, path))
    {
      bobgui_tree_path_free (path);
      return;
    }

  bobgui_tree_model_foreach_helper (model, &iter, path, func, user_data);
  bobgui_tree_path_free (path);
}


/*
 * BobguiTreeRowReference
 */

static void bobgui_tree_row_reference_unref_path (BobguiTreePath  *path,
                                               BobguiTreeModel *model,
                                               int           depth);


G_DEFINE_BOXED_TYPE (BobguiTreeRowReference, bobgui_tree_row_reference,
                     bobgui_tree_row_reference_copy,
                     bobgui_tree_row_reference_free)

struct _BobguiTreeRowReference
{
  GObject *proxy;
  BobguiTreeModel *model;
  BobguiTreePath *path;
};


static void
release_row_references (gpointer data)
{
  RowRefList *refs = data;
  GSList *tmp_list = NULL;

  tmp_list = refs->list;
  while (tmp_list != NULL)
    {
      BobguiTreeRowReference *reference = tmp_list->data;

      if (reference->proxy == (GObject *)reference->model)
        reference->model = NULL;
      reference->proxy = NULL;

      /* we don't free the reference, users are responsible for that. */

      tmp_list = tmp_list->next;
    }

  g_slist_free (refs->list);
  g_free (refs);
}

static void
bobgui_tree_row_ref_inserted (RowRefList  *refs,
                           BobguiTreePath *path,
                           BobguiTreeIter *iter)
{
  GSList *tmp_list;

  if (refs == NULL)
    return;

  /* This function corrects the path stored in the reference to
   * account for an insertion. Note that it's called _after_ the
   * insertion with the path to the newly-inserted row. Which means
   * that the inserted path is in a different "coordinate system" than
   * the old path (e.g. if the inserted path was just before the old
   * path, then inserted path and old path will be the same, and old
   * path must be moved down one).
   */

  tmp_list = refs->list;

  while (tmp_list != NULL)
    {
      BobguiTreeRowReference *reference = tmp_list->data;

      if (reference->path == NULL)
        goto done;

      if (reference->path->depth >= path->depth)
        {
          int i;
          gboolean ancestor = TRUE;

          for (i = 0; i < path->depth - 1; i ++)
            {
              if (path->indices[i] != reference->path->indices[i])
                {
                  ancestor = FALSE;
                  break;
                }
            }
          if (ancestor == FALSE)
            goto done;

          if (path->indices[path->depth-1] <= reference->path->indices[path->depth-1])
            reference->path->indices[path->depth-1] += 1;
        }
    done:
      tmp_list = tmp_list->next;
    }
}

static void
bobgui_tree_row_ref_deleted (RowRefList  *refs,
                          BobguiTreePath *path)
{
  GSList *tmp_list;

  if (refs == NULL)
    return;

  /* This function corrects the path stored in the reference to
   * account for a deletion. Note that it's called _after_ the
   * deletion with the old path of the just-deleted row. Which means
   * that the deleted path is the same now-defunct "coordinate system"
   * as the path saved in the reference, which is what we want to fix.
   */

  tmp_list = refs->list;

  while (tmp_list != NULL)
    {
      BobguiTreeRowReference *reference = tmp_list->data;

      if (reference->path)
        {
          int i;

          if (path->depth > reference->path->depth)
            goto next;
          for (i = 0; i < path->depth - 1; i++)
            {
              if (path->indices[i] != reference->path->indices[i])
                goto next;
            }

          /* We know it affects us. */
          if (path->indices[i] == reference->path->indices[i])
            {
              if (reference->path->depth > path->depth)
                /* some parent was deleted, trying to unref any node
                 * between the deleted parent and the node the reference
                 * is pointing to is bad, as those nodes are already gone.
                 */
                bobgui_tree_row_reference_unref_path (reference->path, reference->model, path->depth - 1);
              else
                bobgui_tree_row_reference_unref_path (reference->path, reference->model, reference->path->depth - 1);
              bobgui_tree_path_free (reference->path);
              reference->path = NULL;
            }
          else if (path->indices[i] < reference->path->indices[i])
            {
              reference->path->indices[path->depth-1]-=1;
            }
        }

next:
      tmp_list = tmp_list->next;
    }
}

static void
bobgui_tree_row_ref_reordered (RowRefList  *refs,
                            BobguiTreePath *path,
                            BobguiTreeIter *iter,
                            int         *new_order)
{
  GSList *tmp_list;
  int length;

  if (refs == NULL)
    return;

  tmp_list = refs->list;

  while (tmp_list != NULL)
    {
      BobguiTreeRowReference *reference = tmp_list->data;

      length = bobgui_tree_model_iter_n_children (BOBGUI_TREE_MODEL (reference->model), iter);

      if (length < 2)
        return;

      if ((reference->path) &&
          (bobgui_tree_path_is_ancestor (path, reference->path)))
        {
          int ref_depth = bobgui_tree_path_get_depth (reference->path);
          int depth = bobgui_tree_path_get_depth (path);

          if (ref_depth > depth)
            {
              int i;
              int *indices = bobgui_tree_path_get_indices (reference->path);

              for (i = 0; i < length; i++)
                {
                  if (new_order[i] == indices[depth])
                    {
                      indices[depth] = i;
                      break;
                    }
                }
            }
        }

      tmp_list = tmp_list->next;
    }
}

/* We do this recursively so that we can unref children nodes
 * before their parent
 */
static void
bobgui_tree_row_reference_unref_path_helper (BobguiTreePath  *path,
                                          BobguiTreeModel *model,
                                          BobguiTreeIter  *parent_iter,
                                          int           depth,
                                          int           current_depth)
{
  BobguiTreeIter iter;

  if (depth == current_depth)
    return;

  bobgui_tree_model_iter_nth_child (model, &iter, parent_iter, path->indices[current_depth]);
  bobgui_tree_row_reference_unref_path_helper (path, model, &iter, depth, current_depth + 1);
  bobgui_tree_model_unref_node (model, &iter);
}

static void
bobgui_tree_row_reference_unref_path (BobguiTreePath  *path,
                                   BobguiTreeModel *model,
                                   int           depth)
{
  BobguiTreeIter iter;

  if (depth <= 0)
    return;

  bobgui_tree_model_iter_nth_child (model, &iter, NULL, path->indices[0]);
  bobgui_tree_row_reference_unref_path_helper (path, model, &iter, depth, 1);
  bobgui_tree_model_unref_node (model, &iter);
}

/**
 * bobgui_tree_row_reference_new:
 * @model: a `BobguiTreeModel`
 * @path: a valid `BobguiTreePath` to monitor
 *
 * Creates a row reference based on @path.
 *
 * This reference will keep pointing to the node pointed to
 * by @path, so long as it exists. Any changes that occur on @model are
 * propagated, and the path is updated appropriately. If
 * @path isn’t a valid path in @model, then %NULL is returned.
 *
 * Returns: (nullable): a newly allocated `BobguiTreeRowReference`
 *
 * Deprecated: 4.10
 */
BobguiTreeRowReference *
bobgui_tree_row_reference_new (BobguiTreeModel *model,
                            BobguiTreePath  *path)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (model), NULL);
  g_return_val_if_fail (path != NULL, NULL);

  /* We use the model itself as the proxy object; and call
   * bobgui_tree_row_reference_inserted(), etc, in the
   * class closure (default handler) marshalers for the signal.
   */
  return bobgui_tree_row_reference_new_proxy (G_OBJECT (model), model, path);
}

/**
 * bobgui_tree_row_reference_new_proxy:
 * @proxy: a proxy `GObject`
 * @model: a `BobguiTreeModel`
 * @path: a valid `BobguiTreePath` to monitor
 *
 * You do not need to use this function.
 *
 * Creates a row reference based on @path.
 *
 * This reference will keep pointing to the node pointed to
 * by @path, so long as it exists. If @path isn’t a valid
 * path in @model, then %NULL is returned. However, unlike
 * references created with bobgui_tree_row_reference_new(), it
 * does not listen to the model for changes. The creator of
 * the row reference must do this explicitly using
 * bobgui_tree_row_reference_inserted(), bobgui_tree_row_reference_deleted(),
 * bobgui_tree_row_reference_reordered().
 *
 * These functions must be called exactly once per proxy when the
 * corresponding signal on the model is emitted. This single call
 * updates all row references for that proxy. Since built-in BOBGUI
 * objects like `BobguiTreeView` already use this mechanism internally,
 * using them as the proxy object will produce unpredictable results.
 * Further more, passing the same object as @model and @proxy
 * doesn’t work for reasons of internal implementation.
 *
 * This type of row reference is primarily meant by structures that
 * need to carefully monitor exactly when a row reference updates
 * itself, and is not generally needed by most applications.
 *
 * Returns: (nullable): a newly allocated `BobguiTreeRowReference`
 *
 * Deprecated: 4.10
 */
BobguiTreeRowReference *
bobgui_tree_row_reference_new_proxy (GObject      *proxy,
                                  BobguiTreeModel *model,
                                  BobguiTreePath  *path)
{
  BobguiTreeRowReference *reference;
  RowRefList *refs;
  BobguiTreeIter parent_iter;
  int i;

  g_return_val_if_fail (G_IS_OBJECT (proxy), NULL);
  g_return_val_if_fail (BOBGUI_IS_TREE_MODEL (model), NULL);
  g_return_val_if_fail (path != NULL, NULL);
  g_return_val_if_fail (path->depth > 0, NULL);

  /* check that the path is valid */
  if (bobgui_tree_model_get_iter (model, &parent_iter, path) == FALSE)
    return NULL;

  /* Now we want to ref every node */
  bobgui_tree_model_iter_nth_child (model, &parent_iter, NULL, path->indices[0]);
  bobgui_tree_model_ref_node (model, &parent_iter);

  for (i = 1; i < path->depth; i++)
    {
      BobguiTreeIter iter;
      bobgui_tree_model_iter_nth_child (model, &iter, &parent_iter, path->indices[i]);
      bobgui_tree_model_ref_node (model, &iter);
      parent_iter = iter;
    }

  /* Make the row reference */
  reference = g_new (BobguiTreeRowReference, 1);

  g_object_ref (proxy);
  g_object_ref (model);
  reference->proxy = proxy;
  reference->model = model;
  reference->path = bobgui_tree_path_copy (path);

  refs = g_object_get_data (G_OBJECT (proxy), ROW_REF_DATA_STRING);

  if (refs == NULL)
    {
      refs = g_new (RowRefList, 1);
      refs->list = NULL;

      g_object_set_data_full (G_OBJECT (proxy),
                              I_(ROW_REF_DATA_STRING),
                              refs, release_row_references);
    }

  refs->list = g_slist_prepend (refs->list, reference);

  return reference;
}

/**
 * bobgui_tree_row_reference_get_path:
 * @reference: a `BobguiTreeRowReference`
 *
 * Returns a path that the row reference currently points to,
 * or %NULL if the path pointed to is no longer valid.
 *
 * Returns: (nullable) (transfer full): a current path
 *
 * Deprecated: 4.10
 */
BobguiTreePath *
bobgui_tree_row_reference_get_path (BobguiTreeRowReference *reference)
{
  g_return_val_if_fail (reference != NULL, NULL);

  if (reference->proxy == NULL)
    return NULL;

  if (reference->path == NULL)
    return NULL;

  return bobgui_tree_path_copy (reference->path);
}

/**
 * bobgui_tree_row_reference_get_model:
 * @reference: a `BobguiTreeRowReference`
 *
 * Returns the model that the row reference is monitoring.
 *
 * Returns: (transfer none): the model
 *
 * Deprecated: 4.10
 */
BobguiTreeModel *
bobgui_tree_row_reference_get_model (BobguiTreeRowReference *reference)
{
  g_return_val_if_fail (reference != NULL, NULL);

  return reference->model;
}

/**
 * bobgui_tree_row_reference_valid:
 * @reference: (nullable): a `BobguiTreeRowReference`
 *
 * Returns %TRUE if the @reference is non-%NULL and refers to
 * a current valid path.
 *
 * Returns: %TRUE if @reference points to a valid path
 *
 * Deprecated: 4.10
 */
gboolean
bobgui_tree_row_reference_valid (BobguiTreeRowReference *reference)
{
  if (reference == NULL || reference->path == NULL)
    return FALSE;

  return TRUE;
}


/**
 * bobgui_tree_row_reference_copy:
 * @reference: a `BobguiTreeRowReference`
 *
 * Copies a `BobguiTreeRowReference`.
 *
 * Returns: a copy of @reference
 *
 * Deprecated: 4.10
 */
BobguiTreeRowReference *
bobgui_tree_row_reference_copy (BobguiTreeRowReference *reference)
{
  return bobgui_tree_row_reference_new_proxy (reference->proxy,
                                           reference->model,
                                           reference->path);
}

/**
 * bobgui_tree_row_reference_free:
 * @reference: (nullable): a `BobguiTreeRowReference`
 *
 * Free’s @reference. @reference may be %NULL
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_row_reference_free (BobguiTreeRowReference *reference)
{
  RowRefList *refs;

  if (reference == NULL)
    return;

  refs = g_object_get_data (G_OBJECT (reference->proxy), ROW_REF_DATA_STRING);

  if (refs == NULL)
    {
      g_warning (G_STRLOC": bad row reference, proxy has no outstanding row references");
      return;
    }

  refs->list = g_slist_remove (refs->list, reference);

  if (refs->list == NULL)
    {
      g_object_set_data (G_OBJECT (reference->proxy),
                         I_(ROW_REF_DATA_STRING),
                         NULL);
    }

  if (reference->path)
    {
      bobgui_tree_row_reference_unref_path (reference->path, reference->model, reference->path->depth);
      bobgui_tree_path_free (reference->path);
    }

  g_object_unref (reference->proxy);
  g_object_unref (reference->model);
  g_free (reference);
}

/**
 * bobgui_tree_row_reference_inserted:
 * @proxy: a `GObject`
 * @path: the row position that was inserted
 *
 * Lets a set of row reference created by
 * bobgui_tree_row_reference_new_proxy() know that the
 * model emitted the ::row-inserted signal.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_row_reference_inserted (GObject     *proxy,
                                 BobguiTreePath *path)
{
  g_return_if_fail (G_IS_OBJECT (proxy));

  bobgui_tree_row_ref_inserted ((RowRefList *)g_object_get_data (proxy, ROW_REF_DATA_STRING), path, NULL);
}

/**
 * bobgui_tree_row_reference_deleted:
 * @proxy: a `GObject`
 * @path: the path position that was deleted
 *
 * Lets a set of row reference created by
 * bobgui_tree_row_reference_new_proxy() know that the
 * model emitted the ::row-deleted signal.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_row_reference_deleted (GObject     *proxy,
                                BobguiTreePath *path)
{
  g_return_if_fail (G_IS_OBJECT (proxy));

  bobgui_tree_row_ref_deleted ((RowRefList *)g_object_get_data (proxy, ROW_REF_DATA_STRING), path);
}

/**
 * bobgui_tree_row_reference_reordered: (skip)
 * @proxy: a `GObject`
 * @path: the parent path of the reordered signal
 * @iter: the iter pointing to the parent of the reordered
 * @new_order: (array): the new order of rows
 *
 * Lets a set of row reference created by
 * bobgui_tree_row_reference_new_proxy() know that the
 * model emitted the ::rows-reordered signal.
 *
 * Deprecated: 4.10
 */
void
bobgui_tree_row_reference_reordered (GObject     *proxy,
                                  BobguiTreePath *path,
                                  BobguiTreeIter *iter,
                                  int         *new_order)
{
  g_return_if_fail (G_IS_OBJECT (proxy));

  bobgui_tree_row_ref_reordered ((RowRefList *)g_object_get_data (proxy, ROW_REF_DATA_STRING), path, iter, new_order);
}
