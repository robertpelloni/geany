/* Tree View/Editable Cells
 *
 * This demo demonstrates the use of editable cells in a BobguiTreeView. If
 * you're new to the BobguiTreeView widgets and associates, look into
 * the BobguiListStore example first. It also shows how to use the
 * BobguiCellRenderer::editing-started signal to do custom setup of the
 * editable widget.
 *
 * The cell renderers used in this demo are BobguiCellRendererText,
 * BobguiCellRendererCombo and BobguiCellRendererProgress.
 */

#include <bobgui/bobgui.h>
#include <string.h>
#include <stdlib.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

typedef struct
{
  int    number;
  char *product;
  int    yummy;
}
Item;

enum
{
  COLUMN_ITEM_NUMBER,
  COLUMN_ITEM_PRODUCT,
  COLUMN_ITEM_YUMMY,
  NUM_ITEM_COLUMNS
};

enum
{
  COLUMN_NUMBER_TEXT,
  NUM_NUMBER_COLUMNS
};

static GArray *articles = NULL;

static void
add_items (void)
{
  Item foo;

  g_return_if_fail (articles != NULL);

  foo.number = 3;
  foo.product = g_strdup ("bottles of coke");
  foo.yummy = 20;
  g_array_append_vals (articles, &foo, 1);

  foo.number = 5;
  foo.product = g_strdup ("packages of noodles");
  foo.yummy = 50;
  g_array_append_vals (articles, &foo, 1);

  foo.number = 2;
  foo.product = g_strdup ("packages of chocolate chip cookies");
  foo.yummy = 90;
  g_array_append_vals (articles, &foo, 1);

  foo.number = 1;
  foo.product = g_strdup ("can vanilla ice cream");
  foo.yummy = 60;
  g_array_append_vals (articles, &foo, 1);

  foo.number = 6;
  foo.product = g_strdup ("eggs");
  foo.yummy = 10;
  g_array_append_vals (articles, &foo, 1);
}

static BobguiTreeModel *
create_items_model (void)
{
  int i = 0;
  BobguiListStore *model;
  BobguiTreeIter iter;

  /* create array */
  articles = g_array_sized_new (FALSE, FALSE, sizeof (Item), 1);

  add_items ();

  /* create list store */
  model = bobgui_list_store_new (NUM_ITEM_COLUMNS, G_TYPE_INT, G_TYPE_STRING,
                              G_TYPE_INT, G_TYPE_BOOLEAN);

  /* add items */
  for (i = 0; i < articles->len; i++)
    {
      bobgui_list_store_append (model, &iter);

      bobgui_list_store_set (model, &iter,
                          COLUMN_ITEM_NUMBER,
                          g_array_index (articles, Item, i).number,
                          COLUMN_ITEM_PRODUCT,
                          g_array_index (articles, Item, i).product,
                          COLUMN_ITEM_YUMMY,
                          g_array_index (articles, Item, i).yummy,
                          -1);
    }

  return BOBGUI_TREE_MODEL (model);
}

static BobguiTreeModel *
create_numbers_model (void)
{
#define N_NUMBERS 10
  int i = 0;
  BobguiListStore *model;
  BobguiTreeIter iter;

  /* create list store */
  model = bobgui_list_store_new (NUM_NUMBER_COLUMNS, G_TYPE_STRING, G_TYPE_INT);

  /* add numbers */
  for (i = 0; i < N_NUMBERS; i++)
    {
      char str[2];

      str[0] = '0' + i;
      str[1] = '\0';

      bobgui_list_store_append (model, &iter);

      bobgui_list_store_set (model, &iter,
                          COLUMN_NUMBER_TEXT, str,
                          -1);
    }

  return BOBGUI_TREE_MODEL (model);

#undef N_NUMBERS
}

