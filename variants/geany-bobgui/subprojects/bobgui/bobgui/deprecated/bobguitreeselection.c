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

#include "config.h"
#include <string.h>
#include "bobguitreeselection.h"
#include "bobguitreeprivate.h"
#include "bobguitreerbtreeprivate.h"
#include "bobguimarshalers.h"
#include "bobguiprivate.h"
#include "bobguitypebuiltins.h"


G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/**
 * BobguiTreeSelection:
 *
 * The selection object for BobguiTreeView
 *
 * The `BobguiTreeSelection` object is a helper object to manage the selection
 * for a `BobguiTreeView` widget.  The `BobguiTreeSelection` object is
 * automatically created when a new `BobguiTreeView` widget is created, and
 * cannot exist independently of this widget.  The primary reason the
 * `BobguiTreeSelection` objects exists is for cleanliness of code and API.
 * That is, there is no conceptual reason all these functions could not be
 * methods on the `BobguiTreeView` widget instead of a separate function.
 *
 * The `BobguiTreeSelection` object is gotten from a `BobguiTreeView` by calling
 * bobgui_tree_view_get_selection().  It can be manipulated to check the
 * selection status of the tree, as well as select and deselect individual
 * rows.  Selection is done completely view side.  As a result, multiple
 * views of the same model can have completely different selections.
 * Additionally, you cannot change the selection of a row on the model that
 * is not currently displayed by the view without expanding its parents
 * first.
 *
 * One of the important things to remember when monitoring the selection of
 * a view is that the `BobguiTreeSelection`::changed signal is mostly a hint.
 * That is, it may only emit one signal when a range of rows is selected.
 * Additionally, it may on occasion emit a `BobguiTreeSelection`::changed signal
 * when nothing has happened (mostly as a result of programmers calling
 * select_row on an already selected row).
 *
 * Deprecated: 4.10: Use [iface@Bobgui.SelectionModel] instead
 */

typedef struct _BobguiTreeSelectionClass   BobguiTreeSelectionClass;

struct _BobguiTreeSelection
{
  GObject parent;

  BobguiTreeView *tree_view;
  BobguiSelectionMode type;
  BobguiTreeSelectionFunc user_func;
  gpointer user_data;
  GDestroyNotify destroy;
};

struct _BobguiTreeSelectionClass
{
  GObjectClass parent_class;

  void (* changed) (BobguiTreeSelection *selection);
};

static void bobgui_tree_selection_finalize          (GObject               *object);
static int  bobgui_tree_selection_real_select_all   (BobguiTreeSelection      *selection);
static int  bobgui_tree_selection_real_unselect_all (BobguiTreeSelection      *selection);
static int  bobgui_tree_selection_real_select_node  (BobguiTreeSelection      *selection,
						  BobguiTreeRBTree         *tree,
						  BobguiTreeRBNode         *node,
						  gboolean               select);
static void bobgui_tree_selection_set_property      (GObject               *object,
                                                  guint                  prop_id,
                                                  const GValue          *value,
                                                  GParamSpec            *pspec);
static void bobgui_tree_selection_get_property      (GObject               *object,
                                                  guint                  prop_id,
                                                  GValue                *value,
                                                  GParamSpec            *pspec);

enum
{
  PROP_0,
  PROP_MODE,
  N_PROPERTIES
};

enum
{
  CHANGED,
  LAST_SIGNAL
};

