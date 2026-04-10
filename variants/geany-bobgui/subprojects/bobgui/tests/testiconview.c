/* testiconview.c
 * Copyright (C) 2002  Anders Carlsson <andersca@gnu.org>
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

#include <bobgui/bobgui.h>
#include <glib/gstdio.h>
#include <sys/types.h>
#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#define NUMBER_OF_ITEMS   10
#define SOME_ITEMS       100
#define MANY_ITEMS     10000

static void
fill_model (BobguiTreeModel *model)
{
  GdkPixbuf *pixbuf;
  int i;
  char *str, *str2;
  BobguiTreeIter iter;
  BobguiListStore *store = BOBGUI_LIST_STORE (model);
  gint32 size;
  
  pixbuf = gdk_pixbuf_new_from_file ("gnome-textfile.png", NULL);

  i = 0;
  
  bobgui_list_store_prepend (store, &iter);

  bobgui_list_store_set (store, &iter,
		      0, pixbuf,
		      1, "Really really\nreally really loooooooooong item name",
		      2, 0,
		      3, "This is a <b>Test</b> of <i>markup</i>",
		      4, TRUE,
		      -1);

  while (i < NUMBER_OF_ITEMS - 1)
    {
      GdkPixbuf *pb;
      size = g_random_int_range (20, 70);
      pb = gdk_pixbuf_scale_simple (pixbuf, size, size, GDK_INTERP_NEAREST);

      str = g_strdup_printf ("Icon %d", i);
      str2 = g_strdup_printf ("Icon <b>%d</b>", i);	
      bobgui_list_store_prepend (store, &iter);
      bobgui_list_store_set (store, &iter,
			  0, pb,
			  1, str,
			  2, i,
			  3, str2,
			  4, TRUE,
			  -1);
      g_free (str);
      g_free (str2);
      i++;
    }
  
  //  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (store), 2, BOBGUI_SORT_ASCENDING);
}

static BobguiTreeModel *
create_model (void)
{
  BobguiListStore *store;
  
  store = bobgui_list_store_new (5, GDK_TYPE_PIXBUF, G_TYPE_STRING, G_TYPE_INT, G_TYPE_STRING, G_TYPE_BOOLEAN);

  return BOBGUI_TREE_MODEL (store);
}


static void
foreach_selected_remove (BobguiWidget *button, BobguiIconView *icon_list)
{
  BobguiTreeIter iter;
  BobguiTreeModel *model;

  GList *list, *selected;

  selected = bobgui_icon_view_get_selected_items (icon_list);
  model = bobgui_icon_view_get_model (icon_list);
  
  for (list = selected; list; list = list->next)
    {
      BobguiTreePath *path = list->data;

      bobgui_tree_model_get_iter (model, &iter, path);
      bobgui_list_store_remove (BOBGUI_LIST_STORE (model), &iter);
      
      bobgui_tree_path_free (path);
    } 
  
  g_list_free (selected);
}


static void
swap_rows (BobguiWidget *button, BobguiIconView *icon_list)
{
  BobguiTreeIter iter, iter2;
  BobguiTreeModel *model;

  model = bobgui_icon_view_get_model (icon_list);
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (model), -2, BOBGUI_SORT_ASCENDING);

  bobgui_tree_model_get_iter_first (model, &iter);
  iter2 = iter;
  bobgui_tree_model_iter_next (model, &iter2);
  bobgui_list_store_swap (BOBGUI_LIST_STORE (model), &iter, &iter2);
}

static void
add_n_items (BobguiIconView *icon_list, int n)
{
  static int count = NUMBER_OF_ITEMS;

  BobguiTreeIter iter;
  BobguiListStore *store;
  GdkPixbuf *pixbuf;
  char *str, *str2;
  int i;

  store = BOBGUI_LIST_STORE (bobgui_icon_view_get_model (icon_list));
  pixbuf = gdk_pixbuf_new_from_file ("gnome-textfile.png", NULL);


  for (i = 0; i < n; i++)
    {
      str = g_strdup_printf ("Icon %d", count);
      str2 = g_strdup_printf ("Icon <b>%d</b>", count);	
      bobgui_list_store_prepend (store, &iter);
      bobgui_list_store_set (store, &iter,
			  0, pixbuf,
			  1, str,
			  2, i,
			  3, str2,
			  -1);
      g_free (str);
      g_free (str2);
      count++;
    }
}

static void
add_some (BobguiWidget *button, BobguiIconView *icon_list)
{
  add_n_items (icon_list, SOME_ITEMS);
}

static void
add_many (BobguiWidget *button, BobguiIconView *icon_list)
{
  add_n_items (icon_list, MANY_ITEMS);
}

static void
add_large (BobguiWidget *button, BobguiIconView *icon_list)
{
  BobguiListStore *store;
  BobguiTreeIter iter;

  GdkPixbuf *pixbuf, *pb;
  char *str;

  store = BOBGUI_LIST_STORE (bobgui_icon_view_get_model (icon_list));
  pixbuf = gdk_pixbuf_new_from_file ("gnome-textfile.png", NULL);

  pb = gdk_pixbuf_scale_simple (pixbuf, 
				2 * gdk_pixbuf_get_width (pixbuf),
				2 * gdk_pixbuf_get_height (pixbuf),
				GDK_INTERP_BILINEAR);

  str = g_strdup_printf ("Some really long text");
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter,
		      0, pb,
		      1, str,
		      2, 0,
		      3, str,
		      -1);
  g_object_unref (pb);
  g_free (str);
  
  pb = gdk_pixbuf_scale_simple (pixbuf, 
				3 * gdk_pixbuf_get_width (pixbuf),
				3 * gdk_pixbuf_get_height (pixbuf),
				GDK_INTERP_BILINEAR);

  str = g_strdup ("see how long text behaves when placed underneath "
		  "an oversized icon which would allow for long lines");
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter,
		      0, pb,
		      1, str,
		      2, 1,
		      3, str,
		      -1);
  g_object_unref (pb);
  g_free (str);

  pb = gdk_pixbuf_scale_simple (pixbuf, 
				3 * gdk_pixbuf_get_width (pixbuf),
				3 * gdk_pixbuf_get_height (pixbuf),
				GDK_INTERP_BILINEAR);

  str = g_strdup ("short text");
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter,
		      0, pb,
		      1, str,
		      2, 2,
		      3, str,
		      -1);
  g_object_unref (pb);
  g_free (str);

  g_object_unref (pixbuf);
}

static void
select_all (BobguiWidget *button, BobguiIconView *icon_list)
{
  bobgui_icon_view_select_all (icon_list);
}

static void
select_nonexisting (BobguiWidget *button, BobguiIconView *icon_list)
{  
  BobguiTreePath *path = bobgui_tree_path_new_from_indices (999999, -1);
  bobgui_icon_view_select_path (icon_list, path);
  bobgui_tree_path_free (path);
}

static void
unselect_all (BobguiWidget *button, BobguiIconView *icon_list)
{
  bobgui_icon_view_unselect_all (icon_list);
}

static void
selection_changed (BobguiIconView *icon_list)
{
  g_print ("Selection changed!\n");
}

typedef struct {
  BobguiIconView     *icon_list;
  BobguiTreePath     *path;
} ItemData;

static void
free_item_data (ItemData *data)
{
  bobgui_tree_path_free (data->path);
  g_free (data);
}

static void
item_activated (BobguiIconView *icon_view,
		BobguiTreePath *path)
{
  BobguiTreeIter iter;
  BobguiTreeModel *model;
  char *text;

  model = bobgui_icon_view_get_model (icon_view);
  bobgui_tree_model_get_iter (model, &iter, path);

  bobgui_tree_model_get (model, &iter, 1, &text, -1);
  g_print ("Item activated, text is %s\n", text);
  g_free (text);
  
}

static void
toggled (BobguiCellRendererToggle *cell,
	 char                  *path_string,
	 gpointer               data)
{
  BobguiTreeModel *model = BOBGUI_TREE_MODEL (data);
  BobguiTreeIter iter;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_string);
  gboolean value;

  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_tree_model_get (model, &iter, 4, &value, -1);

  value = !value;
  bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter, 4, value, -1);

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
  bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter, 1, new_text, -1);

  bobgui_tree_path_free (path);
}

static void
item_cb (BobguiWidget *menuitem,
	 ItemData  *data)
{
  item_activated (data->icon_list, data->path);
}

static void
do_popup_menu (BobguiWidget   *icon_list,
               BobguiTreePath *path)
{
  BobguiIconView *icon_view = BOBGUI_ICON_VIEW (icon_list);
  BobguiWidget *menu;
  BobguiWidget *item;
  ItemData *data;

  if (!path)
    return;

  menu = bobgui_popover_new ();
  bobgui_widget_set_parent (menu, icon_list);

  data = g_new0 (ItemData, 1);
  data->icon_list = icon_view;
  data->path = path;
  g_object_set_data_full (G_OBJECT (menu), "item-path", data, (GDestroyNotify)free_item_data);

  item = bobgui_button_new_with_label ("Activate");
  bobgui_popover_set_child (BOBGUI_POPOVER (menu), item);
  g_signal_connect (item, "clicked", G_CALLBACK (item_cb), data);

  bobgui_popover_popup (BOBGUI_POPOVER (menu));
}

static void
press_handler (BobguiGestureClick *gesture,
               guint            n_press,
               double           x,
               double           y,
               BobguiWidget       *widget)
{
  BobguiTreePath *path = NULL;

  /* Ignore double-clicks and triple-clicks */
  if (n_press > 1)
    return;

  path = bobgui_icon_view_get_path_at_pos (BOBGUI_ICON_VIEW (widget), x, y);
  do_popup_menu (widget, path);
}

