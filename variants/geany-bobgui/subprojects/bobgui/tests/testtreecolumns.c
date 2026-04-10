/* testtreecolumns.c
 * Copyright (C) 2001 Red Hat, Inc
 * Author: Jonathan Blandford
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
#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/*
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 * README README README README README README README README README README
 *
 * DO NOT!!! I REPEAT DO NOT!  EVER LOOK AT THIS CODE AS AN EXAMPLE OF WHAT YOUR
 * CODE SHOULD LOOK LIKE.
 *
 * IT IS VERY CONFUSING, AND IS MEANT TO TEST A LOT OF CODE IN THE TREE.  WHILE
 * IT IS ACTUALLY CORRECT CODE, IT IS NOT USEFUL.
 */

BobguiWidget *left_tree_view;
BobguiWidget *top_right_tree_view;
BobguiWidget *bottom_right_tree_view;
BobguiTreeModel *left_tree_model;
BobguiTreeModel *top_right_tree_model;
BobguiTreeModel *bottom_right_tree_model;
BobguiWidget *sample_tree_view_top;
BobguiWidget *sample_tree_view_bottom;

#define column_data "my_column_data"

static void move_row  (BobguiTreeModel *src,
		       BobguiTreeIter  *src_iter,
		       BobguiTreeModel *dest,
		       BobguiTreeIter  *dest_iter);

/* Kids, don't try this at home.  */

/* Small BobguiTreeModel to model columns */
typedef struct _ViewColumnModel ViewColumnModel;
typedef struct _ViewColumnModelClass ViewColumnModelClass;

struct _ViewColumnModel
{
  BobguiListStore parent;
  BobguiTreeView *view;
  GList *columns;
  int stamp;
};

struct _ViewColumnModelClass
{
  BobguiListStoreClass parent_class;
};

static void view_column_model_tree_model_init (BobguiTreeModelIface *iface);
static void view_column_model_drag_source_init (BobguiTreeDragSourceIface *iface);
static void view_column_model_drag_dest_init (BobguiTreeDragDestIface *iface);


static GType view_column_model_get_type (void);
G_DEFINE_TYPE_WITH_CODE (ViewColumnModel, view_column_model, BOBGUI_TYPE_LIST_STORE,
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_TREE_MODEL, view_column_model_tree_model_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_TREE_DRAG_SOURCE, view_column_model_drag_source_init)
                         G_IMPLEMENT_INTERFACE (BOBGUI_TYPE_TREE_DRAG_DEST, view_column_model_drag_dest_init))



static void view_column_model_init (ViewColumnModel *model)
{
  model->stamp = g_random_int ();
}

static int
view_column_model_get_n_columns (BobguiTreeModel *tree_model)
{
  return 2;
}

static GType
view_column_model_get_column_type (BobguiTreeModel *tree_model,
				   int           index)
{
  switch (index)
    {
    case 0:
      return G_TYPE_STRING;
    case 1:
      return BOBGUI_TYPE_TREE_VIEW_COLUMN;
    default:
      return G_TYPE_INVALID;
    }
}

static gboolean
view_column_model_get_iter (BobguiTreeModel *tree_model,
			    BobguiTreeIter  *iter,
			    BobguiTreePath  *path)

{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;
  GList *list;
  int i;

  g_return_val_if_fail (bobgui_tree_path_get_depth (path) > 0, FALSE);

  i = bobgui_tree_path_get_indices (path)[0];
  list = g_list_nth (view_model->columns, i);

  if (list == NULL)
    return FALSE;

  iter->stamp = view_model->stamp;
  iter->user_data = list;

  return TRUE;
}

static BobguiTreePath *
view_column_model_get_path (BobguiTreeModel *tree_model,
			    BobguiTreeIter  *iter)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;
  BobguiTreePath *retval;
  GList *list;
  int i = 0;

  g_return_val_if_fail (iter->stamp == view_model->stamp, NULL);

  for (list = view_model->columns; list; list = list->next)
    {
      if (list == (GList *)iter->user_data)
	break;
      i++;
    }
  if (list == NULL)
    return NULL;

  retval = bobgui_tree_path_new ();
  bobgui_tree_path_append_index (retval, i);
  return retval;
}