static GParamSpec *properties[N_PROPERTIES];
static guint tree_selection_signals [LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE(BobguiTreeSelection, bobgui_tree_selection, G_TYPE_OBJECT)

static void
bobgui_tree_selection_class_init (BobguiTreeSelectionClass *class)
{
  GObjectClass *object_class;

  object_class = (GObjectClass*) class;

  object_class->finalize = bobgui_tree_selection_finalize;
  object_class->set_property = bobgui_tree_selection_set_property;
  object_class->get_property = bobgui_tree_selection_get_property;
  class->changed = NULL;

  /* Properties */

  /**
   * BobguiTreeSelection:mode:
   *
   * Selection mode.
   * See bobgui_tree_selection_set_mode() for more information on this property.
   */
  properties[PROP_MODE] = g_param_spec_enum ("mode", NULL, NULL,
                                             BOBGUI_TYPE_SELECTION_MODE,
                                             BOBGUI_SELECTION_SINGLE,
                                             BOBGUI_PARAM_READWRITE|G_PARAM_EXPLICIT_NOTIFY);

  /* Install all properties */
  g_object_class_install_properties (object_class, N_PROPERTIES, properties);

  /* Signals */

  /**
   * BobguiTreeSelection::changed:
   * @treeselection: the object which received the signal.
   *
   * Emitted whenever the selection has (possibly) changed. Please note that
   * this signal is mostly a hint.  It may only be emitted once when a range
   * of rows are selected, and it may occasionally be emitted when nothing
   * has happened.
   */
  tree_selection_signals[CHANGED] =
    g_signal_new (I_("changed"),
		  G_OBJECT_CLASS_TYPE (object_class),
		  G_SIGNAL_RUN_FIRST,
		  G_STRUCT_OFFSET (BobguiTreeSelectionClass, changed),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
}

static void
bobgui_tree_selection_init (BobguiTreeSelection *selection)
{
  selection->type = BOBGUI_SELECTION_SINGLE;
}

static void
bobgui_tree_selection_finalize (GObject *object)
{
  BobguiTreeSelection *selection = BOBGUI_TREE_SELECTION (object);

  if (selection->destroy)
    selection->destroy (selection->user_data);

  /* chain parent_class' handler */
  G_OBJECT_CLASS (bobgui_tree_selection_parent_class)->finalize (object);
}

static void
bobgui_tree_selection_set_property (GObject *object,
                                 guint prop_id,
                                 const GValue *value,
                                 GParamSpec *pspec)
{
  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (object));

  switch (prop_id)
    {
      case PROP_MODE:
        bobgui_tree_selection_set_mode (BOBGUI_TREE_SELECTION (object), g_value_get_enum (value));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

static void
bobgui_tree_selection_get_property (GObject *object,
                                 guint prop_id,
                                 GValue *value,
                                 GParamSpec *pspec)
{
  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (object));

  switch (prop_id)
    {
      case PROP_MODE:
        g_value_set_enum (value, bobgui_tree_selection_get_mode (BOBGUI_TREE_SELECTION (object)));
        break;
      default:
        G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
        break;
    }
}

/**
 * _bobgui_tree_selection_new:
 *
 * Creates a new `BobguiTreeSelection` object.  This function should not be invoked,
 * as each `BobguiTreeView` will create its own `BobguiTreeSelection`.
 *
 * Returns: A newly created `BobguiTreeSelection` object.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
BobguiTreeSelection*
_bobgui_tree_selection_new (void)
{
  BobguiTreeSelection *selection;

  selection = g_object_new (BOBGUI_TYPE_TREE_SELECTION, NULL);

  return selection;
}

/**
 * _bobgui_tree_selection_new_with_tree_view:
 * @tree_view: The `BobguiTreeView`.
 *
 * Creates a new `BobguiTreeSelection` object.  This function should not be invoked,
 * as each `BobguiTreeView` will create its own `BobguiTreeSelection`.
 *
 * Returns: A newly created `BobguiTreeSelection` object.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
BobguiTreeSelection*
_bobgui_tree_selection_new_with_tree_view (BobguiTreeView *tree_view)
{
  BobguiTreeSelection *selection;

  g_return_val_if_fail (BOBGUI_IS_TREE_VIEW (tree_view), NULL);

  selection = _bobgui_tree_selection_new ();
  _bobgui_tree_selection_set_tree_view (selection, tree_view);

  return selection;
}

/**
 * _bobgui_tree_selection_set_tree_view:
 * @selection: A `BobguiTreeSelection`.
 * @tree_view: The `BobguiTreeView`.
 *
 * Sets the `BobguiTreeView` of @selection.  This function should not be invoked, as
 * it is used internally by `BobguiTreeView`.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
void
_bobgui_tree_selection_set_tree_view (BobguiTreeSelection *selection,
                                   BobguiTreeView      *tree_view)
{

  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (selection));
  if (tree_view != NULL)
    g_return_if_fail (BOBGUI_IS_TREE_VIEW (tree_view));

  selection->tree_view = tree_view;
}

/**
 * bobgui_tree_selection_set_mode:
 * @selection: A `BobguiTreeSelection`.
 * @type: The selection mode
 *
 * Sets the selection mode of the @selection.  If the previous type was
 * %BOBGUI_SELECTION_MULTIPLE, then the anchor is kept selected, if it was
 * previously selected.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
void
bobgui_tree_selection_set_mode (BobguiTreeSelection *selection,
			     BobguiSelectionMode  type)
{
  BobguiTreeSelectionFunc tmp_func;

  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (selection));

  if (selection->type == type)
    return;

  if (type == BOBGUI_SELECTION_NONE)
    {
      /* We do this so that we unconditionally unset all rows
       */
      tmp_func = selection->user_func;
      selection->user_func = NULL;
      bobgui_tree_selection_unselect_all (selection);
      selection->user_func = tmp_func;

      _bobgui_tree_view_set_anchor_path (selection->tree_view, NULL);
    }
  else if (type == BOBGUI_SELECTION_SINGLE ||
	   type == BOBGUI_SELECTION_BROWSE)
    {
      BobguiTreeRBTree *tree = NULL;
      BobguiTreeRBNode *node = NULL;
      int selected = FALSE;
      BobguiTreePath *anchor_path = NULL;

      anchor_path = _bobgui_tree_view_get_anchor_path (selection->tree_view);

      if (anchor_path)
	{
	  _bobgui_tree_view_find_node (selection->tree_view,
				    anchor_path,
				    &tree,
				    &node);

	  if (node && BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
	    selected = TRUE;
	}

      /* We do this so that we unconditionally unset all rows
       */
      tmp_func = selection->user_func;
      selection->user_func = NULL;
      bobgui_tree_selection_unselect_all (selection);
      selection->user_func = tmp_func;

      if (node && selected)
	_bobgui_tree_selection_internal_select_node (selection,
						  node,
						  tree,
						  anchor_path,
                                                  0,
						  FALSE);
      if (anchor_path)
	bobgui_tree_path_free (anchor_path);
    }

  selection->type = type;

  g_object_notify_by_pspec (G_OBJECT (selection), properties[PROP_MODE]);
}

/**
 * bobgui_tree_selection_get_mode:
 * @selection: a `BobguiTreeSelection`
 *
 * Gets the selection mode for @selection. See
 * bobgui_tree_selection_set_mode().
 *
 * Returns: the current selection mode
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
BobguiSelectionMode
bobgui_tree_selection_get_mode (BobguiTreeSelection *selection)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_SELECTION (selection), BOBGUI_SELECTION_SINGLE);

  return selection->type;
}

/**
 * bobgui_tree_selection_set_select_function:
 * @selection: A `BobguiTreeSelection`.
 * @func: (nullable): The selection function. May be %NULL
 * @data: The selection function’s data. May be %NULL
 * @destroy: The destroy function for user data.  May be %NULL
 *
 * Sets the selection function.
 *
 * If set, this function is called before any node is selected or unselected,
 * giving some control over which nodes are selected. The select function
 * should return %TRUE if the state of the node may be toggled, and %FALSE
 * if the state of the node should be left unchanged.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 */
void
bobgui_tree_selection_set_select_function (BobguiTreeSelection     *selection,
					BobguiTreeSelectionFunc  func,
					gpointer              data,
					GDestroyNotify        destroy)
{

  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (selection));

  if (selection->destroy)
    selection->destroy (selection->user_data);

  selection->user_func = func;
  selection->user_data = data;
  selection->destroy = destroy;
}