static gboolean
popup_menu_handler (BobguiWidget *widget)
{
  BobguiTreePath *path = NULL;
  GList *list;

  list = bobgui_icon_view_get_selected_items (BOBGUI_ICON_VIEW (widget));

  if (list)
    {
      path = (BobguiTreePath*)list->data;
      g_list_free_full (list, (GDestroyNotify) bobgui_tree_path_free);
    }

  do_popup_menu (widget, path);
  return TRUE;
}

int
main (int argc, char **argv)
{
  BobguiWidget *paned, *tv;
  BobguiWidget *window, *icon_list, *scrolled_window;
  BobguiWidget *vbox, *bbox;
  BobguiWidget *button;
  BobguiTreeModel *model;
  BobguiCellRenderer *cell;
  BobguiTreeViewColumn *tvc;
  GdkContentFormats *targets;
  BobguiGesture *gesture;
  
#ifdef BOBGUI_SRCDIR
  g_chdir (BOBGUI_SRCDIR);
#endif

  bobgui_init ();

  /* to test rtl layout, set RTL=1 in the environment */
  if (g_getenv ("RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 700, 400);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  paned = bobgui_paned_new (BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_widget_set_vexpand (paned, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), paned);

  icon_list = bobgui_icon_view_new ();
  bobgui_icon_view_set_selection_mode (BOBGUI_ICON_VIEW (icon_list), BOBGUI_SELECTION_MULTIPLE);

  tv = bobgui_tree_view_new ();
  tvc = bobgui_tree_view_column_new ();
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tv), tvc);

  gesture = bobgui_gesture_click_new ();
  bobgui_gesture_single_set_button (BOBGUI_GESTURE_SINGLE (gesture),
                                 GDK_BUTTON_SECONDARY);
  g_signal_connect (gesture, "pressed",
                    G_CALLBACK (press_handler), icon_list);
  bobgui_widget_add_controller (icon_list, BOBGUI_EVENT_CONTROLLER (gesture));

  g_signal_connect (icon_list, "selection_changed",
		    G_CALLBACK (selection_changed), NULL);
  g_signal_connect (icon_list, "popup_menu",
		    G_CALLBACK (popup_menu_handler), NULL);

  g_signal_connect (icon_list, "item_activated",
		    G_CALLBACK (item_activated), NULL);
  
  model = create_model ();
  bobgui_icon_view_set_model (BOBGUI_ICON_VIEW (icon_list), model);
  bobgui_tree_view_set_model (BOBGUI_TREE_VIEW (tv), model);
  fill_model (model);

#if 0

  bobgui_icon_view_set_pixbuf_column (BOBGUI_ICON_VIEW (icon_list), 0);
  bobgui_icon_view_set_text_column (BOBGUI_ICON_VIEW (icon_list), 1);

#else

  cell = bobgui_cell_renderer_toggle_new ();
  bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (icon_list), cell, FALSE);
  g_object_set (cell, "activatable", TRUE, NULL);
  bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (icon_list),
				  cell, "active", 4, NULL);
  g_signal_connect (cell, "toggled", G_CALLBACK (toggled), model);

  cell = bobgui_cell_renderer_pixbuf_new ();
  bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (icon_list), cell, FALSE);
  bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (icon_list),
				  cell, "pixbuf", 0, NULL);

  cell = bobgui_cell_renderer_text_new ();
  bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (icon_list), cell, FALSE);
  g_object_set (cell, 
		"editable", TRUE, 
		"xalign", 0.5,
		"wrap-mode", PANGO_WRAP_WORD_CHAR,
		"wrap-width", 100,
		NULL);
  bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (icon_list),
				  cell, "text", 1, NULL);
  g_signal_connect (cell, "edited", G_CALLBACK (edited), model);

  /* now the tree view... */
  cell = bobgui_cell_renderer_toggle_new ();
  bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (tvc), cell, FALSE);
  g_object_set (cell, "activatable", TRUE, NULL);
  bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (tvc),
				  cell, "active", 4, NULL);
  g_signal_connect (cell, "toggled", G_CALLBACK (toggled), model);

  cell = bobgui_cell_renderer_pixbuf_new ();
  bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (tvc), cell, FALSE);
  bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (tvc),
				  cell, "pixbuf", 0, NULL);

  cell = bobgui_cell_renderer_text_new ();
  bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (tvc), cell, FALSE);
  g_object_set (cell, "editable", TRUE, NULL);
  bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (tvc),
				  cell, "text", 1, NULL);
  g_signal_connect (cell, "edited", G_CALLBACK (edited), model);