static void
view_column_model_get_value (BobguiTreeModel *tree_model,
			     BobguiTreeIter  *iter,
			     int           column,
			     GValue       *value)
{
#ifndef G_DISABLE_CHECKS
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;

  g_return_if_fail (column < 2);
  g_return_if_fail (view_model->stamp == iter->stamp);
  g_return_if_fail (iter->user_data != NULL);
#endif

  if (column == 0)
    {
      g_value_init (value, G_TYPE_STRING);
      g_value_set_string (value, bobgui_tree_view_column_get_title (BOBGUI_TREE_VIEW_COLUMN (((GList *)iter->user_data)->data)));
    }
  else
    {
      g_value_init (value, BOBGUI_TYPE_TREE_VIEW_COLUMN);
      g_value_set_object (value, ((GList *)iter->user_data)->data);
    }
}

static gboolean
view_column_model_iter_next (BobguiTreeModel  *tree_model,
			     BobguiTreeIter   *iter)
{
#ifndef G_DISABLE_CHECKS
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;

  g_return_val_if_fail (view_model->stamp == iter->stamp, FALSE);
  g_return_val_if_fail (iter->user_data != NULL, FALSE);
#endif

  iter->user_data = ((GList *)iter->user_data)->next;
  return iter->user_data != NULL;
}

static gboolean
view_column_model_iter_children (BobguiTreeModel *tree_model,
				 BobguiTreeIter  *iter,
				 BobguiTreeIter  *parent)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;

  /* this is a list, nodes have no children */
  if (parent)
    return FALSE;

  /* but if parent == NULL we return the list itself as children of the
   * "root"
   */

  if (view_model->columns)
    {
      iter->stamp = view_model->stamp;
      iter->user_data = view_model->columns;
      return TRUE;
    }
  else
    return FALSE;
}

static gboolean
view_column_model_iter_has_child (BobguiTreeModel *tree_model,
				  BobguiTreeIter  *iter)
{
  return FALSE;
}

static int
view_column_model_iter_n_children (BobguiTreeModel *tree_model,
				   BobguiTreeIter  *iter)
{
  return g_list_length (((ViewColumnModel *)tree_model)->columns);
}

static int
view_column_model_iter_nth_child (BobguiTreeModel *tree_model,
 				  BobguiTreeIter  *iter,
				  BobguiTreeIter  *parent,
				  int           n)
{
  ViewColumnModel *view_model = (ViewColumnModel *)tree_model;

  if (parent)
    return FALSE;

  iter->stamp = view_model->stamp;
  iter->user_data = g_list_nth ((GList *)view_model->columns, n);

  return (iter->user_data != NULL);
}

static gboolean
view_column_model_iter_parent (BobguiTreeModel *tree_model,
			       BobguiTreeIter  *iter,
			       BobguiTreeIter  *child)
{
  return FALSE;
}

static void
view_column_model_tree_model_init (BobguiTreeModelIface *iface)
{
  iface->get_n_columns = view_column_model_get_n_columns;
  iface->get_column_type = view_column_model_get_column_type;
  iface->get_iter = view_column_model_get_iter;
  iface->get_path = view_column_model_get_path;
  iface->get_value = view_column_model_get_value;
  iface->iter_next = view_column_model_iter_next;
  iface->iter_children = view_column_model_iter_children;
  iface->iter_has_child = view_column_model_iter_has_child;
  iface->iter_n_children = view_column_model_iter_n_children;
  iface->iter_nth_child = view_column_model_iter_nth_child;
  iface->iter_parent = view_column_model_iter_parent;
}

static GdkContentProvider *
view_column_model_drag_data_get (BobguiTreeDragSource *drag_source,
				 BobguiTreePath       *path)
{
  return bobgui_tree_create_row_drag_content (BOBGUI_TREE_MODEL (drag_source), path);
}

static gboolean
view_column_model_drag_data_delete (BobguiTreeDragSource *drag_source,
				    BobguiTreePath       *path)
{
  /* Nothing -- we handle moves on the dest side */
  
  return TRUE;
}

