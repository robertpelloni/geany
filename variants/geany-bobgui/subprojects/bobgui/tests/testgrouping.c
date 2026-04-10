#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiTreeModel *
create_model (void)
{
  BobguiTreeStore *store;
  BobguiTreeIter iter;
  BobguiTreeIter parent;

  store = bobgui_tree_store_new (1, G_TYPE_STRING);

  bobgui_tree_store_insert_with_values (store, &parent, NULL, 0,
				     0, "Applications", -1);

  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "File Manager", -1);
  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "Gossip", -1);
  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "System Settings", -1);
  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "The GIMP", -1);
  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "Terminal", -1);
  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "Word Processor", -1);


  bobgui_tree_store_insert_with_values (store, &parent, NULL, 1,
				     0, "Documents", -1);

  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "blaat.txt", -1);
  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "sliff.txt", -1);
  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "test.txt", -1);
  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "blaat.txt", -1);
  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "brrrr.txt", -1);
  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "hohoho.txt", -1);


  bobgui_tree_store_insert_with_values (store, &parent, NULL, 2,
				     0, "Images", -1);

  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "image1.png", -1);
  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "image2.png", -1);
  bobgui_tree_store_insert_with_values (store, &iter, &parent, 0,
				     0, "image3.jpg", -1);

  return BOBGUI_TREE_MODEL (store);
}

static void
set_color_func (BobguiTreeViewColumn *column,
		BobguiCellRenderer   *cell,
		BobguiTreeModel      *model,
		BobguiTreeIter       *iter,
		gpointer           data)
{
  if (bobgui_tree_model_iter_has_child (model, iter))
    g_object_set (cell, "cell-background", "Grey", NULL);
  else
    g_object_set (cell, "cell-background", NULL, NULL);
}

static void
tree_view_row_activated (BobguiTreeView       *tree_view,
			 BobguiTreePath       *path,
			 BobguiTreeViewColumn *column)
{
  if (bobgui_tree_path_get_depth (path) > 1)
    return;

  if (bobgui_tree_view_row_expanded (BOBGUI_TREE_VIEW (tree_view), path))
    bobgui_tree_view_collapse_row (BOBGUI_TREE_VIEW (tree_view), path);
  else
    bobgui_tree_view_expand_row (BOBGUI_TREE_VIEW (tree_view), path, FALSE);
}

static gboolean
tree_view_select_func (BobguiTreeSelection *selection,
		       BobguiTreeModel     *model,
		       BobguiTreePath      *path,
		       gboolean          path_currently_selected,
		       gpointer          data)
{
  if (bobgui_tree_path_get_depth (path) > 1)
    return TRUE;

  return FALSE;
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
  BobguiWidget *window, *sw, *tv;
  BobguiTreeModel *model;
  BobguiCellRenderer *renderer;
  BobguiTreeViewColumn *column;
  gboolean done = FALSE;

  bobgui_init ();

  model = create_model ();

  window = bobgui_window_new ();
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 320, 480);

  sw = bobgui_scrolled_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), sw);

  tv = bobgui_tree_view_new_with_model (model);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), tv);

  g_signal_connect (tv, "row-activated",
		    G_CALLBACK (tree_view_row_activated), tv);
  g_object_set (tv,
		"show-expanders", FALSE,
		"level-indentation", 10,
		NULL);

  bobgui_tree_view_set_headers_visible (BOBGUI_TREE_VIEW (tv), FALSE);
  bobgui_tree_view_expand_all (BOBGUI_TREE_VIEW (tv));

  bobgui_tree_selection_set_select_function (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (tv)),
					  tree_view_select_func,
					  NULL,
					  NULL);

  renderer = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("(none)",
						     renderer,
						     "text", 0,
						     NULL);
  bobgui_tree_view_column_set_cell_data_func (column,
					   renderer,
					   set_color_func,
					   NULL,
					   NULL);
  bobgui_tree_view_insert_column (BOBGUI_TREE_VIEW (tv), column, 0);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