/**
 * bobgui_tree_selection_get_select_function: (skip)
 * @selection: A `BobguiTreeSelection`.
 *
 * Returns the current selection function.
 *
 * Returns: The function.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
BobguiTreeSelectionFunc
bobgui_tree_selection_get_select_function (BobguiTreeSelection *selection)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_SELECTION (selection), NULL);

  return selection->user_func;
}

/**
 * bobgui_tree_selection_get_user_data: (skip)
 * @selection: A `BobguiTreeSelection`.
 *
 * Returns the user data for the selection function.
 *
 * Returns: The user data.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
gpointer
bobgui_tree_selection_get_user_data (BobguiTreeSelection *selection)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_SELECTION (selection), NULL);

  return selection->user_data;
}

/**
 * bobgui_tree_selection_get_tree_view:
 * @selection: A `BobguiTreeSelection`
 *
 * Returns the tree view associated with @selection.
 *
 * Returns: (transfer none): A `BobguiTreeView`
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
BobguiTreeView *
bobgui_tree_selection_get_tree_view (BobguiTreeSelection *selection)
{
  g_return_val_if_fail (BOBGUI_IS_TREE_SELECTION (selection), NULL);

  return selection->tree_view;
}

/**
 * bobgui_tree_selection_get_selected:
 * @selection: A `BobguiTreeSelection`.
 * @model: (out) (optional) (transfer none): A pointer to set to the `BobguiTreeModel`
 * @iter: (out) (optional): The `BobguiTreeIter`
 *
 * Sets @iter to the currently selected node if @selection is set to
 * %BOBGUI_SELECTION_SINGLE or %BOBGUI_SELECTION_BROWSE.  @iter may be NULL if you
 * just want to test if @selection has any selected nodes.  @model is filled
 * with the current model as a convenience.  This function will not work if you
 * use @selection is %BOBGUI_SELECTION_MULTIPLE.
 *
 * Returns: TRUE, if there is a selected node.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
gboolean
bobgui_tree_selection_get_selected (BobguiTreeSelection  *selection,
				 BobguiTreeModel     **model,
				 BobguiTreeIter       *iter)
{
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  BobguiTreePath *anchor_path;
  gboolean retval = FALSE;
  gboolean found_node;

  g_return_val_if_fail (BOBGUI_IS_TREE_SELECTION (selection), FALSE);
  g_return_val_if_fail (selection->type != BOBGUI_SELECTION_MULTIPLE, FALSE);
  g_return_val_if_fail (selection->tree_view != NULL, FALSE);

  /* Clear the iter */
  if (iter)
    memset (iter, 0, sizeof (BobguiTreeIter));

  if (model)
    *model = bobgui_tree_view_get_model (selection->tree_view);

  anchor_path = _bobgui_tree_view_get_anchor_path (selection->tree_view);

  if (anchor_path == NULL)
    return FALSE;

  found_node = !_bobgui_tree_view_find_node (selection->tree_view,
                                          anchor_path,
                                          &tree,
                                          &node);

  if (found_node && BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
    {
      /* we only want to return the anchor if it exists in the rbtree and
       * is selected.
       */
      if (iter == NULL)
	retval = TRUE;
      else
        retval = bobgui_tree_model_get_iter (bobgui_tree_view_get_model (selection->tree_view),
                                          iter,
                                          anchor_path);
    }
  else
    {
      /* We don't want to return the anchor if it isn't actually selected.
       */
      retval = FALSE;
    }

  bobgui_tree_path_free (anchor_path);

  return retval;
}

