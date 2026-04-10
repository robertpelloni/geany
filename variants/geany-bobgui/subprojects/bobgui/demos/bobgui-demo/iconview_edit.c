/* Icon View/Editing and Drag-and-Drop
 * #Keywords: dnd
 *
 * The BobguiIconView widget supports Editing and Drag-and-Drop.
 * This example also demonstrates using the generic BobguiCellLayout
 * interface to set up cell renderers in an icon view.
 */

#include <bobgui/bobgui.h>
#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

enum
{
  COL_TEXT,
  NUM_COLS
};


static void
fill_store (BobguiListStore *store)
{
  BobguiTreeIter iter;
  const char *text[] = { "Red", "Green", "Blue", "Yellow" };
  int i;

  /* First clear the store */
  bobgui_list_store_clear (store);

  for (i = 0; i < 4; i++)
    {
      bobgui_list_store_append (store, &iter);
      bobgui_list_store_set (store, &iter, COL_TEXT, text[i], -1);
    }
}

static BobguiListStore *
create_store (void)
{
  BobguiListStore *store;

  store = bobgui_list_store_new (NUM_COLS, G_TYPE_STRING);

  return store;
}

static void
set_cell_color (BobguiCellLayout   *cell_layout,
                BobguiCellRenderer *cell,
                BobguiTreeModel    *tree_model,
                BobguiTreeIter     *iter,
                gpointer         data)
{
  char *text;
  GdkRGBA color;
  guint32 pixel = 0;
  GdkPixbuf *pixbuf;

  bobgui_tree_model_get (tree_model, iter, COL_TEXT, &text, -1);
  if (!text)
    return;

  if (gdk_rgba_parse (&color, text))
    pixel =
      ((int)(color.red * 255)) << 24 |
      ((int)(color.green * 255)) << 16 |
      ((int)(color.blue  * 255)) << 8 |
      ((int)(color.alpha * 255));

  g_free (text);

  pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB, TRUE, 8, 24, 24);
  gdk_pixbuf_fill (pixbuf, pixel);

  g_object_set (cell, "pixbuf", pixbuf, NULL);

  g_object_unref (pixbuf);
}

static void
edited (BobguiCellRendererText *cell,
        char                *path_string,
        char                *text,
        gpointer             data)
{
  BobguiTreeModel *model;
  BobguiTreeIter iter;
  BobguiTreePath *path;

  model = bobgui_icon_view_get_model (BOBGUI_ICON_VIEW (data));
  path = bobgui_tree_path_new_from_string (path_string);

  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter,
                      COL_TEXT, text, -1);

  bobgui_tree_path_free (path);
}

BobguiWidget *
do_iconview_edit (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *icon_view;
      BobguiListStore *store;
      BobguiCellRenderer *renderer;

      window = bobgui_window_new ();

      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Editing and Drag-and-Drop");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      store = create_store ();
      fill_store (store);

      icon_view = bobgui_icon_view_new_with_model (BOBGUI_TREE_MODEL (store));
      g_object_unref (store);

      bobgui_icon_view_set_selection_mode (BOBGUI_ICON_VIEW (icon_view),
                                        BOBGUI_SELECTION_SINGLE);
      bobgui_icon_view_set_item_orientation (BOBGUI_ICON_VIEW (icon_view),
                                          BOBGUI_ORIENTATION_HORIZONTAL);
      bobgui_icon_view_set_columns (BOBGUI_ICON_VIEW (icon_view), 2);
      bobgui_icon_view_set_reorderable (BOBGUI_ICON_VIEW (icon_view), TRUE);

      renderer = bobgui_cell_renderer_pixbuf_new ();
      bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (icon_view),
                                  renderer, TRUE);
      bobgui_cell_layout_set_cell_data_func (BOBGUI_CELL_LAYOUT (icon_view),
                                          renderer,
                                          set_cell_color,
                                          NULL, NULL);

      renderer = bobgui_cell_renderer_text_new ();
      bobgui_cell_layout_pack_start (BOBGUI_CELL_LAYOUT (icon_view),
                                  renderer, TRUE);
      g_object_set (renderer, "editable", TRUE, NULL);
      g_signal_connect (renderer, "edited", G_CALLBACK (edited), icon_view);
      bobgui_cell_layout_set_attributes (BOBGUI_CELL_LAYOUT (icon_view),
                                      renderer,
                                      "text", COL_TEXT,
                                      NULL);

      bobgui_window_set_child (BOBGUI_WINDOW (window), icon_view);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
