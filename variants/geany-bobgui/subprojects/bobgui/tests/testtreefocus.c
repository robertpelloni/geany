/* testtreefocus.c
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

typedef struct _TreeStruct TreeStruct;
struct _TreeStruct
{
  const char *label;
  gboolean alex;
  gboolean havoc;
  gboolean tim;
  gboolean owen;
  gboolean dave;
  gboolean world_holiday; /* shared by the european hackers */
  TreeStruct *children;
};


static TreeStruct january[] =
{
  {"New Years Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  {"Presidential Inauguration", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  {"Martin Luther King Jr. day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct february[] =
{
  { "Presidents' Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { "Groundhog Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Valentine's Day", FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, NULL },
  { NULL }
};

static TreeStruct march[] =
{
  { "National Tree Planting Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "St Patrick's Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeStruct april[] =
{
  { "April Fools' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Army Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Earth Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Administrative Professionals' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct may[] =
{
  { "Nurses' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "National Day of Prayer", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Mothers' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Armed Forces Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Memorial Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeStruct june[] =
{
  { "June Fathers' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Juneteenth (Liberation Day)", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Flag Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct july[] =
{
  { "Parents' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Independence Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct august[] =
{
  { "Air Force Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Coast Guard Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Friendship Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct september[] =
{
  { "Grandparents' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Citizenship Day or Constitution Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Labor Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeStruct october[] =
{
  { "National Children's Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Bosses' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Sweetest Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Mother-in-Law's Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Navy Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Columbus Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { "Halloween", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeStruct november[] =
{
  { "Marine Corps Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Veterans' Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { "Thanksgiving", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeStruct december[] =
{
  { "Pearl Harbor Remembrance Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Christmas", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { "Kwanzaa", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};


static TreeStruct toplevel[] =
{
  {"January", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, january},
  {"February", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, february},
  {"March", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, march},
  {"April", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, april},
  {"May", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, may},
  {"June", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, june},
  {"July", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, july},
  {"August", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, august},
  {"September", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, september},
  {"October", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, october},
  {"November", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, november},
  {"December", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, december},
  {NULL}
};


enum
{
  HOLIDAY_COLUMN = 0,
  ALEX_COLUMN,
  HAVOC_COLUMN,
  TIM_COLUMN,
  OWEN_COLUMN,
  DAVE_COLUMN,
  VISIBLE_COLUMN,
  WORLD_COLUMN,
  NUM_COLUMNS
};

static BobguiTreeModel *
make_model (void)
{
  BobguiTreeStore *model;
  TreeStruct *month = toplevel;
  BobguiTreeIter iter;

  model = bobgui_tree_store_new (NUM_COLUMNS,
			      G_TYPE_STRING,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN);

  while (month->label)
    {
      TreeStruct *holiday = month->children;

      bobgui_tree_store_append (model, &iter, NULL);
      bobgui_tree_store_set (model, &iter,
			  HOLIDAY_COLUMN, month->label,
			  ALEX_COLUMN, FALSE,
			  HAVOC_COLUMN, FALSE,
			  TIM_COLUMN, FALSE,
			  OWEN_COLUMN, FALSE,
			  DAVE_COLUMN, FALSE,
			  VISIBLE_COLUMN, FALSE,
			  WORLD_COLUMN, FALSE,
			  -1);
      while (holiday->label)
	{
	  BobguiTreeIter child_iter;

	  bobgui_tree_store_append (model, &child_iter, &iter);
	  bobgui_tree_store_set (model, &child_iter,
			      HOLIDAY_COLUMN, holiday->label,
			      ALEX_COLUMN, holiday->alex,
			      HAVOC_COLUMN, holiday->havoc,
			      TIM_COLUMN, holiday->tim,
			      OWEN_COLUMN, holiday->owen,
			      DAVE_COLUMN, holiday->dave,
			      VISIBLE_COLUMN, TRUE,
			      WORLD_COLUMN, holiday->world_holiday,
			      -1);

	  holiday ++;
	}
      month ++;
    }

  return BOBGUI_TREE_MODEL (model);
}

static void
alex_toggled (BobguiCellRendererToggle *cell,
	      char                  *path_str,
	      gpointer               data)
{
  BobguiTreeModel *model = (BobguiTreeModel *) data;
  BobguiTreeIter iter;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_str);
  gboolean alex;

  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_tree_model_get (model, &iter, ALEX_COLUMN, &alex, -1);

  alex = !alex;
  bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &iter, ALEX_COLUMN, alex, -1);

  bobgui_tree_path_free (path);
}

static void
havoc_toggled (BobguiCellRendererToggle *cell,
	       char                  *path_str,
	       gpointer               data)
{
  BobguiTreeModel *model = (BobguiTreeModel *) data;
  BobguiTreeIter iter;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_str);
  gboolean havoc;

  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_tree_model_get (model, &iter, HAVOC_COLUMN, &havoc, -1);

  havoc = !havoc;
  bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &iter, HAVOC_COLUMN, havoc, -1);

  bobgui_tree_path_free (path);
}

static void
owen_toggled (BobguiCellRendererToggle *cell,
	      char                  *path_str,
	      gpointer               data)
{
  BobguiTreeModel *model = (BobguiTreeModel *) data;
  BobguiTreeIter iter;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_str);
  gboolean owen;

  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_tree_model_get (model, &iter, OWEN_COLUMN, &owen, -1);

  owen = !owen;
  bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &iter, OWEN_COLUMN, owen, -1);

  bobgui_tree_path_free (path);
}

static void
tim_toggled (BobguiCellRendererToggle *cell,
	     char                  *path_str,
	     gpointer               data)
{
  BobguiTreeModel *model = (BobguiTreeModel *) data;
  BobguiTreeIter iter;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_str);
  gboolean tim;

  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_tree_model_get (model, &iter, TIM_COLUMN, &tim, -1);

  tim = !tim;
  bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &iter, TIM_COLUMN, tim, -1);

  bobgui_tree_path_free (path);
}

static void
dave_toggled (BobguiCellRendererToggle *cell,
	      char                  *path_str,
	      gpointer               data)
{
  BobguiTreeModel *model = (BobguiTreeModel *) data;
  BobguiTreeIter iter;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_str);
  gboolean dave;

  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_tree_model_get (model, &iter, DAVE_COLUMN, &dave, -1);

  dave = !dave;
  bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &iter, DAVE_COLUMN, dave, -1);

  bobgui_tree_path_free (path);
}

static void
set_indicator_size (BobguiTreeViewColumn *column,
		    BobguiCellRenderer *cell,
		    BobguiTreeModel *model,
		    BobguiTreeIter *iter,
		    gpointer data)
{
  int size;
  BobguiTreePath *path;

  path = bobgui_tree_model_get_path (model, iter);
  size = bobgui_tree_path_get_indices (path)[0]  * 2 + 10;
  bobgui_tree_path_free (path);

  g_object_set (cell, "indicator_size", size, NULL);
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
  BobguiTreeModel *model;
  BobguiCellRenderer *renderer;
  int col_offset;
  BobguiTreeViewColumn *column;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Card planning sheet");
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
  bobgui_box_append (BOBGUI_BOX (vbox), bobgui_label_new ("Jonathan's Holiday Card Planning Sheet"));
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (scrolled_window), TRUE);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window), BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);
  bobgui_widget_set_vexpand (scrolled_window, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), scrolled_window);

  model = make_model ();
  tree_view = bobgui_tree_view_new_with_model (model);
  bobgui_tree_selection_set_mode (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (tree_view)),
			       BOBGUI_SELECTION_MULTIPLE);
  renderer = bobgui_cell_renderer_text_new ();
  col_offset = bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
							    -1, "Holiday",
							    renderer,
							    "text", HOLIDAY_COLUMN, NULL);
  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (tree_view), col_offset - 1);
  bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (column), TRUE);

  /* Alex Column */
  renderer = bobgui_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (alex_toggled), model);

  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
							    -1, "Alex",
							    renderer,
							    "active", ALEX_COLUMN,
							    "visible", VISIBLE_COLUMN,
							    "activatable", WORLD_COLUMN,
							    NULL);
  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (tree_view), col_offset - 1);
  bobgui_tree_view_column_set_sizing (BOBGUI_TREE_VIEW_COLUMN (column), BOBGUI_TREE_VIEW_COLUMN_FIXED);
  bobgui_tree_view_column_set_fixed_width (BOBGUI_TREE_VIEW_COLUMN (column), 50);
  bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (column), TRUE);

  /* Havoc Column */
  renderer = bobgui_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (havoc_toggled), model);

  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
							    -1, "Havoc",
							    renderer,
							    "active", HAVOC_COLUMN,
							    "visible", VISIBLE_COLUMN,
							    NULL);
  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (tree_view), col_offset - 1);
  bobgui_tree_view_column_set_sizing (BOBGUI_TREE_VIEW_COLUMN (column), BOBGUI_TREE_VIEW_COLUMN_FIXED);
  bobgui_tree_view_column_set_fixed_width (BOBGUI_TREE_VIEW_COLUMN (column), 50);
  bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (column), TRUE);

  /* Tim Column */
  renderer = bobgui_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (tim_toggled), model);

  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Tim",
					       renderer,
					       "active", TIM_COLUMN,
					       "visible", VISIBLE_COLUMN,
					       "activatable", WORLD_COLUMN,
					       NULL);
  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (tree_view), col_offset - 1);
  bobgui_tree_view_column_set_sizing (BOBGUI_TREE_VIEW_COLUMN (column), BOBGUI_TREE_VIEW_COLUMN_FIXED);
  bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (column), TRUE);
  bobgui_tree_view_column_set_fixed_width (BOBGUI_TREE_VIEW_COLUMN (column), 50);

  /* Owen Column */
  renderer = bobgui_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (owen_toggled), model);
  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Owen",
					       renderer,
					       "active", OWEN_COLUMN,
					       "visible", VISIBLE_COLUMN,
					       NULL);
  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (tree_view), col_offset - 1);
  bobgui_tree_view_column_set_sizing (BOBGUI_TREE_VIEW_COLUMN (column), BOBGUI_TREE_VIEW_COLUMN_FIXED);
  bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (column), TRUE);
  bobgui_tree_view_column_set_fixed_width (BOBGUI_TREE_VIEW_COLUMN (column), 50);

  /* Owen Column */
  renderer = bobgui_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled", G_CALLBACK (dave_toggled), model);
  g_object_set (renderer, "xalign", 0.0, NULL);
  col_offset = bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Dave",
					       renderer,
					       "active", DAVE_COLUMN,
					       "visible", VISIBLE_COLUMN,
					       NULL);
  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (tree_view), col_offset - 1);
  bobgui_tree_view_column_set_cell_data_func (column, renderer, set_indicator_size, NULL, NULL);
  bobgui_tree_view_column_set_sizing (BOBGUI_TREE_VIEW_COLUMN (column), BOBGUI_TREE_VIEW_COLUMN_FIXED);
  bobgui_tree_view_column_set_fixed_width (BOBGUI_TREE_VIEW_COLUMN (column), 50);
  bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (column), TRUE);

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), tree_view);

  g_signal_connect (tree_view, "realize",
		    G_CALLBACK (bobgui_tree_view_expand_all),
		    NULL);
  bobgui_window_set_default_size (BOBGUI_WINDOW (window),
			       650, 400);
  bobgui_window_present (BOBGUI_WINDOW (window));

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Model");
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
  bobgui_box_append (BOBGUI_BOX (vbox), bobgui_label_new ("The model revealed"));
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (scrolled_window), TRUE);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window), BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);
  bobgui_widget_set_vexpand (scrolled_window, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), scrolled_window);


  tree_view = bobgui_tree_view_new_with_model (model);
  g_object_unref (model);

  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Holiday Column",
					       bobgui_cell_renderer_text_new (),
					       "text", 0, NULL);

  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Alex Column",
					       bobgui_cell_renderer_text_new (),
					       "text", 1, NULL);

  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Havoc Column",
					       bobgui_cell_renderer_text_new (),
					       "text", 2, NULL);

  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Tim Column",
					       bobgui_cell_renderer_text_new (),
					       "text", 3, NULL);

  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Owen Column",
					       bobgui_cell_renderer_text_new (),
					       "text", 4, NULL);

  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Dave Column",
					       bobgui_cell_renderer_text_new (),
					       "text", 5, NULL);

  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Visible Column",
					       bobgui_cell_renderer_text_new (),
					       "text", 6, NULL);

  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "World Holiday",
					       bobgui_cell_renderer_text_new (),
					       "text", 7, NULL);

  g_signal_connect (tree_view, "realize",
		    G_CALLBACK (bobgui_tree_view_expand_all),
		    NULL);
			   
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), tree_view);

  bobgui_window_set_default_size (BOBGUI_WINDOW (window),
			       650, 400);

  bobgui_window_present (BOBGUI_WINDOW (window));
  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}