static gboolean
view_column_model_row_drop_possible (BobguiTreeDragDest *drag_dest,
				     BobguiTreePath     *dest_path,
				     const GValue    *value)
{
  BobguiTreeModel *src_model;
  
  if (bobgui_tree_get_row_drag_data (value,
				  &src_model,
				  NULL))
    {
      if (src_model == left_tree_model ||
	  src_model == top_right_tree_model ||
	  src_model == bottom_right_tree_model)
	return TRUE;
    }

  return FALSE;
}

static gboolean
view_column_model_drag_data_received (BobguiTreeDragDest *drag_dest,
				      BobguiTreePath     *dest,
				      const GValue    *value)
{
  BobguiTreeModel *src_model;
  BobguiTreePath *src_path = NULL;
  gboolean retval = FALSE;
  
  if (bobgui_tree_get_row_drag_data (value,
				  &src_model,
				  &src_path))
    {
      BobguiTreeIter src_iter;
      BobguiTreeIter dest_iter;
      gboolean have_dest;

      /* We are a little lazy here, and assume if we can't convert dest
       * to an iter, we need to append. See bobguiliststore.c for a more
       * careful handling of this.
       */
      have_dest = bobgui_tree_model_get_iter (BOBGUI_TREE_MODEL (drag_dest), &dest_iter, dest);

      if (bobgui_tree_model_get_iter (src_model, &src_iter, src_path))
	{
	  if (src_model == left_tree_model ||
	      src_model == top_right_tree_model ||
	      src_model == bottom_right_tree_model)
	    {
	      move_row (src_model, &src_iter, BOBGUI_TREE_MODEL (drag_dest),
			have_dest ? &dest_iter : NULL);
	      retval = TRUE;
	    }
	}

      bobgui_tree_path_free (src_path);
    }
  
  return retval;
}

static void
view_column_model_drag_source_init (BobguiTreeDragSourceIface *iface)
{
  iface->drag_data_get = view_column_model_drag_data_get;
  iface->drag_data_delete = view_column_model_drag_data_delete;
}

static void
view_column_model_drag_dest_init (BobguiTreeDragDestIface *iface)
{
  iface->drag_data_received = view_column_model_drag_data_received;
  iface->row_drop_possible = view_column_model_row_drop_possible;
}

static void
view_column_model_class_init (ViewColumnModelClass *klass)
{
}

static void
update_columns (BobguiTreeView *view, ViewColumnModel *view_model)
{
  GList *old_columns = view_model->columns;
  int old_length, length;
  GList *a, *b;

  view_model->columns = bobgui_tree_view_get_columns (view_model->view);

  /* As the view tells us one change at a time, we can do this hack. */
  length = g_list_length (view_model->columns);
  old_length = g_list_length (old_columns);
  if (length != old_length)
    {
      BobguiTreePath *path;
      int i = 0;

      /* where are they different */
      for (a = old_columns, b = view_model->columns; a && b; a = a->next, b = b->next)
	{
	  if (a->data != b->data)
	    break;
	  i++;
	}
      path = bobgui_tree_path_new ();
      bobgui_tree_path_append_index (path, i);
      if (length < old_length)
	{
	  view_model->stamp++;
	  bobgui_tree_model_row_deleted (BOBGUI_TREE_MODEL (view_model), path);
	}
      else
	{
	  BobguiTreeIter iter;
	  iter.stamp = view_model->stamp;
	  iter.user_data = b;
	  bobgui_tree_model_row_inserted (BOBGUI_TREE_MODEL (view_model), path, &iter);
	}
      bobgui_tree_path_free (path);
    }
  else
    {
      int i;
      int m = 0, n = 1;
      int *new_order;
      BobguiTreePath *path;

      a = old_columns; b = view_model->columns;

      while (a->data == b->data)
	{
	  a = a->next;
	  b = b->next;
	  if (a == NULL)
	    return;
	  m++;
	}

      new_order = g_new (int, length);

      if (a->next->data == b->data)
	{
	  b = b->next;
	  while (b->data != a->data)
	    {
	      b = b->next;
	      n++;
	    }
	  for (i = 0; i < m; i++)
	    new_order[i] = i;
	  for (i = m; i < m+n; i++)
	    new_order[i] = i+1;
	  new_order[i] = m;
	  for (i = m + n +1; i < length; i++)
	    new_order[i] = i;
	}
      else
	{
	  a = a->next;
	  while (a->data != b->data)
	    {
	      a = a->next;
	      n++;
	    }
	  for (i = 0; i < m; i++)
	    new_order[i] = i;
	  new_order[m] = m+n;
	  for (i = m+1; i < m + n+ 1; i++)
	    new_order[i] = i - 1;
	  for (i = m + n + 1; i < length; i++)
	    new_order[i] = i;
	}

      path = bobgui_tree_path_new ();
      bobgui_tree_model_rows_reordered (BOBGUI_TREE_MODEL (view_model),
				     path,
				     NULL,
				     new_order);
      bobgui_tree_path_free (path);
      g_free (new_order);
    }
  if (old_columns)
    g_list_free (old_columns);
}