static void
add_item (BobguiWidget *button, gpointer data)
{
  Item foo;
  BobguiTreeIter current, iter;
  BobguiTreePath *path;
  BobguiTreeModel *model;
  BobguiTreeViewColumn *column;
  BobguiTreeView *treeview = (BobguiTreeView *)data;

  g_return_if_fail (articles != NULL);

  foo.number = 0;
  foo.product = g_strdup ("Description here");
  foo.yummy = 50;
  g_array_append_vals (articles, &foo, 1);

  /* Insert a new row below the current one */
  bobgui_tree_view_get_cursor (treeview, &path, NULL);
  model = bobgui_tree_view_get_model (treeview);
  if (path)
    {
      bobgui_tree_model_get_iter (model, &current, path);
      bobgui_tree_path_free (path);
      bobgui_list_store_insert_after (BOBGUI_LIST_STORE (model), &iter, &current);
    }
  else
    {
      bobgui_list_store_insert (BOBGUI_LIST_STORE (model), &iter, -1);
    }

  /* Set the data for the new row */
  bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter,
                      COLUMN_ITEM_NUMBER, foo.number,
                      COLUMN_ITEM_PRODUCT, foo.product,
                      COLUMN_ITEM_YUMMY, foo.yummy,
                      -1);

  /* Move focus to the new row */
  path = bobgui_tree_model_get_path (model, &iter);
  column = bobgui_tree_view_get_column (treeview, 0);
  bobgui_tree_view_set_cursor (treeview, path, column, FALSE);

  bobgui_tree_path_free (path);
}

static void
remove_item (BobguiWidget *widget, gpointer data)
{
  BobguiTreeIter iter;
  BobguiTreeView *treeview = (BobguiTreeView *)data;
  BobguiTreeModel *model = bobgui_tree_view_get_model (treeview);
  BobguiTreeSelection *selection = bobgui_tree_view_get_selection (treeview);

  if (bobgui_tree_selection_get_selected (selection, NULL, &iter))
    {
      int i;
      BobguiTreePath *path;

      path = bobgui_tree_model_get_path (model, &iter);
      i = bobgui_tree_path_get_indices (path)[0];
      bobgui_list_store_remove (BOBGUI_LIST_STORE (model), &iter);

      g_array_remove_index (articles, i);

      bobgui_tree_path_free (path);
    }
}

static gboolean
separator_row (BobguiTreeModel *model,
               BobguiTreeIter  *iter,
               gpointer      data)
{
  BobguiTreePath *path;
  int idx;

  path = bobgui_tree_model_get_path (model, iter);
  idx = bobgui_tree_path_get_indices (path)[0];

  bobgui_tree_path_free (path);

  return idx == 5;
}

static void
editing_started (BobguiCellRenderer *cell,
                 BobguiCellEditable *editable,
                 const char      *path,
                 gpointer         data)
{
  bobgui_combo_box_set_row_separator_func (BOBGUI_COMBO_BOX (editable),
                                        separator_row, NULL, NULL);
}

static void
cell_edited (BobguiCellRendererText *cell,
             const char          *path_string,
             const char          *new_text,
             gpointer             data)
{
  BobguiTreeModel *model = (BobguiTreeModel *)data;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_string);
  BobguiTreeIter iter;

  int column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  bobgui_tree_model_get_iter (model, &iter, path);

  switch (column)
    {
    case COLUMN_ITEM_NUMBER:
      {
        int i;

        i = bobgui_tree_path_get_indices (path)[0];
        g_array_index (articles, Item, i).number = atoi (new_text);

        bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter, column,
                            g_array_index (articles, Item, i).number, -1);
      }
      break;

    case COLUMN_ITEM_PRODUCT:
      {
        int i;
        char *old_text;

        bobgui_tree_model_get (model, &iter, column, &old_text, -1);
        g_free (old_text);

        i = bobgui_tree_path_get_indices (path)[0];
        g_free (g_array_index (articles, Item, i).product);
        g_array_index (articles, Item, i).product = g_strdup (new_text);

        bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter, column,
                            g_array_index (articles, Item, i).product, -1);
      }
      break;

    default:
      g_assert_not_reached ();
    }

  bobgui_tree_path_free (path);
}