#endif
  /* Allow DND between the icon view and the tree view */
  
  targets = gdk_content_formats_new_for_gtype (BOBGUI_TYPE_TREE_ROW_DATA);
  bobgui_icon_view_enable_model_drag_source (BOBGUI_ICON_VIEW (icon_list),
					  GDK_BUTTON1_MASK,
                                          targets,
					  GDK_ACTION_MOVE);
  bobgui_icon_view_enable_model_drag_dest (BOBGUI_ICON_VIEW (icon_list),
                                        targets,
					GDK_ACTION_MOVE);

  bobgui_tree_view_enable_model_drag_source (BOBGUI_TREE_VIEW (tv),
					  GDK_BUTTON1_MASK,
                                          targets,
					  GDK_ACTION_MOVE);
  bobgui_tree_view_enable_model_drag_dest (BOBGUI_TREE_VIEW (tv),
                                        targets,
					GDK_ACTION_MOVE);
  gdk_content_formats_unref (targets);

  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), icon_list);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window),
  				  BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);

  bobgui_paned_set_start_child (BOBGUI_PANED (paned), scrolled_window);

  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), tv);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window),
  				  BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);

  bobgui_paned_set_end_child (BOBGUI_PANED (paned), scrolled_window);

  bbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_halign (bbox, BOBGUI_ALIGN_START);
  bobgui_box_append (BOBGUI_BOX (vbox), bbox);

  button = bobgui_button_new_with_label ("Add some");
  g_signal_connect (button, "clicked", G_CALLBACK (add_some), icon_list);
  bobgui_box_append (BOBGUI_BOX (bbox), button);

  button = bobgui_button_new_with_label ("Add many");
  g_signal_connect (button, "clicked", G_CALLBACK (add_many), icon_list);
  bobgui_box_append (BOBGUI_BOX (bbox), button);

  button = bobgui_button_new_with_label ("Add large");
  g_signal_connect (button, "clicked", G_CALLBACK (add_large), icon_list);
  bobgui_box_append (BOBGUI_BOX (bbox), button);

  button = bobgui_button_new_with_label ("Remove selected");
  g_signal_connect (button, "clicked", G_CALLBACK (foreach_selected_remove), icon_list);
  bobgui_box_append (BOBGUI_BOX (bbox), button);

  button = bobgui_button_new_with_label ("Swap");
  g_signal_connect (button, "clicked", G_CALLBACK (swap_rows), icon_list);
  bobgui_box_append (BOBGUI_BOX (bbox), button);

  bbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  bobgui_widget_set_halign (bbox, BOBGUI_ALIGN_START);
  bobgui_box_append (BOBGUI_BOX (vbox), bbox);

  button = bobgui_button_new_with_label ("Select all");
  g_signal_connect (button, "clicked", G_CALLBACK (select_all), icon_list);
  bobgui_box_append (BOBGUI_BOX (bbox), button);

  button = bobgui_button_new_with_label ("Unselect all");
  g_signal_connect (button, "clicked", G_CALLBACK (unselect_all), icon_list);
  bobgui_box_append (BOBGUI_BOX (bbox), button);

  button = bobgui_button_new_with_label ("Select nonexisting");
  g_signal_connect (button, "clicked", G_CALLBACK (select_nonexisting), icon_list);
  bobgui_box_append (BOBGUI_BOX (bbox), button);

  icon_list = bobgui_icon_view_new ();

  scrolled_window = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (scrolled_window), icon_list);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (scrolled_window),
                                  BOBGUI_POLICY_AUTOMATIC, BOBGUI_POLICY_AUTOMATIC);
  bobgui_paned_set_end_child (BOBGUI_PANED (paned), scrolled_window);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