/**
 * bobgui_tree_selection_get_selected_rows:
 * @selection: A `BobguiTreeSelection`.
 * @model: (out) (optional) (transfer none): A pointer to set to the `BobguiTreeModel`
 *
 * Creates a list of path of all selected rows. Additionally, if you are
 * planning on modifying the model after calling this function, you may
 * want to convert the returned list into a list of `BobguiTreeRowReference`s.
 * To do this, you can use bobgui_tree_row_reference_new().
 *
 * To free the return value, use:
 *
 * ```c
 * g_list_free_full (list, (GDestroyNotify) bobgui_tree_path_free);
 * ```
 *
 * Returns: (element-type BobguiTreePath) (transfer full): A `GList` containing a `BobguiTreePath` for each selected row.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
GList *
bobgui_tree_selection_get_selected_rows (BobguiTreeSelection   *selection,
                                      BobguiTreeModel      **model)
{
  GList *list = NULL;
  BobguiTreeRBTree *tree = NULL;
  BobguiTreeRBNode *node = NULL;
  BobguiTreePath *path;

  g_return_val_if_fail (BOBGUI_IS_TREE_SELECTION (selection), NULL);
  g_return_val_if_fail (selection->tree_view != NULL, NULL);

  if (model)
    *model = bobgui_tree_view_get_model (selection->tree_view);

  tree = _bobgui_tree_view_get_rbtree (selection->tree_view);

  if (tree == NULL || tree->root == NULL)
    return NULL;

  if (selection->type == BOBGUI_SELECTION_NONE)
    return NULL;
  else if (selection->type != BOBGUI_SELECTION_MULTIPLE)
    {
      BobguiTreeIter iter;

      if (bobgui_tree_selection_get_selected (selection, NULL, &iter))
        {
	  path = bobgui_tree_model_get_path (bobgui_tree_view_get_model (selection->tree_view), &iter);
	  list = g_list_append (list, path);

	  return list;
	}

      return NULL;
    }

  node = bobgui_tree_rbtree_first (tree);
  path = bobgui_tree_path_new_first ();

  while (node != NULL)
    {
      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
	list = g_list_prepend (list, bobgui_tree_path_copy (path));

      if (node->children)
        {
	  tree = node->children;
          node = bobgui_tree_rbtree_first (tree);

	  bobgui_tree_path_append_index (path, 0);
	}
      else
        {
	  gboolean done = FALSE;

	  do
	    {
	      node = bobgui_tree_rbtree_next (tree, node);
	      if (node != NULL)
	        {
		  done = TRUE;
		  bobgui_tree_path_next (path);
		}
	      else
	        {
		  node = tree->parent_node;
		  tree = tree->parent_tree;

		  if (!tree)
		    {
		      bobgui_tree_path_free (path);

		      goto done;
		    }

		  bobgui_tree_path_up (path);
		}
	    }
	  while (!done);
	}
    }

  bobgui_tree_path_free (path);

 done:
  return g_list_reverse (list);
}

static void
bobgui_tree_selection_count_selected_rows_helper (BobguiTreeRBTree *tree,
                                               BobguiTreeRBNode *node,
                                               gpointer       data)
{
  int *count = (int *)data;

  g_return_if_fail (node != NULL);

  if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
    (*count)++;

  if (node->children)
    bobgui_tree_rbtree_traverse (node->children, node->children->root,
                              G_PRE_ORDER,
                              bobgui_tree_selection_count_selected_rows_helper, data);
}

/**
 * bobgui_tree_selection_count_selected_rows:
 * @selection: A `BobguiTreeSelection`.
 *
 * Returns the number of rows that have been selected in @tree.
 *
 * Returns: The number of rows selected.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
int
bobgui_tree_selection_count_selected_rows (BobguiTreeSelection *selection)
{
  int count = 0;
  BobguiTreeRBTree *tree;

  g_return_val_if_fail (BOBGUI_IS_TREE_SELECTION (selection), 0);
  g_return_val_if_fail (selection->tree_view != NULL, 0);

  tree = _bobgui_tree_view_get_rbtree (selection->tree_view);

  if (tree == NULL || tree->root == NULL)
    return 0;

  if (selection->type == BOBGUI_SELECTION_SINGLE ||
      selection->type == BOBGUI_SELECTION_BROWSE)
    {
      if (bobgui_tree_selection_get_selected (selection, NULL, NULL))
        return 1;
      else
        return 0;
    }

  bobgui_tree_rbtree_traverse (tree, tree->root,
                            G_PRE_ORDER,
                            bobgui_tree_selection_count_selected_rows_helper,
                            &count);

  return count;
}

/* bobgui_tree_selection_selected_foreach helper */
static void
model_changed (gpointer data)
{
  gboolean *stop = (gboolean *)data;

  *stop = TRUE;
}

/**
 * bobgui_tree_selection_selected_foreach:
 * @selection: A `BobguiTreeSelection`.
 * @func: (scope call): The function to call for each selected node.
 * @data: user data to pass to the function.
 *
 * Calls a function for each selected node. Note that you cannot modify
 * the tree or selection from within this function. As a result,
 * bobgui_tree_selection_get_selected_rows() might be more useful.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
void
bobgui_tree_selection_selected_foreach (BobguiTreeSelection            *selection,
				     BobguiTreeSelectionForeachFunc  func,
				     gpointer                     data)
{
  BobguiTreePath *path;
  BobguiTreeRBTree *tree;
  BobguiTreeRBNode *node;
  BobguiTreeIter iter;
  BobguiTreeModel *model;

  gulong inserted_id, deleted_id, reordered_id, changed_id;
  gboolean stop = FALSE;

  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);

  tree = _bobgui_tree_view_get_rbtree (selection->tree_view);

  if (func == NULL || tree == NULL || tree->root == NULL)
    return;

  model = bobgui_tree_view_get_model (selection->tree_view);

  if (selection->type == BOBGUI_SELECTION_SINGLE ||
      selection->type == BOBGUI_SELECTION_BROWSE)
    {
      path = _bobgui_tree_view_get_anchor_path (selection->tree_view);

      if (path)
	{
	  bobgui_tree_model_get_iter (model, &iter, path);
	  (* func) (model, path, &iter, data);
	  bobgui_tree_path_free (path);
	}
      return;
    }

  node = bobgui_tree_rbtree_first (tree);

  g_object_ref (model);

  /* connect to signals to monitor changes in treemodel */
  inserted_id = g_signal_connect_swapped (model, "row-inserted",
					  G_CALLBACK (model_changed),
				          &stop);
  deleted_id = g_signal_connect_swapped (model, "row-deleted",
					 G_CALLBACK (model_changed),
				         &stop);
  reordered_id = g_signal_connect_swapped (model, "rows-reordered",
					   G_CALLBACK (model_changed),
				           &stop);
  changed_id = g_signal_connect_swapped (selection->tree_view, "notify::model",
					 G_CALLBACK (model_changed),
					 &stop);

  /* find the node internally */
  path = bobgui_tree_path_new_first ();

  while (node != NULL)
    {
      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
        {
          bobgui_tree_model_get_iter (model, &iter, path);
	  (* func) (model, path, &iter, data);
        }

      if (stop)
	goto out;

      if (node->children)
	{
	  tree = node->children;
          node = bobgui_tree_rbtree_first (tree);

	  bobgui_tree_path_append_index (path, 0);
	}
      else
	{
	  gboolean done = FALSE;

	  do
	    {
	      node = bobgui_tree_rbtree_next (tree, node);
	      if (node != NULL)
		{
		  done = TRUE;
		  bobgui_tree_path_next (path);
		}
	      else
		{
		  node = tree->parent_node;
		  tree = tree->parent_tree;

		  if (tree == NULL)
		    {
		      /* we've run out of tree */
		      /* We're done with this function */

		      goto out;
		    }

		  bobgui_tree_path_up (path);
		}
	    }
	  while (!done);
	}
    }

