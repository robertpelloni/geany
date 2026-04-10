/* testtreeflow.c
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

BobguiTreeModel *model = NULL;
static GRand *grand = NULL;
BobguiTreeSelection *selection = NULL;
enum
{
  TEXT_COLUMN,
  NUM_COLUMNS
};

static const char *words[] =
{
  "Boom",
  "Borp",
  "Multiline\ntext",
  "Bingo",
  "Veni\nVedi\nVici",
  NULL
};


#define NUM_WORDS 5
#define NUM_ROWS 100


static void
initialize_model (void)
{
  int i;
  BobguiTreeIter iter;

  model = (BobguiTreeModel *) bobgui_list_store_new (NUM_COLUMNS, G_TYPE_STRING);
  grand = g_rand_new ();
  for (i = 0; i < NUM_ROWS; i++)
    {
      bobgui_list_store_append (BOBGUI_LIST_STORE (model), &iter);
      bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter,
			  TEXT_COLUMN, words[g_rand_int_range (grand, 0, NUM_WORDS)],
			  -1);
    }
}

static void
futz_row (void)
{
  int i;
  BobguiTreePath *path;
  BobguiTreeIter iter;
  BobguiTreeIter iter2;

  i = g_rand_int_range (grand, 0,
			bobgui_tree_model_iter_n_children (model, NULL));
  path = bobgui_tree_path_new ();
  bobgui_tree_path_append_index (path, i);
  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_tree_path_free (path);

  if (bobgui_tree_selection_iter_is_selected (selection, &iter))
    return;
  switch (g_rand_int_range (grand, 0, 3))
    {
    case 0:
      /* insert */
            bobgui_list_store_insert_after (BOBGUI_LIST_STORE (model),
            				   &iter2, &iter);
            bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter2,
            			  TEXT_COLUMN, words[g_rand_int_range (grand, 0, NUM_WORDS)],
            			  -1);
      break;
    case 1:
      /* delete */
      if (bobgui_tree_model_iter_n_children (model, NULL) == 0)
	return;
      bobgui_list_store_remove (BOBGUI_LIST_STORE (model), &iter);
      break;
    case 2:
      /* modify */
      return;
      if (bobgui_tree_model_iter_n_children (model, NULL) == 0)
	return;
      bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter,
      			  TEXT_COLUMN, words[g_rand_int_range (grand, 0, NUM_WORDS)],
      			  -1);
      break;
    default:
      g_assert_not_reached ();
    }
}

static gboolean
futz (void)
{
  int i;

  for (i = 0; i < 15; i++)
    futz_row ();
  g_print ("Number of rows: %d\n", bobgui_tree_model_iter_n_children (model, NULL));
  return TRUE;
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
  BobguiWidget *vbox;
  BobguiWidget *scrolled_window;
  BobguiWidget *tree_view;
  BobguiWidget *hbox;
  BobguiWidget *button;
  BobguiTreePath *path;
  gboolean done = FALSE;

  bobgui_init ();

  path = bobgui_tree_path_new_from_string ("80");
  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Reflow test");
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
  bobgui_box_append (BOBGUI_BOX (vbox), bobgui_label_new ("Incremental Reflow Test"));
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);
  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window),
				  BOBGUI_POLICY_AUTOMATIC,
				  BOBGUI_POLICY_AUTOMATIC);
  bobgui_widget_set_vexpand (scrolled_window, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), scrolled_window);

  initialize_model ();
  tree_view = bobgui_tree_view_new_with_model (model);
  bobgui_tree_view_scroll_to_cell (BOBGUI_TREE_VIEW (tree_view), path, NULL, TRUE, 0.5, 0.0);
  selection = bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (tree_view));
  bobgui_tree_selection_select_path (selection, path);
  bobgui_tree_view_set_headers_visible (BOBGUI_TREE_VIEW (tree_view), FALSE);
  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1,
					       NULL,
					       bobgui_cell_renderer_text_new (),
					       "text", TEXT_COLUMN,
					       NULL);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), tree_view);
  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);
  button = bobgui_button_new_with_mnemonic ("<b>_Futz!!</b>");
  bobgui_box_append (BOBGUI_BOX (hbox), button);
  bobgui_label_set_use_markup (BOBGUI_LABEL (bobgui_button_get_child (BOBGUI_BUTTON (button))), TRUE);
  g_signal_connect (button, "clicked", G_CALLBACK (futz), NULL);
  g_signal_connect (button, "realize", G_CALLBACK (bobgui_widget_grab_focus), NULL);
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 400);
  bobgui_window_present (BOBGUI_WINDOW (window));
  g_timeout_add (1000, (GSourceFunc) futz, NULL);
  while (!done)
    g_main_context_iteration (NULL, TRUE);
  return 0;
}