static BobguiTreeModel *
view_column_model_new (BobguiTreeView *view)
{
  BobguiTreeModel *retval;

  retval = g_object_new (view_column_model_get_type (), NULL);
  ((ViewColumnModel *)retval)->view = view;
  ((ViewColumnModel *)retval)->columns = bobgui_tree_view_get_columns (view);

  g_signal_connect (view, "columns_changed", G_CALLBACK (update_columns), retval);

  return retval;
}

/* Back to sanity.
 */

static void
add_clicked (BobguiWidget *button, gpointer data)
{
  static int i = 0;

  BobguiTreeIter iter;
  BobguiTreeViewColumn *column;
  BobguiTreeSelection *selection;
  BobguiCellRenderer *cell;
  char *label = g_strdup_printf ("Column %d", i);

  cell = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes (label, cell, "text", 0, NULL);
  g_object_set_data_full (G_OBJECT (column), column_data, label, g_free);
  bobgui_tree_view_column_set_reorderable (column, TRUE);
  bobgui_tree_view_column_set_sizing (column, BOBGUI_TREE_VIEW_COLUMN_GROW_ONLY);
  bobgui_tree_view_column_set_resizable (column, TRUE);
  bobgui_list_store_append (BOBGUI_LIST_STORE (left_tree_model), &iter);
  bobgui_list_store_set (BOBGUI_LIST_STORE (left_tree_model), &iter, 0, label, 1, column, -1);
  i++;

  selection = bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (left_tree_view));
  bobgui_tree_selection_select_iter (selection, &iter);
}

static void
get_visible (BobguiTreeViewColumn *tree_column,
	     BobguiCellRenderer   *cell,
	     BobguiTreeModel      *tree_model,
	     BobguiTreeIter       *iter,
	     gpointer           data)
{
  BobguiTreeViewColumn *column;

  bobgui_tree_model_get (tree_model, iter, 1, &column, -1);
  if (column)
    {
      bobgui_cell_renderer_toggle_set_active (BOBGUI_CELL_RENDERER_TOGGLE (cell),
					   bobgui_tree_view_column_get_visible (column));
    }
}

static void
set_visible (BobguiCellRendererToggle *cell,
	     char                  *path_str,
	     gpointer               data)
{
  BobguiTreeView *tree_view = (BobguiTreeView *) data;
  BobguiTreeViewColumn *column;
  BobguiTreeModel *model;
  BobguiTreeIter iter;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_str);

  model = bobgui_tree_view_get_model (tree_view);

  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_tree_model_get (model, &iter, 1, &column, -1);

  if (column)
    {
      bobgui_tree_view_column_set_visible (column, ! bobgui_tree_view_column_get_visible (column));
      bobgui_tree_model_row_changed (model, path, &iter);
    }
  bobgui_tree_path_free (path);
}