out:
  if (path)
    bobgui_tree_path_free (path);

  g_signal_handler_disconnect (model, inserted_id);
  g_signal_handler_disconnect (model, deleted_id);
  g_signal_handler_disconnect (model, reordered_id);
  g_signal_handler_disconnect (selection->tree_view, changed_id);
  g_object_unref (model);

  /* check if we have to spew a scary message */
  if (stop)
    g_warning ("The model has been modified from within bobgui_tree_selection_selected_foreach.\n"
	       "This function is for observing the selections of the tree only.  If\n"
	       "you are trying to get all selected items from the tree, try using\n"
	       "bobgui_tree_selection_get_selected_rows instead.");
}

/**
 * bobgui_tree_selection_select_path:
 * @selection: A `BobguiTreeSelection`.
 * @path: The `BobguiTreePath` to be selected.
 *
 * Select the row at @path.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
void
bobgui_tree_selection_select_path (BobguiTreeSelection *selection,
                                BobguiTreePath      *path)
{
  BobguiTreeRBNode *node;
  BobguiTreeRBTree *tree;
  gboolean ret;
  BobguiTreeSelectMode mode = 0;

  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);
  g_return_if_fail (path != NULL);

  ret = _bobgui_tree_view_find_node (selection->tree_view,
				  path,
				  &tree,
				  &node);

  if (node == NULL || BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED) ||
      ret == TRUE)
    return;

  if (selection->type == BOBGUI_SELECTION_MULTIPLE)
    mode = BOBGUI_TREE_SELECT_MODE_TOGGLE;

  _bobgui_tree_selection_internal_select_node (selection,
					    node,
					    tree,
					    path,
                                            mode,
					    FALSE);
}

/**
 * bobgui_tree_selection_unselect_path:
 * @selection: A `BobguiTreeSelection`.
 * @path: The `BobguiTreePath` to be unselected.
 *
 * Unselects the row at @path.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
void
bobgui_tree_selection_unselect_path (BobguiTreeSelection *selection,
				  BobguiTreePath      *path)
{
  BobguiTreeRBNode *node;
  BobguiTreeRBTree *tree;
  gboolean ret;

  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);
  g_return_if_fail (path != NULL);

  ret = _bobgui_tree_view_find_node (selection->tree_view,
				  path,
				  &tree,
				  &node);

  if (node == NULL || !BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED) ||
      ret == TRUE)
    return;

  _bobgui_tree_selection_internal_select_node (selection,
					    node,
					    tree,
					    path,
                                            BOBGUI_TREE_SELECT_MODE_TOGGLE,
					    TRUE);
}

/**
 * bobgui_tree_selection_select_iter:
 * @selection: A `BobguiTreeSelection`.
 * @iter: The `BobguiTreeIter` to be selected.
 *
 * Selects the specified iterator.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
void
bobgui_tree_selection_select_iter (BobguiTreeSelection *selection,
				BobguiTreeIter      *iter)
{
  BobguiTreePath *path;
  BobguiTreeModel *model;

  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);

  model = bobgui_tree_view_get_model (selection->tree_view);
  g_return_if_fail (model != NULL);
  g_return_if_fail (iter != NULL);

  path = bobgui_tree_model_get_path (model, iter);

  if (path == NULL)
    return;

  bobgui_tree_selection_select_path (selection, path);
  bobgui_tree_path_free (path);
}


/**
 * bobgui_tree_selection_unselect_iter:
 * @selection: A `BobguiTreeSelection`.
 * @iter: The `BobguiTreeIter` to be unselected.
 *
 * Unselects the specified iterator.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
void
bobgui_tree_selection_unselect_iter (BobguiTreeSelection *selection,
				  BobguiTreeIter      *iter)
{
  BobguiTreePath *path;
  BobguiTreeModel *model;

  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);

  model = bobgui_tree_view_get_model (selection->tree_view);
  g_return_if_fail (model != NULL);
  g_return_if_fail (iter != NULL);

  path = bobgui_tree_model_get_path (model, iter);

  if (path == NULL)
    return;

  bobgui_tree_selection_unselect_path (selection, path);
  bobgui_tree_path_free (path);
}

/**
 * bobgui_tree_selection_path_is_selected:
 * @selection: A `BobguiTreeSelection`.
 * @path: A `BobguiTreePath` to check selection on.
 *
 * Returns %TRUE if the row pointed to by @path is currently selected.  If @path
 * does not point to a valid location, %FALSE is returned
 *
 * Returns: %TRUE if @path is selected.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
gboolean
bobgui_tree_selection_path_is_selected (BobguiTreeSelection *selection,
				     BobguiTreePath      *path)
{
  BobguiTreeRBNode *node;
  BobguiTreeRBTree *tree;
  gboolean ret;

  g_return_val_if_fail (BOBGUI_IS_TREE_SELECTION (selection), FALSE);
  g_return_val_if_fail (path != NULL, FALSE);
  g_return_val_if_fail (selection->tree_view != NULL, FALSE);

  if (bobgui_tree_view_get_model (selection->tree_view) == NULL)
    return FALSE;

  ret = _bobgui_tree_view_find_node (selection->tree_view,
				  path,
				  &tree,
				  &node);

  if ((node == NULL) || !BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED) ||
      ret == TRUE)
    return FALSE;

  return TRUE;
}

/**
 * bobgui_tree_selection_iter_is_selected:
 * @selection: A `BobguiTreeSelection`
 * @iter: A valid `BobguiTreeIter`
 *
 * Returns %TRUE if the row at @iter is currently selected.
 *
 * Returns: %TRUE, if @iter is selected
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
gboolean
bobgui_tree_selection_iter_is_selected (BobguiTreeSelection *selection,
				     BobguiTreeIter      *iter)
{
  BobguiTreePath *path;
  BobguiTreeModel *model;
  gboolean retval;

  g_return_val_if_fail (BOBGUI_IS_TREE_SELECTION (selection), FALSE);
  g_return_val_if_fail (iter != NULL, FALSE);
  g_return_val_if_fail (selection->tree_view != NULL, FALSE);

  model = bobgui_tree_view_get_model (selection->tree_view);
  g_return_val_if_fail (model != NULL, FALSE);

  path = bobgui_tree_model_get_path (model, iter);
  if (path == NULL)
    return FALSE;

  retval = bobgui_tree_selection_path_is_selected (selection, path);
  bobgui_tree_path_free (path);

  return retval;
}


/* Wish I was in python, right now... */
struct _TempTuple {
  BobguiTreeSelection *selection;
  int dirty;
};

