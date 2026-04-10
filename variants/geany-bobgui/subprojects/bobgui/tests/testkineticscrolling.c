#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
on_button_clicked (BobguiWidget *widget, gpointer data)
{
  g_print ("Button %d clicked\n", GPOINTER_TO_INT (data));
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
kinetic_scrolling (void)
{
  BobguiWidget *window, *swindow, *grid;
  BobguiWidget *label;
  BobguiWidget *button_grid, *button;
  BobguiWidget *treeview;
  BobguiCellRenderer *renderer;
  BobguiListStore *store;
  BobguiWidget *textview;
  GdkContentFormats *targets;
  int i;

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 400);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  grid = bobgui_grid_new ();

  label = bobgui_label_new ("Non scrollable widget using viewport");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 0, 0, 1, 1);
  bobgui_widget_set_hexpand (label, TRUE);

  label = bobgui_label_new ("Scrollable widget: TreeView");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 1, 0, 1, 1);
  bobgui_widget_set_hexpand (label, TRUE);

  label = bobgui_label_new ("Scrollable widget: TextView");
  bobgui_grid_attach (BOBGUI_GRID (grid), label, 2, 0, 1, 1);
  bobgui_widget_set_hexpand (label, TRUE);

  button_grid = bobgui_grid_new ();
  for (i = 0; i < 80; i++)
    {
      char *button_label = g_strdup_printf ("Button number %d", i);

      button = bobgui_button_new_with_label (button_label);
      bobgui_grid_attach (BOBGUI_GRID (button_grid), button, 0, i, 1, 1);
      bobgui_widget_set_hexpand (button, TRUE);
      g_signal_connect (button, "clicked",
                        G_CALLBACK (on_button_clicked),
                        GINT_TO_POINTER (i));
      g_free (button_label);
    }

  swindow = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_kinetic_scrolling (BOBGUI_SCROLLED_WINDOW (swindow), TRUE);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (swindow), button_grid);

  bobgui_grid_attach (BOBGUI_GRID (grid), swindow, 0, 1, 1, 1);

  treeview = bobgui_tree_view_new ();
  targets = gdk_content_formats_new_for_gtype (BOBGUI_TYPE_TREE_ROW_DATA);
  bobgui_tree_view_enable_model_drag_source (BOBGUI_TREE_VIEW (treeview),
                                          GDK_BUTTON1_MASK,
                                          targets,
                                          GDK_ACTION_MOVE | GDK_ACTION_COPY);
  bobgui_tree_view_enable_model_drag_dest (BOBGUI_TREE_VIEW (treeview),
                                        targets,
                                        GDK_ACTION_MOVE | GDK_ACTION_COPY);
  gdk_content_formats_unref (targets);

  renderer = bobgui_cell_renderer_text_new ();
  g_object_set (renderer, "editable", TRUE, NULL);
  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (treeview),
                                               0, "Title",
                                               renderer,
                                               "text", 0,
                                               NULL);
  store = bobgui_list_store_new (1, G_TYPE_STRING);
  for (i = 0; i < 80; i++)
    {
      BobguiTreeIter iter;
      char *iter_label = g_strdup_printf ("Row number %d", i);

      bobgui_list_store_append (store, &iter);
      bobgui_list_store_set (store, &iter, 0, iter_label, -1);
      g_free (iter_label);
    }
  bobgui_tree_view_set_model (BOBGUI_TREE_VIEW (treeview), BOBGUI_TREE_MODEL (store));
  g_object_unref (store);

  swindow = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_kinetic_scrolling (BOBGUI_SCROLLED_WINDOW (swindow), TRUE);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (swindow), treeview);

  bobgui_grid_attach (BOBGUI_GRID (grid), swindow, 1, 1, 1, 1);
  bobgui_widget_set_hexpand (swindow, TRUE);
  bobgui_widget_set_vexpand (swindow, TRUE);

  textview = bobgui_text_view_new ();
  swindow = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_kinetic_scrolling (BOBGUI_SCROLLED_WINDOW (swindow), TRUE);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (swindow), textview);

  bobgui_grid_attach (BOBGUI_GRID (grid), swindow, 2, 1, 1, 1);
  bobgui_widget_set_hexpand (swindow, TRUE);
  bobgui_widget_set_vexpand (swindow, TRUE);

  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int argc, char **argv)
{
  bobgui_init ();

  kinetic_scrolling ();

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
