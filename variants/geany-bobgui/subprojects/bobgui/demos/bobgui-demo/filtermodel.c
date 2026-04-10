/* Tree View/Filter Model
 * #Keywords: BobguiTreeView
 *
 * This example demonstrates how BobguiTreeModelFilter can be used not
 * just to show a subset of the rows, but also to compute columns
 * that are not actually present in the underlying model.
 */

#include <bobgui/bobgui.h>
#include <stdlib.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

enum {
  WIDTH_COLUMN,
  HEIGHT_COLUMN,
  AREA_COLUMN,
  SQUARE_COLUMN
};

static void
format_number (BobguiTreeViewColumn *col,
               BobguiCellRenderer   *cell,
               BobguiTreeModel      *model,
               BobguiTreeIter       *iter,
               gpointer           data)
{
  int num;
  char *text;

  bobgui_tree_model_get (model, iter, GPOINTER_TO_INT (data), &num, -1);
  text = g_strdup_printf ("%d", num);
  g_object_set (cell, "text", text, NULL);
  g_free (text);
}

static void
filter_modify_func (BobguiTreeModel *model,
                    BobguiTreeIter  *iter,
                    GValue       *value,
                    int           column,
                    gpointer      data)
{
  BobguiTreeModelFilter *filter_model = BOBGUI_TREE_MODEL_FILTER (model);
  int width, height;
  BobguiTreeModel *child_model;
  BobguiTreeIter child_iter;

  child_model = bobgui_tree_model_filter_get_model (filter_model);
  bobgui_tree_model_filter_convert_iter_to_child_iter (filter_model, &child_iter, iter);

  bobgui_tree_model_get (child_model, &child_iter,
                      WIDTH_COLUMN, &width,
                      HEIGHT_COLUMN, &height,
                      -1);

  switch (column)
    {
    case WIDTH_COLUMN:
      g_value_set_int (value, width);
      break;
    case HEIGHT_COLUMN:
      g_value_set_int (value, height);
      break;
    case AREA_COLUMN:
      g_value_set_int (value, width * height);
      break;
    case SQUARE_COLUMN:
      g_value_set_boolean (value, width == height);
      break;
    default:
      g_assert_not_reached ();
    }
}

static gboolean
visible_func (BobguiTreeModel *model,
              BobguiTreeIter  *iter,
              gpointer      data)
{
  int width;

  bobgui_tree_model_get (model, iter,
                      WIDTH_COLUMN, &width,
                      -1);

  return width < 10;
}

static void
cell_edited (BobguiCellRendererSpin *cell,
             const char          *path_string,
             const char          *new_text,
             BobguiListStore        *store)
{
  int val;
  BobguiTreePath *path;
  BobguiTreeIter iter;
  int column;

  path = bobgui_tree_path_new_from_string (path_string);
  bobgui_tree_model_get_iter (BOBGUI_TREE_MODEL (store), &iter, path);
  bobgui_tree_path_free (path);

  column = GPOINTER_TO_INT (g_object_get_data (G_OBJECT (cell), "column"));

  val = atoi (new_text);

  bobgui_list_store_set (store, &iter, column, val, -1);
}

