/* testtreeedit.c
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

typedef struct {
  const char *string;
  gboolean is_editable;
  gboolean is_sensitive;
  int progress;
} ListEntry;

enum {
  STRING_COLUMN,
  IS_EDITABLE_COLUMN,
  IS_SENSITIVE_COLUMN,
  ICON_NAME_COLUMN,
  LAST_ICON_NAME_COLUMN,
  PROGRESS_COLUMN,
  NUM_COLUMNS
};

static ListEntry model_strings[] =
{
  {"A simple string", TRUE, TRUE, 0 },
  {"Another string!", TRUE, TRUE, 10 },
  {"", TRUE, TRUE, 0 },
  {"Guess what, a third string. This one can't be edited", FALSE, TRUE, 47 },
  {"And then a fourth string. Neither can this", FALSE, TRUE, 48 },
  {"Multiline\nFun!", TRUE, FALSE, 75 },
  { NULL }
};

static BobguiTreeModel *
create_model (void)
{
  BobguiTreeStore *model;
  BobguiTreeIter iter;
  int i;

  model = bobgui_tree_store_new (NUM_COLUMNS,
			      G_TYPE_STRING,
			      G_TYPE_BOOLEAN,
			      G_TYPE_BOOLEAN,
			      G_TYPE_STRING,
			      G_TYPE_STRING,
			      G_TYPE_INT);

  for (i = 0; model_strings[i].string != NULL; i++)
    {
      bobgui_tree_store_append (model, &iter, NULL);

      bobgui_tree_store_set (model, &iter,
			  STRING_COLUMN, model_strings[i].string,
			  IS_EDITABLE_COLUMN, model_strings[i].is_editable,
			  IS_SENSITIVE_COLUMN, model_strings[i].is_sensitive,
			  ICON_NAME_COLUMN, "document-new",
			  LAST_ICON_NAME_COLUMN, "edit-delete",
			  PROGRESS_COLUMN, model_strings[i].progress,
			  -1);
    }
  
  return BOBGUI_TREE_MODEL (model);
}

static void
editable_toggled (BobguiCellRendererToggle *cell,
		  char                  *path_string,
		  gpointer               data)
{
  BobguiTreeModel *model = BOBGUI_TREE_MODEL (data);
  BobguiTreeIter iter;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_string);
  gboolean value;

  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_tree_model_get (model, &iter, IS_EDITABLE_COLUMN, &value, -1);

  value = !value;
  bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &iter, IS_EDITABLE_COLUMN, value, -1);

  bobgui_tree_path_free (path);
}

static void
sensitive_toggled (BobguiCellRendererToggle *cell,
		   char                  *path_string,
		   gpointer               data)
{
  BobguiTreeModel *model = BOBGUI_TREE_MODEL (data);
  BobguiTreeIter iter;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_string);
  gboolean value;

  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_tree_model_get (model, &iter, IS_SENSITIVE_COLUMN, &value, -1);

  value = !value;
  bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &iter, IS_SENSITIVE_COLUMN, value, -1);

  bobgui_tree_path_free (path);
}

static void
edited (BobguiCellRendererText *cell,
	char                *path_string,
	char                *new_text,
	gpointer             data)
{
  BobguiTreeModel *model = BOBGUI_TREE_MODEL (data);
  BobguiTreeIter iter;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_string);

  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &iter, STRING_COLUMN, new_text, -1);

  bobgui_tree_path_free (path);
}

static void
pressed_cb (BobguiGesture *gesture,
            int         n_press,
            double      x,
            double      y,
            BobguiWidget  *widget)
{
  /* Deselect if people click outside any row. */
  if (!bobgui_tree_view_get_path_at_pos (BOBGUI_TREE_VIEW (widget), x, y, NULL, NULL, NULL, NULL))
    bobgui_tree_selection_unselect_all (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (widget)));
}

typedef struct {
  BobguiCellArea     *area;
  BobguiCellRenderer *renderer;
} CallbackData;