static void
add_columns (BobguiTreeView  *treeview,
             BobguiTreeModel *items_model,
             BobguiTreeModel *numbers_model)
{
  BobguiCellRenderer *renderer;

  /* number column */
  renderer = bobgui_cell_renderer_combo_new ();
  g_object_set (renderer,
                "model", numbers_model,
                "text-column", COLUMN_NUMBER_TEXT,
                "has-entry", FALSE,
                "editable", TRUE,
                NULL);
  g_signal_connect (renderer, "edited",
                    G_CALLBACK (cell_edited), items_model);
  g_signal_connect (renderer, "editing-started",
                    G_CALLBACK (editing_started), NULL);
  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_ITEM_NUMBER));

  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (treeview),
                                               -1, "Number", renderer,
                                               "text", COLUMN_ITEM_NUMBER,
                                               NULL);

  /* product column */
  renderer = bobgui_cell_renderer_text_new ();
  g_object_set (renderer,
                "editable", TRUE,
                NULL);
  g_signal_connect (renderer, "edited",
                    G_CALLBACK (cell_edited), items_model);
  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_ITEM_PRODUCT));

  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (treeview),
                                               -1, "Product", renderer,
                                               "text", COLUMN_ITEM_PRODUCT,
                                               NULL);

  /* yummy column */
  renderer = bobgui_cell_renderer_progress_new ();
  g_object_set_data (G_OBJECT (renderer), "column", GINT_TO_POINTER (COLUMN_ITEM_YUMMY));

  bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (treeview),
                                               -1, "Yummy", renderer,
                                               "value", COLUMN_ITEM_YUMMY,
                                               NULL);
}

BobguiWidget *
do_editable_cells (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *vbox;
      BobguiWidget *hbox;
      BobguiWidget *sw;
      BobguiWidget *treeview;
      BobguiWidget *button;
      BobguiTreeModel *items_model;
      BobguiTreeModel *numbers_model;

      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Editable Cells");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
      bobgui_widget_set_margin_start (vbox, 5);
      bobgui_widget_set_margin_end (vbox, 5);
      bobgui_widget_set_margin_top (vbox, 5);
      bobgui_widget_set_margin_bottom (vbox, 5);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      bobgui_box_append (BOBGUI_BOX (vbox),
                          bobgui_label_new ("Shopping list (you can edit the cells!)"));

      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_AUTOMATIC,
                                      BOBGUI_POLICY_AUTOMATIC);
      bobgui_box_append (BOBGUI_BOX (vbox), sw);

      /* create models */
      items_model = create_items_model ();
      numbers_model = create_numbers_model ();

      /* create tree view */
      treeview = bobgui_tree_view_new_with_model (items_model);
      bobgui_widget_set_vexpand (treeview, TRUE);
      bobgui_tree_selection_set_mode (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (treeview)),
                                   BOBGUI_SELECTION_SINGLE);

      add_columns (BOBGUI_TREE_VIEW (treeview), items_model, numbers_model);

      g_object_unref (numbers_model);
      g_object_unref (items_model);

      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), treeview);

      /* some buttons */
      hbox = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 4);
      bobgui_box_set_homogeneous (BOBGUI_BOX (hbox), TRUE);
      bobgui_box_append (BOBGUI_BOX (vbox), hbox);

      button = bobgui_button_new_with_label ("Add item");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (add_item), treeview);
      bobgui_box_append (BOBGUI_BOX (hbox), button);

      button = bobgui_button_new_with_label ("Remove item");
      g_signal_connect (button, "clicked",
                        G_CALLBACK (remove_item), treeview);
      bobgui_box_append (BOBGUI_BOX (hbox), button);

      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 320, 200);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