BobguiWidget *
do_filtermodel (BobguiWidget *do_widget)
{
  static BobguiWidget *window;
  BobguiWidget *tree;
  BobguiListStore *store;
  BobguiTreeModel *model;
  BobguiTreeViewColumn *column;
  BobguiCellRenderer *cell;
  GType types[4];

  if (!window)
    {
      BobguiBuilder *builder;

      builder = bobgui_builder_new_from_resource ("/filtermodel/filtermodel.ui");
      window = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "window1"));
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      store = (BobguiListStore*)bobgui_builder_get_object (builder, "liststore1");

      column = (BobguiTreeViewColumn*)bobgui_builder_get_object (builder, "treeviewcolumn1");
      cell = (BobguiCellRenderer*)bobgui_builder_get_object (builder, "cellrenderertext1");
      bobgui_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (WIDTH_COLUMN), NULL);
      g_object_set_data (G_OBJECT (cell), "column", GINT_TO_POINTER (WIDTH_COLUMN));
      g_signal_connect (cell, "edited", G_CALLBACK (cell_edited), store);

      column = (BobguiTreeViewColumn*)bobgui_builder_get_object (builder, "treeviewcolumn2");
      cell = (BobguiCellRenderer*)bobgui_builder_get_object (builder, "cellrenderertext2");
      bobgui_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (HEIGHT_COLUMN), NULL);
      g_object_set_data (G_OBJECT (cell), "column", GINT_TO_POINTER (HEIGHT_COLUMN));
      g_signal_connect (cell, "edited", G_CALLBACK (cell_edited), store);

      column = (BobguiTreeViewColumn*)bobgui_builder_get_object (builder, "treeviewcolumn3");
      cell = (BobguiCellRenderer*)bobgui_builder_get_object (builder, "cellrenderertext3");
      bobgui_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (WIDTH_COLUMN), NULL);

      column = (BobguiTreeViewColumn*)bobgui_builder_get_object (builder, "treeviewcolumn4");
      cell = (BobguiCellRenderer*)bobgui_builder_get_object (builder, "cellrenderertext4");
      bobgui_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (HEIGHT_COLUMN), NULL);

      column = (BobguiTreeViewColumn*)bobgui_builder_get_object (builder, "treeviewcolumn5");
      cell = (BobguiCellRenderer*)bobgui_builder_get_object (builder, "cellrenderertext5");
      bobgui_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (AREA_COLUMN), NULL);

      column = (BobguiTreeViewColumn*)bobgui_builder_get_object (builder, "treeviewcolumn6");
      cell = (BobguiCellRenderer*)bobgui_builder_get_object (builder, "cellrendererpixbuf1");
      bobgui_tree_view_column_add_attribute (column, cell, "visible", SQUARE_COLUMN);

      tree = (BobguiWidget*)bobgui_builder_get_object (builder, "treeview2");

      types[WIDTH_COLUMN] = G_TYPE_INT;
      types[HEIGHT_COLUMN] = G_TYPE_INT;
      types[AREA_COLUMN] = G_TYPE_INT;
      types[SQUARE_COLUMN] = G_TYPE_BOOLEAN;
      model = bobgui_tree_model_filter_new (BOBGUI_TREE_MODEL (store), NULL);
      bobgui_tree_model_filter_set_modify_func (BOBGUI_TREE_MODEL_FILTER (model),
                                             G_N_ELEMENTS (types), types,
                                             filter_modify_func, NULL, NULL);

      bobgui_tree_view_set_model (BOBGUI_TREE_VIEW (tree), model);

      column = (BobguiTreeViewColumn*)bobgui_builder_get_object (builder, "treeviewcolumn7");
      cell = (BobguiCellRenderer*)bobgui_builder_get_object (builder, "cellrenderertext6");
      bobgui_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (WIDTH_COLUMN), NULL);

      column = (BobguiTreeViewColumn*)bobgui_builder_get_object (builder, "treeviewcolumn8");
      cell = (BobguiCellRenderer*)bobgui_builder_get_object (builder, "cellrenderertext7");
      bobgui_tree_view_column_set_cell_data_func (column, cell,
                                               format_number, GINT_TO_POINTER (HEIGHT_COLUMN), NULL);

      tree = (BobguiWidget*)bobgui_builder_get_object (builder, "treeview3");

      model = bobgui_tree_model_filter_new (BOBGUI_TREE_MODEL (store), NULL);
      bobgui_tree_model_filter_set_visible_func (BOBGUI_TREE_MODEL_FILTER (model),
                                              visible_func, NULL, NULL);
      bobgui_tree_view_set_model (BOBGUI_TREE_VIEW (tree), model);

      g_object_unref (builder);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