static void
align_cell_toggled (BobguiToggleButton  *toggle,
		    CallbackData     *data)
{
  gboolean active = bobgui_toggle_button_get_active (toggle);

  bobgui_cell_area_cell_set (data->area, data->renderer, "align", active, NULL);
}

static void
expand_cell_toggled (BobguiToggleButton  *toggle,
		     CallbackData     *data)
{
  gboolean active = bobgui_toggle_button_get_active (toggle);

  bobgui_cell_area_cell_set (data->area, data->renderer, "expand", active, NULL);
}

static void
fixed_cell_toggled (BobguiToggleButton  *toggle,
		    CallbackData     *data)
{
  gboolean active = bobgui_toggle_button_get_active (toggle);

  bobgui_cell_area_cell_set (data->area, data->renderer, "fixed-size", active, NULL);
}

enum {
  CNTL_EXPAND,
  CNTL_ALIGN,
  CNTL_FIXED
};

static void
create_control (BobguiWidget *box, int number, int cntl, CallbackData *data)
{
  BobguiWidget *checkbutton;
  GCallback  callback = NULL;
  char *name = NULL;

  switch (cntl)
    {
    case CNTL_EXPAND: 
      name = g_strdup_printf ("Expand Cell #%d", number); 
      callback = G_CALLBACK (expand_cell_toggled);
      break;
    case CNTL_ALIGN: 
      name = g_strdup_printf ("Align Cell #%d", number); 
      callback = G_CALLBACK (align_cell_toggled);
      break;
    case CNTL_FIXED: 
      name = g_strdup_printf ("Fix size Cell #%d", number); 
      callback = G_CALLBACK (fixed_cell_toggled);
      break;
    default:
      g_assert_not_reached ();
    }

  checkbutton = bobgui_check_button_new_with_label (name);
  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (checkbutton), cntl == CNTL_FIXED);
  bobgui_box_append (BOBGUI_BOX (box), checkbutton);

  g_signal_connect (G_OBJECT (checkbutton), "toggled", callback, data);
  g_free (name);
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
main (int argc, char **argv)
{
  BobguiWidget *window;
  BobguiWidget *scrolled_window;
  BobguiWidget *tree_view;
  BobguiWidget *vbox, *hbox, *cntl_vbox;
  BobguiTreeModel *tree_model;
  BobguiCellRenderer *renderer;
  BobguiTreeViewColumn *column;
  BobguiCellArea *area;
  CallbackData callback[4];
  BobguiGesture *gesture;
  gboolean done = FALSE;

  bobgui_init ();

  if (g_getenv ("RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "BobguiTreeView editing sample");
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 6);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (scrolled_window), TRUE);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window),
				  BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);
  bobgui_widget_set_vexpand (scrolled_window, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), scrolled_window);

  tree_model = create_model ();
  tree_view = bobgui_tree_view_new_with_model (tree_model);
  gesture = bobgui_gesture_click_new ();
  g_signal_connect (gesture, "pressed", G_CALLBACK (pressed_cb), tree_view);
  bobgui_widget_add_controller (tree_view, BOBGUI_EVENT_CONTROLLER (gesture));
  bobgui_tree_view_set_headers_visible (BOBGUI_TREE_VIEW (tree_view), TRUE);

  column = bobgui_tree_view_column_new ();
  bobgui_tree_view_column_set_title (column, "String");
  area = bobgui_cell_layout_get_area (BOBGUI_CELL_LAYOUT (column));

  renderer = bobgui_cell_renderer_pixbuf_new ();
  bobgui_tree_view_column_pack_start (column, renderer, FALSE);
  bobgui_tree_view_column_set_attributes (column, renderer,
				       "icon-name", ICON_NAME_COLUMN, 
				       "sensitive", IS_SENSITIVE_COLUMN,
				       NULL);
  callback[0].area = area;
  callback[0].renderer = renderer;

  renderer = bobgui_cell_renderer_text_new ();
  bobgui_tree_view_column_pack_start (column, renderer, FALSE);
  bobgui_tree_view_column_set_attributes (column, renderer,
				       "text", STRING_COLUMN,
				       "editable", IS_EDITABLE_COLUMN,
				       "sensitive", IS_SENSITIVE_COLUMN,
				       NULL);
  callback[1].area = area;
  callback[1].renderer = renderer;
  g_signal_connect (renderer, "edited",
		    G_CALLBACK (edited), tree_model);
  g_object_set (renderer,
                "placeholder-text", "Type here",
                NULL);

  renderer = bobgui_cell_renderer_text_new ();
  bobgui_tree_view_column_pack_start (column, renderer, FALSE);
  bobgui_tree_view_column_set_attributes (column, renderer,
		  		       "text", STRING_COLUMN,
				       "editable", IS_EDITABLE_COLUMN,
				       "sensitive", IS_SENSITIVE_COLUMN,
				       NULL);
  callback[2].area = area;
  callback[2].renderer = renderer;
  g_signal_connect (renderer, "edited",
		    G_CALLBACK (edited), tree_model);
  g_object_set (renderer,
                "placeholder-text", "Type here too",
                NULL);

  renderer = bobgui_cell_renderer_pixbuf_new ();
  g_object_set (renderer,
		"xalign", 0.0,
		NULL);
  bobgui_tree_view_column_pack_start (column, renderer, FALSE);
  bobgui_tree_view_column_set_attributes (column, renderer,
				       "icon-name", LAST_ICON_NAME_COLUMN, 
				       "sensitive", IS_SENSITIVE_COLUMN,
				       NULL);
  callback[3].area = area;
  callback[3].renderer = renderer;

  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tree_view), column);

  renderer = bobgui_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled",
		    G_CALLBACK (editable_toggled), tree_model);
  
  g_object_set (renderer,
		"xalign", 0.0,
		NULL);
  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Editable",
					       renderer,
					       "active", IS_EDITABLE_COLUMN,
					       NULL);

  renderer = bobgui_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled",
		    G_CALLBACK (sensitive_toggled), tree_model);
  
  g_object_set (renderer,
		"xalign", 0.0,
		NULL);
  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Sensitive",
					       renderer,
					       "active", IS_SENSITIVE_COLUMN,
					       NULL);

  renderer = bobgui_cell_renderer_progress_new ();
  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
					       -1, "Progress",
					       renderer,
					       "value", PROGRESS_COLUMN,
					       NULL);

  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), tree_view);

  bobgui_window_set_default_size (BOBGUI_WINDOW (window),
			       800, 250);

  hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
  bobgui_box_append (BOBGUI_BOX (vbox), hbox);

  /* Alignment controls */
  cntl_vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 2);
  bobgui_box_append (BOBGUI_BOX (hbox), cntl_vbox);

  create_control (cntl_vbox, 1, CNTL_ALIGN, &callback[0]);
  create_control (cntl_vbox, 2, CNTL_ALIGN, &callback[1]);
  create_control (cntl_vbox, 3, CNTL_ALIGN, &callback[2]);
  create_control (cntl_vbox, 4, CNTL_ALIGN, &callback[3]);

  /* Expand controls */
  cntl_vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 2);
  bobgui_box_append (BOBGUI_BOX (hbox), cntl_vbox);

  create_control (cntl_vbox, 1, CNTL_EXPAND, &callback[0]);
  create_control (cntl_vbox, 2, CNTL_EXPAND, &callback[1]);
  create_control (cntl_vbox, 3, CNTL_EXPAND, &callback[2]);
  create_control (cntl_vbox, 4, CNTL_EXPAND, &callback[3]);

  /* Fixed controls */
  cntl_vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 2);
  bobgui_box_append (BOBGUI_BOX (hbox), cntl_vbox);

  create_control (cntl_vbox, 1, CNTL_FIXED, &callback[0]);
  create_control (cntl_vbox, 2, CNTL_FIXED, &callback[1]);
  create_control (cntl_vbox, 3, CNTL_FIXED, &callback[2]);
  create_control (cntl_vbox, 4, CNTL_FIXED, &callback[3]);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
