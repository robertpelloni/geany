#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static gboolean
clicked_icon (BobguiTreeView  *tv,
              int           x,
              int           y,
              BobguiTreePath **path)
{
  BobguiTreeViewColumn *col;
  int cell_x, cell_y;
  int cell_pos, cell_width;
  GList *cells, *l;
  int depth;
  int level_indentation;
  int expander_size;
  int indent;

  if (bobgui_tree_view_get_path_at_pos (tv, x, y, path, &col, &cell_x, &cell_y))
    {
      cells = bobgui_cell_layout_get_cells (BOBGUI_CELL_LAYOUT (col));

#if 1
      /* ugly workaround to fix the problem:
       * manually calculate the indent for the row
       */
      depth = bobgui_tree_path_get_depth (*path);
      level_indentation = bobgui_tree_view_get_level_indentation (tv);
      expander_size = 16; /* Hardcoded in bobguitreeview.c */
      expander_size += 4;
      indent = (depth - 1) * level_indentation + depth * expander_size;
#else
      indent = 0;
#endif

      for (l = cells; l; l = l->next)
        {
          bobgui_tree_view_column_cell_get_position (col, l->data, &cell_pos, &cell_width);
          if (cell_pos + indent <= cell_x && cell_x <= cell_pos + indent + cell_width)
            {
              g_print ("clicked in %s\n", g_type_name_from_instance (l->data));
              if (BOBGUI_IS_CELL_RENDERER_PIXBUF (l->data))
                {
                  g_list_free (cells);
                  return TRUE;
                }
            }
        }

      g_list_free (cells);
    }

  return FALSE;
}

static void
release_event (BobguiGestureClick *gesture,
               guint            n_press,
               double           x,
               double           y,
               BobguiTreeView     *tv)
{
  BobguiTreePath *path;
  int tx, ty;

  bobgui_tree_view_convert_widget_to_tree_coords (tv, x, y, &tx, &ty);

  if (clicked_icon (tv, tx, ty, &path))
    {
      BobguiTreeModel *model;
      BobguiTreeIter iter;
      char *text;

      model = bobgui_tree_view_get_model (tv);
      bobgui_tree_model_get_iter (model, &iter, path);
      bobgui_tree_model_get (model, &iter, 0, &text, -1);

      g_print ("text was: %s\n", text);
      g_free (text);
      bobgui_tree_path_free (path);
    }
}

int main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *sw;
  BobguiWidget *tv;
  BobguiTreeViewColumn *col;
  BobguiCellRenderer *cell;
  BobguiTreeStore *store;
  BobguiTreeIter iter;
  BobguiGesture *gesture;

  bobgui_init ();

  window = bobgui_window_new ();
  sw = bobgui_scrolled_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), sw);
  tv = bobgui_tree_view_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), tv);

  col = bobgui_tree_view_column_new ();
  cell = bobgui_cell_renderer_text_new ();
  bobgui_tree_view_column_pack_start (col, cell, TRUE);
  bobgui_tree_view_column_add_attribute (col, cell, "text", 0);

  cell = bobgui_cell_renderer_toggle_new ();
  bobgui_tree_view_column_pack_start (col, cell, FALSE);
  bobgui_tree_view_column_add_attribute (col, cell, "active", 1);

  cell = bobgui_cell_renderer_text_new ();
  bobgui_tree_view_column_pack_start (col, cell, TRUE);
  bobgui_tree_view_column_add_attribute (col, cell, "text", 0);

  cell = bobgui_cell_renderer_pixbuf_new ();
  bobgui_tree_view_column_pack_start (col, cell, FALSE);
  bobgui_tree_view_column_add_attribute (col, cell, "icon-name", 2);

  cell = bobgui_cell_renderer_toggle_new ();
  bobgui_tree_view_column_pack_start (col, cell, FALSE);
  bobgui_tree_view_column_add_attribute (col, cell, "active", 1);

  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tv), col);

  store = bobgui_tree_store_new (3, G_TYPE_STRING, G_TYPE_BOOLEAN, G_TYPE_STRING);
  bobgui_tree_store_insert_with_values (store, NULL, NULL, 0, 0, "One row", 1, FALSE, 2, "document-open", -1);
  bobgui_tree_store_insert_with_values (store, &iter, NULL, 1, 0, "Two row", 1, FALSE, 2, "dialog-warning", -1);
  bobgui_tree_store_insert_with_values (store, NULL, &iter, 0, 0, "Three row", 1, FALSE, 2, "dialog-error", -1);

  bobgui_tree_view_set_model (BOBGUI_TREE_VIEW (tv), BOBGUI_TREE_MODEL (store));

  gesture = bobgui_gesture_click_new ();
  bobgui_event_controller_set_propagation_phase (BOBGUI_EVENT_CONTROLLER (gesture),
                                              BOBGUI_PHASE_CAPTURE);
  g_signal_connect (gesture, "released",
                    G_CALLBACK (release_event), tv);
  bobgui_widget_add_controller (tv, BOBGUI_EVENT_CONTROLLER (gesture));

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