static void
select_all_helper (BobguiTreeRBTree  *tree,
		   BobguiTreeRBNode  *node,
		   gpointer    data)
{
  struct _TempTuple *tuple = data;

  if (node->children)
    bobgui_tree_rbtree_traverse (node->children,
			  node->children->root,
			  G_PRE_ORDER,
			  select_all_helper,
			  data);
  if (!BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
    {
      tuple->dirty = bobgui_tree_selection_real_select_node (tuple->selection, tree, node, TRUE) || tuple->dirty;
    }
}


/* We have a real_{un,}select_all function that doesn't emit the signal, so we
 * can use it in other places without fear of the signal being emitted.
 */
static int
bobgui_tree_selection_real_select_all (BobguiTreeSelection *selection)
{
  struct _TempTuple *tuple;
  BobguiTreeRBTree *tree;

  tree = _bobgui_tree_view_get_rbtree (selection->tree_view);

  if (tree == NULL)
    return FALSE;

  /* Mark all nodes selected */
  tuple = g_new (struct _TempTuple, 1);
  tuple->selection = selection;
  tuple->dirty = FALSE;

  bobgui_tree_rbtree_traverse (tree, tree->root,
                            G_PRE_ORDER,
                            select_all_helper,
                            tuple);
  if (tuple->dirty)
    {
      g_free (tuple);
      return TRUE;
    }
  g_free (tuple);
  return FALSE;
}

/**
 * bobgui_tree_selection_select_all:
 * @selection: A `BobguiTreeSelection`.
 *
 * Selects all the nodes. @selection must be set to %BOBGUI_SELECTION_MULTIPLE
 * mode.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
void
bobgui_tree_selection_select_all (BobguiTreeSelection *selection)
{

  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);

  if (_bobgui_tree_view_get_rbtree (selection->tree_view) == NULL ||
      bobgui_tree_view_get_model (selection->tree_view) == NULL)
    return;

  g_return_if_fail (selection->type == BOBGUI_SELECTION_MULTIPLE);

  if (bobgui_tree_selection_real_select_all (selection))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

static void
unselect_all_helper (BobguiTreeRBTree *tree,
                     BobguiTreeRBNode *node,
                     gpointer       data)
{
  struct _TempTuple *tuple = data;

  if (node->children)
    bobgui_tree_rbtree_traverse (node->children,
                              node->children->root,
                              G_PRE_ORDER,
                              unselect_all_helper,
                              data);
  if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
    {
      tuple->dirty = bobgui_tree_selection_real_select_node (tuple->selection, tree, node, FALSE) || tuple->dirty;
    }
}

static gboolean
bobgui_tree_selection_real_unselect_all (BobguiTreeSelection *selection)
{
  struct _TempTuple *tuple;

  if (selection->type == BOBGUI_SELECTION_SINGLE ||
      selection->type == BOBGUI_SELECTION_BROWSE)
    {
      BobguiTreeRBTree *tree = NULL;
      BobguiTreeRBNode *node = NULL;
      BobguiTreePath *anchor_path;

      anchor_path = _bobgui_tree_view_get_anchor_path (selection->tree_view);

      if (anchor_path == NULL)
        return FALSE;

      _bobgui_tree_view_find_node (selection->tree_view,
                                anchor_path,
				&tree,
				&node);

      bobgui_tree_path_free (anchor_path);

      if (tree == NULL)
        return FALSE;

      if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
	{
	  if (bobgui_tree_selection_real_select_node (selection, tree, node, FALSE))
	    {
	      _bobgui_tree_view_set_anchor_path (selection->tree_view, NULL);
	      return TRUE;
	    }
	}
      return FALSE;
    }
  else
    {
      BobguiTreeRBTree *tree;

      tuple = g_new (struct _TempTuple, 1);
      tuple->selection = selection;
      tuple->dirty = FALSE;

      tree = _bobgui_tree_view_get_rbtree (selection->tree_view);
      bobgui_tree_rbtree_traverse (tree, tree->root,
                                G_PRE_ORDER,
                                unselect_all_helper,
                                tuple);

      if (tuple->dirty)
        {
          g_free (tuple);
          return TRUE;
        }
      g_free (tuple);
      return FALSE;
    }
}

/**
 * bobgui_tree_selection_unselect_all:
 * @selection: A `BobguiTreeSelection`.
 *
 * Unselects all the nodes.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
void
bobgui_tree_selection_unselect_all (BobguiTreeSelection *selection)
{

  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);

  if (_bobgui_tree_view_get_rbtree (selection->tree_view) == NULL ||
      bobgui_tree_view_get_model (selection->tree_view) == NULL)
    return;

  if (bobgui_tree_selection_real_unselect_all (selection))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

enum
{
  RANGE_SELECT,
  RANGE_UNSELECT
};

static int
bobgui_tree_selection_real_modify_range (BobguiTreeSelection *selection,
                                      int               mode,
				      BobguiTreePath      *start_path,
				      BobguiTreePath      *end_path)
{
  BobguiTreeRBNode *start_node = NULL, *end_node = NULL;
  BobguiTreeRBTree *start_tree, *end_tree;
  BobguiTreePath *anchor_path = NULL;
  gboolean dirty = FALSE;

  switch (bobgui_tree_path_compare (start_path, end_path))
    {
    case 1:
      _bobgui_tree_view_find_node (selection->tree_view,
				end_path,
				&start_tree,
				&start_node);
      _bobgui_tree_view_find_node (selection->tree_view,
				start_path,
				&end_tree,
				&end_node);
      anchor_path = start_path;
      break;
    case 0:
      _bobgui_tree_view_find_node (selection->tree_view,
				start_path,
				&start_tree,
				&start_node);
      end_tree = start_tree;
      end_node = start_node;
      anchor_path = start_path;
      break;
    case -1:
      _bobgui_tree_view_find_node (selection->tree_view,
				start_path,
				&start_tree,
				&start_node);
      _bobgui_tree_view_find_node (selection->tree_view,
				end_path,
				&end_tree,
				&end_node);
      anchor_path = start_path;
      break;
    default:
      g_assert_not_reached ();
      break;
    }

  /* Invalid start or end node? */
  if (start_node == NULL || end_node == NULL)
    return dirty;

  if (anchor_path)
    _bobgui_tree_view_set_anchor_path (selection->tree_view, anchor_path);

  do
    {
      dirty |= bobgui_tree_selection_real_select_node (selection, start_tree, start_node, (mode == RANGE_SELECT)?TRUE:FALSE);

      if (start_node == end_node)
	break;

      if (start_node->children)
	{
	  start_tree = start_node->children;
          start_node = bobgui_tree_rbtree_first (start_tree);
	}
      else
	{
	  bobgui_tree_rbtree_next_full (start_tree, start_node, &start_tree, &start_node);
	  if (start_tree == NULL)
	    {
	      /* we just ran out of tree.  That means someone passed in bogus values.
	       */
	      return dirty;
	    }
	}
    }
  while (TRUE);

  return dirty;
}

