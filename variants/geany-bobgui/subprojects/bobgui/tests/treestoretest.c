/* treestoretest.c
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
#include <stdlib.h>
#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

BobguiTreeStore *base_model;
static int node_count = 0;

static void
selection_changed (BobguiTreeSelection *selection,
		   BobguiWidget        *button)
{
  if (bobgui_tree_selection_get_selected (selection, NULL, NULL))
    bobgui_widget_set_sensitive (button, TRUE);
  else
    bobgui_widget_set_sensitive (button, FALSE);
}

static void
node_set (BobguiTreeIter *iter)
{
  int n;
  char *str;

  str = g_strdup_printf ("Row (<span color=\"red\">%d</span>)", node_count++);
  bobgui_tree_store_set (base_model, iter, 0, str, -1);
  g_free (str);

  n = g_random_int_range (10000,99999);
  if (n < 0)
    n *= -1;
  str = g_strdup_printf ("%d", n);

  bobgui_tree_store_set (base_model, iter, 1, str, -1);
  g_free (str);
}

static void
iter_remove (BobguiWidget *button, BobguiTreeView *tree_view)
{
  BobguiTreeIter selected;
  BobguiTreeModel *model;

  model = bobgui_tree_view_get_model (tree_view);

  if (bobgui_tree_selection_get_selected (bobgui_tree_view_get_selection (tree_view),
				       NULL,
				       &selected))
    {
      if (BOBGUI_IS_TREE_STORE (model))
	{
	  bobgui_tree_store_remove (BOBGUI_TREE_STORE (model), &selected);
	}
    }
}

static void
iter_insert (BobguiWidget *button, BobguiTreeView *tree_view)
{
  BobguiWidget *entry;
  BobguiTreeIter iter;
  BobguiTreeIter selected;
  BobguiTreeModel *model = bobgui_tree_view_get_model (tree_view);

  entry = g_object_get_data (G_OBJECT (button), "user_data");
  if (bobgui_tree_selection_get_selected (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      bobgui_tree_store_insert (BOBGUI_TREE_STORE (model),
			     &iter,
			     &selected,
			     atoi (bobgui_editable_get_text (BOBGUI_EDITABLE (entry))));
    }
  else
    {
      bobgui_tree_store_insert (BOBGUI_TREE_STORE (model),
			     &iter,
			     NULL,
			     atoi (bobgui_editable_get_text (BOBGUI_EDITABLE (entry))));
    }

  node_set (&iter);
}

static void
iter_change (BobguiWidget *button, BobguiTreeView *tree_view)
{
  BobguiWidget *entry;
  BobguiTreeIter selected;
  BobguiTreeModel *model = bobgui_tree_view_get_model (tree_view);

  entry = g_object_get_data (G_OBJECT (button), "user_data");
  if (bobgui_tree_selection_get_selected (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (tree_view)),
				       NULL, &selected))
    {
      bobgui_tree_store_set (BOBGUI_TREE_STORE (model),
			  &selected,
			  1,
			  bobgui_editable_get_text (BOBGUI_EDITABLE (entry)),
			  -1);
    }
}

static void
iter_insert_with_values (BobguiWidget *button, BobguiTreeView *tree_view)
{
  BobguiWidget *entry;
  BobguiTreeIter iter;
  BobguiTreeIter selected;
  BobguiTreeModel *model = bobgui_tree_view_get_model (tree_view);
  char *str1, *str2;

  entry = g_object_get_data (G_OBJECT (button), "user_data");
  str1 = g_strdup_printf ("Row (<span color=\"red\">%d</span>)", node_count++);
  str2 = g_strdup_printf ("%d", atoi (bobgui_editable_get_text (BOBGUI_EDITABLE (entry))));

  if (bobgui_tree_selection_get_selected (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model),
					 &iter,
					 &selected,
					 -1,
					 0, str1,
					 1, str2,
					 -1);
    }
  else
    {
      bobgui_tree_store_insert_with_values (BOBGUI_TREE_STORE (model),
					 &iter,
					 NULL,
					 -1,
					 0, str1,
					 1, str2,
					 -1);
    }

  g_free (str1);
  g_free (str2);
}

static void
iter_insert_before  (BobguiWidget *button, BobguiTreeView *tree_view)
{
  BobguiTreeIter iter;
  BobguiTreeIter selected;
  BobguiTreeModel *model = bobgui_tree_view_get_model (tree_view);

  if (bobgui_tree_selection_get_selected (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      bobgui_tree_store_insert_before (BOBGUI_TREE_STORE (model),
				    &iter,
				    NULL,
				    &selected);
    }
  else
    {
      bobgui_tree_store_insert_before (BOBGUI_TREE_STORE (model),
				    &iter,
				    NULL,
				    NULL);
    }

  node_set (&iter);
}

static void
iter_insert_after (BobguiWidget *button, BobguiTreeView *tree_view)
{
  BobguiTreeIter iter;
  BobguiTreeIter selected;
  BobguiTreeModel *model = bobgui_tree_view_get_model (tree_view);

  if (bobgui_tree_selection_get_selected (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      if (BOBGUI_IS_TREE_STORE (model))
	{
	  bobgui_tree_store_insert_after (BOBGUI_TREE_STORE (model),
				       &iter,
				       NULL,
				       &selected);
	  node_set (&iter);
	}
    }
  else
    {
      if (BOBGUI_IS_TREE_STORE (model))
	{
	  bobgui_tree_store_insert_after (BOBGUI_TREE_STORE (model),
				       &iter,
				       NULL,
				       NULL);
	  node_set (&iter);
	}
    }
}

static void
iter_prepend (BobguiWidget *button, BobguiTreeView *tree_view)
{
  BobguiTreeIter iter;
  BobguiTreeIter selected;
  BobguiTreeModel *model = bobgui_tree_view_get_model (tree_view);
  BobguiTreeSelection *selection = bobgui_tree_view_get_selection (tree_view);

  if (bobgui_tree_selection_get_selected (selection, NULL, &selected))
    {
      if (BOBGUI_IS_TREE_STORE (model))
	{
	  bobgui_tree_store_prepend (BOBGUI_TREE_STORE (model),
				  &iter,
				  &selected);
	  node_set (&iter);
	}
    }
  else
    {
      if (BOBGUI_IS_TREE_STORE (model))
	{
	  bobgui_tree_store_prepend (BOBGUI_TREE_STORE (model),
				  &iter,
				  NULL);
	  node_set (&iter);
	}
    }
}

static void
iter_append (BobguiWidget *button, BobguiTreeView *tree_view)
{
  BobguiTreeIter iter;
  BobguiTreeIter selected;
  BobguiTreeModel *model = bobgui_tree_view_get_model (tree_view);

  if (bobgui_tree_selection_get_selected (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (tree_view)),
				       NULL,
				       &selected))
    {
      if (BOBGUI_IS_TREE_STORE (model))
	{
	  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter, &selected);
	  node_set (&iter);
	}
    }
  else
    {
      if (BOBGUI_IS_TREE_STORE (model))
	{
	  bobgui_tree_store_append (BOBGUI_TREE_STORE (model), &iter, NULL);
	  node_set (&iter);
	}
    }
}

static gboolean done = FALSE;

static void
quit_cb (BobguiWidget *widget,
         gpointer   user_data)
{
  gboolean *is_done = user_data;

  *is_done = TRUE;

  g_main_context_wakeup (NULL);
}

static void
make_window (int view_type)
{
  BobguiWidget *window;
  BobguiWidget *vbox;
  BobguiWidget *hbox, *entry;
  BobguiWidget *button;
  BobguiWidget *scrolled_window;
  BobguiWidget *tree_view;
  BobguiTreeViewColumn *column;
  BobguiCellRenderer *cell;
  GObject *selection;

  /* Make the Widgets/Objects */
  window = bobgui_window_new ();
  switch (view_type)
    {
    case 0:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Unsorted list");
      break;
    case 1:
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Sorted list");
      break;
    default:
      g_assert_not_reached ();
    }

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 350);
  scrolled_window = bobgui_scrolled_window_new ();
  switch (view_type)
    {
    case 0:
      tree_view = bobgui_tree_view_new_with_model (BOBGUI_TREE_MODEL (base_model));
      break;
    case 1:
      {
	BobguiTreeModel *sort_model;
	
	sort_model = bobgui_tree_model_sort_new_with_model (BOBGUI_TREE_MODEL (base_model));
	tree_view = bobgui_tree_view_new_with_model (BOBGUI_TREE_MODEL (sort_model));
      }
      break;
    default:
      g_assert_not_reached ();
      tree_view = NULL; /* Quiet compiler */
      break;
    }

  selection = G_OBJECT (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (tree_view)));
  bobgui_tree_selection_set_mode (BOBGUI_TREE_SELECTION (selection), BOBGUI_SELECTION_SINGLE);

  /* Put them together */
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), tree_view);
  bobgui_widget_set_vexpand (scrolled_window, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), scrolled_window);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window),
				  BOBGUI_POLICY_AUTOMATIC,
				  BOBGUI_POLICY_AUTOMATIC);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  /* buttons */
  button = bobgui_button_new_with_label ("bobgui_tree_store_remove");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (selection_changed),
                    button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (iter_remove),
                    tree_view);
  bobgui_widget_set_sensitive (button, FALSE);

  button = bobgui_button_new_with_label ("bobgui_tree_store_insert");
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 8);
  entry = bobgui_entry_new ();
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  bobgui_box_append (BOBGUI_BOX (hbox), entry);
  g_object_set_data (G_OBJECT (button), "user_data", entry);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (iter_insert),
                    tree_view);

  button = bobgui_button_new_with_label ("bobgui_tree_store_set");
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 8);
  entry = bobgui_entry_new ();
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  bobgui_box_append (BOBGUI_BOX (hbox), entry);
  g_object_set_data (G_OBJECT (button), "user_data", entry);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (iter_change),
		    tree_view);

  button = bobgui_button_new_with_label ("bobgui_tree_store_insert_with_values");
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 8);
  entry = bobgui_entry_new ();
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  bobgui_box_append (BOBGUI_BOX (hbox), entry);
  g_object_set_data (G_OBJECT (button), "user_data", entry);
  g_signal_connect (button, "clicked",
		    G_CALLBACK (iter_insert_with_values),
		    tree_view);

  button = bobgui_button_new_with_label ("bobgui_tree_store_insert_before");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (iter_insert_before),
                    tree_view);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (selection_changed),
                    button);
  bobgui_widget_set_sensitive (button, FALSE);

  button = bobgui_button_new_with_label ("bobgui_tree_store_insert_after");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (iter_insert_after),
                    tree_view);
  g_signal_connect (selection, "changed",
                    G_CALLBACK (selection_changed),
                    button);
  bobgui_widget_set_sensitive (button, FALSE);

  button = bobgui_button_new_with_label ("bobgui_tree_store_prepend");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (iter_prepend),
                    tree_view);

  button = bobgui_button_new_with_label ("bobgui_tree_store_append");
  bobgui_box_append (BOBGUI_BOX (vbox), button);
  g_signal_connect (button, "clicked",
                    G_CALLBACK (iter_append),
                    tree_view);

  /* The selected column */
  cell = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("Node ID", cell, "markup", 0, NULL);
  bobgui_tree_view_column_set_sort_column_id (column, 0);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), column);

  cell = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("Random Number", cell, "text", 1, NULL);
  bobgui_tree_view_column_set_sort_column_id (column, 1);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), column);

  /* A few to start */
  if (view_type == 0)
    {
      iter_append (NULL, BOBGUI_TREE_VIEW (tree_view));
      iter_append (NULL, BOBGUI_TREE_VIEW (tree_view));
      iter_append (NULL, BOBGUI_TREE_VIEW (tree_view));
      iter_append (NULL, BOBGUI_TREE_VIEW (tree_view));
      iter_append (NULL, BOBGUI_TREE_VIEW (tree_view));
      iter_append (NULL, BOBGUI_TREE_VIEW (tree_view));
    }
  /* Show it all */
  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  bobgui_init ();

  base_model = bobgui_tree_store_new (2, G_TYPE_STRING, G_TYPE_STRING);

  /* FIXME: reverse this */
  make_window (0);
  make_window (1);

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}