static void
move_to_left (BobguiTreeModel *src,
	      BobguiTreeIter  *src_iter,
	      BobguiTreeIter  *dest_iter)
{
  BobguiTreeIter iter;
  BobguiTreeViewColumn *column;
  BobguiTreeSelection *selection;
  char *label;

  bobgui_tree_model_get (src, src_iter, 0, &label, 1, &column, -1);

  if (src == top_right_tree_model)
    bobgui_tree_view_remove_column (BOBGUI_TREE_VIEW (sample_tree_view_top), column);
  else
    bobgui_tree_view_remove_column (BOBGUI_TREE_VIEW (sample_tree_view_bottom), column);

  /*  bobgui_list_store_remove (BOBGUI_LIST_STORE (bobgui_tree_view_get_model (BOBGUI_TREE_VIEW (data))), &iter);*/

  /* Put it back on the left */
  if (dest_iter)
    bobgui_list_store_insert_before (BOBGUI_LIST_STORE (left_tree_model),
				  &iter, dest_iter);
  else
    bobgui_list_store_append (BOBGUI_LIST_STORE (left_tree_model), &iter);
  
  bobgui_list_store_set (BOBGUI_LIST_STORE (left_tree_model), &iter, 0, label, 1, column, -1);
  selection = bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (left_tree_view));
  bobgui_tree_selection_select_iter (selection, &iter);

  g_free (label);
}

static void
move_to_right (BobguiTreeIter  *src_iter,
	       BobguiTreeModel *dest,
	       BobguiTreeIter  *dest_iter)
{
  char *label;
  BobguiTreeViewColumn *column;
  int before = -1;

  bobgui_tree_model_get (BOBGUI_TREE_MODEL (left_tree_model),
		      src_iter, 0, &label, 1, &column, -1);
  bobgui_list_store_remove (BOBGUI_LIST_STORE (left_tree_model), src_iter);

  if (dest_iter)
    {
      BobguiTreePath *path = bobgui_tree_model_get_path (dest, dest_iter);
      before = (bobgui_tree_path_get_indices (path))[0];
      bobgui_tree_path_free (path);
    }
  
  if (dest == top_right_tree_model)
    bobgui_tree_view_insert_column (BOBGUI_TREE_VIEW (sample_tree_view_top), column, before);
  else
    bobgui_tree_view_insert_column (BOBGUI_TREE_VIEW (sample_tree_view_bottom), column, before);

  g_free (label);
}

static void
move_up_or_down (BobguiTreeModel *src,
		 BobguiTreeIter  *src_iter,
		 BobguiTreeModel *dest,
		 BobguiTreeIter  *dest_iter)
{
  BobguiTreeViewColumn *column;
  char *label;
  int before = -1;
  
  bobgui_tree_model_get (src, src_iter, 0, &label, 1, &column, -1);

  if (dest_iter)
    {
      BobguiTreePath *path = bobgui_tree_model_get_path (dest, dest_iter);
      before = (bobgui_tree_path_get_indices (path))[0];
      bobgui_tree_path_free (path);
    }
  
  if (src == top_right_tree_model)
    bobgui_tree_view_remove_column (BOBGUI_TREE_VIEW (sample_tree_view_top), column);
  else
    bobgui_tree_view_remove_column (BOBGUI_TREE_VIEW (sample_tree_view_bottom), column);

  if (dest == top_right_tree_model)
    bobgui_tree_view_insert_column (BOBGUI_TREE_VIEW (sample_tree_view_top), column, before);
  else
    bobgui_tree_view_insert_column (BOBGUI_TREE_VIEW (sample_tree_view_bottom), column, before);

  g_free (label);
}

static void
move_row  (BobguiTreeModel *src,
	   BobguiTreeIter  *src_iter,
	   BobguiTreeModel *dest,
	   BobguiTreeIter  *dest_iter)
{
  if (src == left_tree_model)
    move_to_right (src_iter, dest, dest_iter);
  else if (dest == left_tree_model)
    move_to_left (src, src_iter, dest_iter);
  else 
    move_up_or_down (src, src_iter, dest, dest_iter);
}

static void
add_left_clicked (BobguiWidget *button,
		  gpointer data)
{
  BobguiTreeIter iter;

  BobguiTreeSelection *selection = bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (data));

  bobgui_tree_selection_get_selected (selection, NULL, &iter);

  move_to_left (bobgui_tree_view_get_model (BOBGUI_TREE_VIEW (data)), &iter, NULL);
}

static void
add_right_clicked (BobguiWidget *button, gpointer data)
{
  BobguiTreeIter iter;

  BobguiTreeSelection *selection = bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (left_tree_view));

  bobgui_tree_selection_get_selected (selection, NULL, &iter);

  move_to_right (&iter, bobgui_tree_view_get_model (BOBGUI_TREE_VIEW (data)), NULL);
}