/**
 * bobgui_tree_selection_select_range:
 * @selection: A `BobguiTreeSelection`.
 * @start_path: The initial node of the range.
 * @end_path: The final node of the range.
 *
 * Selects a range of nodes, determined by @start_path and @end_path inclusive.
 * @selection must be set to %BOBGUI_SELECTION_MULTIPLE mode.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
void
bobgui_tree_selection_select_range (BobguiTreeSelection *selection,
				 BobguiTreePath      *start_path,
				 BobguiTreePath      *end_path)
{

  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);
  g_return_if_fail (selection->type == BOBGUI_SELECTION_MULTIPLE);
  g_return_if_fail (bobgui_tree_view_get_model (selection->tree_view) != NULL);

  if (bobgui_tree_selection_real_modify_range (selection, RANGE_SELECT, start_path, end_path))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

/**
 * bobgui_tree_selection_unselect_range:
 * @selection: A `BobguiTreeSelection`.
 * @start_path: The initial node of the range.
 * @end_path: The initial node of the range.
 *
 * Unselects a range of nodes, determined by @start_path and @end_path
 * inclusive.
 *
 * Deprecated: 4.10: Use BobguiListView or BobguiColumnView
 **/
void
bobgui_tree_selection_unselect_range (BobguiTreeSelection *selection,
                                   BobguiTreePath      *start_path,
				   BobguiTreePath      *end_path)
{

  g_return_if_fail (BOBGUI_IS_TREE_SELECTION (selection));
  g_return_if_fail (selection->tree_view != NULL);
  g_return_if_fail (bobgui_tree_view_get_model (selection->tree_view) != NULL);

  if (bobgui_tree_selection_real_modify_range (selection, RANGE_UNSELECT, start_path, end_path))
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

gboolean
_bobgui_tree_selection_row_is_selectable (BobguiTreeSelection *selection,
                                       BobguiTreeRBNode    *node,
                                       BobguiTreePath      *path)
{
  BobguiTreeIter iter;
  BobguiTreeModel *model;
  BobguiTreeViewRowSeparatorFunc separator_func;
  gpointer separator_data;
  gboolean sensitive = FALSE;

  model = bobgui_tree_view_get_model (selection->tree_view);

  _bobgui_tree_view_get_row_separator_func (selection->tree_view,
					 &separator_func, &separator_data);

  if (!bobgui_tree_model_get_iter (model, &iter, path))
    sensitive = TRUE;

  if (!sensitive && separator_func)
    {
      /* never allow separators to be selected */
      if ((* separator_func) (model, &iter, separator_data))
	return FALSE;
    }

  if (selection->user_func)
    return (*selection->user_func) (selection, model, path,
				    BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED),
				    selection->user_data);
  else
    return TRUE;
}


/* Called internally by bobguitreeview.c It handles actually selecting the tree.
 */

/*
 * docs about the “override_browse_mode”, we set this flag when we want to
 * unset select the node and override the select browse mode behaviour (that is
 * 'one node should *always* be selected').
 */
void
_bobgui_tree_selection_internal_select_node (BobguiTreeSelection *selection,
                                          BobguiTreeRBNode    *node,
                                          BobguiTreeRBTree    *tree,
                                          BobguiTreePath      *path,
                                          BobguiTreeSelectMode mode,
                                          gboolean          override_browse_mode)
{
  int flags;
  int dirty = FALSE;
  BobguiTreePath *anchor_path = NULL;

  if (selection->type == BOBGUI_SELECTION_NONE)
    return;

  anchor_path = _bobgui_tree_view_get_anchor_path (selection->tree_view);

  if (selection->type == BOBGUI_SELECTION_SINGLE ||
      selection->type == BOBGUI_SELECTION_BROWSE)
    {
      /* just unselect */
      if (selection->type == BOBGUI_SELECTION_BROWSE && override_browse_mode)
        {
	  dirty = bobgui_tree_selection_real_unselect_all (selection);
	}
      /* Did we try to select the same node again? */
      else if (selection->type == BOBGUI_SELECTION_SINGLE &&
	       anchor_path && bobgui_tree_path_compare (path, anchor_path) == 0)
	{
	  if ((mode & BOBGUI_TREE_SELECT_MODE_TOGGLE) == BOBGUI_TREE_SELECT_MODE_TOGGLE)
	    {
	      dirty = bobgui_tree_selection_real_unselect_all (selection);
	    }
	}
      else
	{
	  if (anchor_path)
	    {
	      /* We only want to select the new node if we can unselect the old one,
	       * and we can select the new one. */
	      dirty = _bobgui_tree_selection_row_is_selectable (selection, node, path);

	      /* if dirty is FALSE, we weren't able to select the new one, otherwise, we try to
	       * unselect the new one
	       */
	      if (dirty)
		dirty = bobgui_tree_selection_real_unselect_all (selection);

	      /* if dirty is TRUE at this point, we successfully unselected the
	       * old one, and can then select the new one */
	      if (dirty)
		{

		  _bobgui_tree_view_set_anchor_path (selection->tree_view, NULL);

		  if (bobgui_tree_selection_real_select_node (selection, tree, node, TRUE))
		    _bobgui_tree_view_set_anchor_path (selection->tree_view, path);
		}
	    }
	  else
	    {
	      if (bobgui_tree_selection_real_select_node (selection, tree, node, TRUE))
		{
		  dirty = TRUE;

		  _bobgui_tree_view_set_anchor_path (selection->tree_view, path);
		}
	    }
	}
    }
  else if (selection->type == BOBGUI_SELECTION_MULTIPLE)
    {
      if ((mode & BOBGUI_TREE_SELECT_MODE_EXTEND) == BOBGUI_TREE_SELECT_MODE_EXTEND
          && (anchor_path == NULL))
	{
	  _bobgui_tree_view_set_anchor_path (selection->tree_view, path);

	  dirty = bobgui_tree_selection_real_select_node (selection, tree, node, TRUE);
	}
      else if ((mode & (BOBGUI_TREE_SELECT_MODE_EXTEND | BOBGUI_TREE_SELECT_MODE_TOGGLE)) == (BOBGUI_TREE_SELECT_MODE_EXTEND | BOBGUI_TREE_SELECT_MODE_TOGGLE))
	{
	  bobgui_tree_selection_select_range (selection,
					   anchor_path,
					   path);
	}
      else if ((mode & BOBGUI_TREE_SELECT_MODE_TOGGLE) == BOBGUI_TREE_SELECT_MODE_TOGGLE)
	{
	  flags = node->flags;

	  _bobgui_tree_view_set_anchor_path (selection->tree_view, path);

	  if ((flags & BOBGUI_TREE_RBNODE_IS_SELECTED) == BOBGUI_TREE_RBNODE_IS_SELECTED)
	    dirty |= bobgui_tree_selection_real_select_node (selection, tree, node, FALSE);
	  else
	    dirty |= bobgui_tree_selection_real_select_node (selection, tree, node, TRUE);
	}
      else if ((mode & BOBGUI_TREE_SELECT_MODE_EXTEND) == BOBGUI_TREE_SELECT_MODE_EXTEND)
	{
	  dirty = bobgui_tree_selection_real_unselect_all (selection);
	  dirty |= bobgui_tree_selection_real_modify_range (selection,
                                                         RANGE_SELECT,
							 anchor_path,
							 path);
	}
      else
	{
	  dirty = bobgui_tree_selection_real_unselect_all (selection);

	  _bobgui_tree_view_set_anchor_path (selection->tree_view, path);

	  dirty |= bobgui_tree_selection_real_select_node (selection, tree, node, TRUE);
	}
    }

  if (anchor_path)
    bobgui_tree_path_free (anchor_path);

  if (dirty)
    g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}


void
_bobgui_tree_selection_emit_changed (BobguiTreeSelection *selection)
{
  g_signal_emit (selection, tree_selection_signals[CHANGED], 0);
}

/* NOTE: Any {un,}selection ever done _MUST_ be done through this function!
 */

static int
bobgui_tree_selection_real_select_node (BobguiTreeSelection *selection,
                                     BobguiTreeRBTree    *tree,
                                     BobguiTreeRBNode    *node,
                                     gboolean          select)
{
  gboolean toggle = FALSE;
  BobguiTreePath *path = NULL;

  g_return_val_if_fail (node != NULL, FALSE);

  select = !! select;

  if (BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED) != select)
    {
      path = _bobgui_tree_path_new_from_rbtree (tree, node);
      toggle = _bobgui_tree_selection_row_is_selectable (selection, node, path);
      bobgui_tree_path_free (path);
    }

  if (toggle)
    {
      if (!BOBGUI_TREE_RBNODE_FLAG_SET (node, BOBGUI_TREE_RBNODE_IS_SELECTED))
        {
          BOBGUI_TREE_RBNODE_SET_FLAG (node, BOBGUI_TREE_RBNODE_IS_SELECTED);
        }
      else
        {
          BOBGUI_TREE_RBNODE_UNSET_FLAG (node, BOBGUI_TREE_RBNODE_IS_SELECTED);
        }

      bobgui_widget_queue_draw (BOBGUI_WIDGET (selection->tree_view));

      return TRUE;
    }

  return FALSE;
}