static void
selection_changed (BobguiTreeSelection *selection, BobguiWidget *button)
{
  if (bobgui_tree_selection_get_selected (selection, NULL, NULL))
    bobgui_widget_set_sensitive (button, TRUE);
  else
    bobgui_widget_set_sensitive (button, FALSE);
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *hbox, *vbox;
  BobguiWidget *vbox2, *bbox;
  BobguiWidget *button;
  BobguiTreeViewColumn *column;
  BobguiCellRenderer *cell;
  BobguiWidget *swindow;
  BobguiTreeModel *sample_model;
  GdkContentFormats *targets;
  int i;
  gboolean done = FALSE;

  bobgui_init ();

  /* First initialize all the models for signal purposes */
  left_tree_model = (BobguiTreeModel *) bobgui_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
  sample_model = (BobguiTreeModel *) bobgui_list_store_new (1, G_TYPE_STRING);
  sample_tree_view_top = bobgui_tree_view_new_with_model (sample_model);
  sample_tree_view_bottom = bobgui_tree_view_new_with_model (sample_model);
  top_right_tree_model = (BobguiTreeModel *) view_column_model_new (BOBGUI_TREE_VIEW (sample_tree_view_top));
  bottom_right_tree_model = (BobguiTreeModel *) view_column_model_new (BOBGUI_TREE_VIEW (sample_tree_view_bottom));
  top_right_tree_view = bobgui_tree_view_new_with_model (top_right_tree_model);
  bottom_right_tree_view = bobgui_tree_view_new_with_model (bottom_right_tree_model);

  for (i = 0; i < 10; i++)
    {
      BobguiTreeIter iter;
      char *string = g_strdup_printf ("%d", i);
      bobgui_list_store_append (BOBGUI_LIST_STORE (sample_model), &iter);
      bobgui_list_store_set (BOBGUI_LIST_STORE (sample_model), &iter, 0, string, -1);
      g_free (string);
    }

  /* Set up the test windows. */
  window = bobgui_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done); 
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 300);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Top Window");
  swindow = bobgui_scrolled_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), swindow);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (swindow), sample_tree_view_top);
  bobgui_window_present (BOBGUI_WINDOW (window));

  window = bobgui_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done); 
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 300);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Bottom Window");
  swindow = bobgui_scrolled_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), swindow);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (swindow), sample_tree_view_bottom);
  bobgui_window_present (BOBGUI_WINDOW (window));

  /* Set up the main window */
  window = bobgui_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 500, 300);
  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 8);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  /* Left Pane */
  cell = bobgui_cell_renderer_text_new ();

  swindow = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (swindow), BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);
  left_tree_view = bobgui_tree_view_new_with_model (left_tree_model);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (swindow), left_tree_view);
  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (left_tree_view), -1,
					       "Unattached Columns", cell, "text", 0, NULL);
  cell = bobgui_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (set_visible), left_tree_view);
  column = bobgui_tree_view_column_new_with_attributes ("Visible", cell, NULL);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (left_tree_view), column);

  bobgui_tree_view_column_set_cell_data_func (column, cell, get_visible, NULL, NULL);
  bobgui_box_append (BOBGUI_BOX (hbox), swindow);

  /* Middle Pane */
  vbox2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
  bobgui_box_append (BOBGUI_BOX (hbox), vbox2);

  bbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_box_append (BOBGUI_BOX (vbox2), bbox);

  button = bobgui_button_new_with_mnemonic ("<< (_Q)");
  bobgui_widget_set_sensitive (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (add_left_clicked), top_right_tree_view);
  g_signal_connect (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (top_right_tree_view)),
                    "changed", G_CALLBACK (selection_changed), button);
  bobgui_box_append (BOBGUI_BOX (bbox), button);

  button = bobgui_button_new_with_mnemonic (">> (_W)");
  bobgui_widget_set_sensitive (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (add_right_clicked), top_right_tree_view);
  g_signal_connect (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (left_tree_view)),
                    "changed", G_CALLBACK (selection_changed), button);
  bobgui_box_append (BOBGUI_BOX (bbox), button);

  bbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_box_append (BOBGUI_BOX (vbox2), bbox);

  button = bobgui_button_new_with_mnemonic ("<< (_E)");
  bobgui_widget_set_sensitive (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (add_left_clicked), bottom_right_tree_view);
  g_signal_connect (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (bottom_right_tree_view)),
                    "changed", G_CALLBACK (selection_changed), button);
  bobgui_box_append (BOBGUI_BOX (bbox), button);

  button = bobgui_button_new_with_mnemonic (">> (_R)");
  bobgui_widget_set_sensitive (button, FALSE);
  g_signal_connect (button, "clicked", G_CALLBACK (add_right_clicked), bottom_right_tree_view);
  g_signal_connect (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (left_tree_view)),
                    "changed", G_CALLBACK (selection_changed), button);
  bobgui_box_append (BOBGUI_BOX (bbox), button);


  /* Right Pane */
  vbox2 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
  bobgui_box_append (BOBGUI_BOX (hbox), vbox2);

  swindow = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (swindow), BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);
  bobgui_tree_view_set_headers_visible (BOBGUI_TREE_VIEW (top_right_tree_view), FALSE);
  cell = bobgui_cell_renderer_text_new ();
  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (top_right_tree_view), -1,
					       NULL, cell, "text", 0, NULL);
  cell = bobgui_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (set_visible), top_right_tree_view);
  column = bobgui_tree_view_column_new_with_attributes (NULL, cell, NULL);
  bobgui_tree_view_column_set_cell_data_func (column, cell, get_visible, NULL, NULL);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (top_right_tree_view), column);

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (swindow), top_right_tree_view);
  bobgui_box_append (BOBGUI_BOX (vbox2), swindow);

  swindow = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (swindow), BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);
  bobgui_tree_view_set_headers_visible (BOBGUI_TREE_VIEW (bottom_right_tree_view), FALSE);
  cell = bobgui_cell_renderer_text_new ();
  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (bottom_right_tree_view), -1,
					       NULL, cell, "text", 0, NULL);
  cell = bobgui_cell_renderer_toggle_new ();
  g_signal_connect (cell, "toggled", G_CALLBACK (set_visible), bottom_right_tree_view);
  column = bobgui_tree_view_column_new_with_attributes (NULL, cell, NULL);
  bobgui_tree_view_column_set_cell_data_func (column, cell, get_visible, NULL, NULL);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (bottom_right_tree_view), column);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (swindow), bottom_right_tree_view);
  bobgui_box_append (BOBGUI_BOX (vbox2), swindow);


  /* Drag and Drop */
  targets = gdk_content_formats_new_for_gtype (BOBGUI_TYPE_TREE_ROW_DATA);
  bobgui_tree_view_enable_model_drag_source (BOBGUI_TREE_VIEW (left_tree_view),
					  GDK_BUTTON1_MASK,
                                          targets,
					  GDK_ACTION_MOVE);
  bobgui_tree_view_enable_model_drag_dest (BOBGUI_TREE_VIEW (left_tree_view),
					targets,
					GDK_ACTION_MOVE);

  bobgui_tree_view_enable_model_drag_source (BOBGUI_TREE_VIEW (top_right_tree_view),
					  GDK_BUTTON1_MASK,
					  targets,
					  GDK_ACTION_MOVE);
  bobgui_tree_view_enable_model_drag_dest (BOBGUI_TREE_VIEW (top_right_tree_view),
					targets,
					GDK_ACTION_MOVE);

  bobgui_tree_view_enable_model_drag_source (BOBGUI_TREE_VIEW (bottom_right_tree_view),
					  GDK_BUTTON1_MASK,
					  targets,
					  GDK_ACTION_MOVE);
  bobgui_tree_view_enable_model_drag_dest (BOBGUI_TREE_VIEW (bottom_right_tree_view),
					targets,
					GDK_ACTION_MOVE);
  gdk_content_formats_unref (targets);

  bobgui_box_append (BOBGUI_BOX (vbox), bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL));

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 8);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);
  button = bobgui_button_new_with_mnemonic ("_Add new Column");
  g_signal_connect (button, "clicked", G_CALLBACK (add_clicked), left_tree_model);
  bobgui_box_append (BOBGUI_BOX (hbox), button);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
