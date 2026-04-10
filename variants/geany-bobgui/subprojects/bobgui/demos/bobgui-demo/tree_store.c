/* Tree View/Tree Store
 *
 * The BobguiTreeStore is used to store data in tree form, to be
 * used later on by a BobguiTreeView to display it. This demo builds
 * a simple BobguiTreeStore and displays it. If you're new to the
 * BobguiTreeView widgets and associates, look into the BobguiListStore
 * example first.
 *
 */

#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

/* TreeItem structure */
typedef struct _TreeItem TreeItem;
struct _TreeItem
{
  const char     *label;
  gboolean        alex;
  gboolean        havoc;
  gboolean        tim;
  gboolean        owen;
  gboolean        dave;
  gboolean        world_holiday; /* shared by the European hackers */
  TreeItem       *children;
};

/* columns */
enum
{
  HOLIDAY_NAME_COLUMN = 0,
  ALEX_COLUMN,
  HAVOC_COLUMN,
  TIM_COLUMN,
  OWEN_COLUMN,
  DAVE_COLUMN,

  VISIBLE_COLUMN,
  WORLD_COLUMN,
  NUM_COLUMNS
};

/* tree data */
static TreeItem january[] =
{
  {"New Years Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  {"Presidential Inauguration", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  {"Martin Luther King Jr. day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem february[] =
{
  { "Presidents' Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { "Groundhog Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Valentine's Day", FALSE, FALSE, FALSE, FALSE, TRUE, TRUE, NULL },
  { NULL }
};

static TreeItem march[] =
{
  { "National Tree Planting Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "St Patrick's Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { NULL }
};
static TreeItem april[] =
{
  { "April Fools' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Army Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Earth Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Administrative Professionals' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem may[] =
{
  { "Nurses' Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "National Day of Prayer", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Mothers' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Armed Forces Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Memorial Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeItem june[] =
{
  { "June Fathers' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Juneteenth (Liberation Day)", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Flag Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem july[] =
{
  { "Parents' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Independence Day", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem august[] =
{
  { "Air Force Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Coast Guard Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Friendship Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem september[] =
{
  { "Grandparents' Day", FALSE, FALSE, FALSE, FALSE, FALSE, TRUE, NULL },
  { "Citizenship Day or Constitution Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Labor Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { NULL }
};

static TreeItem october[] =
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

static TreeItem november[] =
{
  { "Marine Corps Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Veterans' Day", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { "Thanksgiving", FALSE, TRUE, FALSE, TRUE, FALSE, FALSE, NULL },
  { NULL }
};

static TreeItem december[] =
{
  { "Pearl Harbor Remembrance Day", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { "Christmas", TRUE, TRUE, TRUE, TRUE, FALSE, TRUE, NULL },
  { "Kwanzaa", FALSE, FALSE, FALSE, FALSE, FALSE, FALSE, NULL },
  { NULL }
};


static TreeItem toplevel[] =
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


static BobguiTreeModel *
create_model (void)
{
  BobguiTreeStore *model;
  BobguiTreeIter iter;
  TreeItem *month = toplevel;

  /* create tree store */
  model = bobgui_tree_store_new (NUM_COLUMNS,
                              G_TYPE_STRING,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN);

  /* add data to the tree store */
  while (month->label)
    {
      TreeItem *holiday = month->children;

      bobgui_tree_store_append (model, &iter, NULL);
      bobgui_tree_store_set (model, &iter,
                          HOLIDAY_NAME_COLUMN, month->label,
                          ALEX_COLUMN, FALSE,
                          HAVOC_COLUMN, FALSE,
                          TIM_COLUMN, FALSE,
                          OWEN_COLUMN, FALSE,
                          DAVE_COLUMN, FALSE,
                          VISIBLE_COLUMN, FALSE,
                          WORLD_COLUMN, FALSE,
                          -1);

      /* add children */
      while (holiday->label)
        {
          BobguiTreeIter child_iter;

          bobgui_tree_store_append (model, &child_iter, &iter);
          bobgui_tree_store_set (model, &child_iter,
                              HOLIDAY_NAME_COLUMN, holiday->label,
                              ALEX_COLUMN, holiday->alex,
                              HAVOC_COLUMN, holiday->havoc,
                              TIM_COLUMN, holiday->tim,
                              OWEN_COLUMN, holiday->owen,
                              DAVE_COLUMN, holiday->dave,
                              VISIBLE_COLUMN, TRUE,
                              WORLD_COLUMN, holiday->world_holiday,
                              -1);

          holiday++;
        }

      month++;
    }

  return BOBGUI_TREE_MODEL (model);
}

static void
item_toggled (BobguiCellRendererToggle *cell,
              char                  *path_str,
              gpointer               data)
{
  BobguiTreeModel *model = (BobguiTreeModel *)data;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_str);
  BobguiTreeIter iter;
  gboolean toggle_item;

  int *column;

  column = g_object_get_data (G_OBJECT (cell), "column");

  /* get toggled iter */
  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_tree_model_get (model, &iter, column, &toggle_item, -1);

  /* do something with the value */
  toggle_item ^= 1;

  /* set new value */
  bobgui_tree_store_set (BOBGUI_TREE_STORE (model), &iter, column,
                      toggle_item, -1);

  /* clean up */
  bobgui_tree_path_free (path);
}

static void
add_columns (BobguiTreeView *treeview)
{
  int col_offset;
  BobguiCellRenderer *renderer;
  BobguiTreeViewColumn *column;
  BobguiTreeModel *model = bobgui_tree_view_get_model (treeview);

  /* column for holiday names */
  renderer = bobgui_cell_renderer_text_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);

  col_offset = bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (treeview),
                                                            -1, "Holiday",
                                                            renderer, "text",
                                                            HOLIDAY_NAME_COLUMN,
                                                            NULL);
  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (treeview), col_offset - 1);
  bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (column), TRUE);

  /* alex column */
  renderer = bobgui_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (G_OBJECT (renderer), "column", (int *)ALEX_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (treeview),
                                                            -1, "Alex",
                                                            renderer,
                                                            "active",
                                                            ALEX_COLUMN,
                                                            "visible",
                                                            VISIBLE_COLUMN,
                                                            "activatable",
                                                            WORLD_COLUMN, NULL);

  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (treeview), col_offset - 1);
  bobgui_tree_view_column_set_sizing (BOBGUI_TREE_VIEW_COLUMN (column),
                                   BOBGUI_TREE_VIEW_COLUMN_FIXED);
  bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (column), TRUE);

  /* havoc column */
  renderer = bobgui_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (G_OBJECT (renderer), "column", (int *)HAVOC_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (treeview),
                                                            -1, "Havoc",
                                                            renderer,
                                                            "active",
                                                            HAVOC_COLUMN,
                                                            "visible",
                                                            VISIBLE_COLUMN,
                                                            NULL);

  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (treeview), col_offset - 1);
  bobgui_tree_view_column_set_sizing (BOBGUI_TREE_VIEW_COLUMN (column),
                                   BOBGUI_TREE_VIEW_COLUMN_FIXED);
  bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (column), TRUE);

  /* tim column */
  renderer = bobgui_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (G_OBJECT (renderer), "column", (int *)TIM_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (treeview),
                                                            -1, "Tim",
                                                            renderer,
                                                            "active",
                                                            TIM_COLUMN,
                                                            "visible",
                                                            VISIBLE_COLUMN,
                                                            "activatable",
                                                            WORLD_COLUMN, NULL);

  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (treeview), col_offset - 1);
  bobgui_tree_view_column_set_sizing (BOBGUI_TREE_VIEW_COLUMN (column),
                                   BOBGUI_TREE_VIEW_COLUMN_FIXED);
  bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (column), TRUE);

  /* owen column */
  renderer = bobgui_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (G_OBJECT (renderer), "column", (int *)OWEN_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (treeview),
                                                            -1, "Owen",
                                                            renderer,
                                                            "active",
                                                            OWEN_COLUMN,
                                                            "visible",
                                                            VISIBLE_COLUMN,
                                                            NULL);

  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (treeview), col_offset - 1);
  bobgui_tree_view_column_set_sizing (BOBGUI_TREE_VIEW_COLUMN (column),
                                   BOBGUI_TREE_VIEW_COLUMN_FIXED);
  bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (column), TRUE);

  /* dave column */
  renderer = bobgui_cell_renderer_toggle_new ();
  g_object_set (renderer, "xalign", 0.0, NULL);
  g_object_set_data (G_OBJECT (renderer), "column", (int *)DAVE_COLUMN);

  g_signal_connect (renderer, "toggled", G_CALLBACK (item_toggled), model);

  col_offset = bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (treeview),
                                                            -1, "Dave",
                                                            renderer,
                                                            "active",
                                                            DAVE_COLUMN,
                                                            "visible",
                                                            VISIBLE_COLUMN,
                                                            NULL);

  column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (treeview), col_offset - 1);
  bobgui_tree_view_column_set_sizing (BOBGUI_TREE_VIEW_COLUMN (column),
                                   BOBGUI_TREE_VIEW_COLUMN_FIXED);
  bobgui_tree_view_column_set_clickable (BOBGUI_TREE_VIEW_COLUMN (column), TRUE);
}

BobguiWidget *
do_tree_store (BobguiWidget *do_widget)
{
  static BobguiWidget *window = NULL;

  if (!window)
    {
      BobguiWidget *vbox;
      BobguiWidget *sw;
      BobguiWidget *treeview;
      BobguiTreeModel *model;

      /* create window, etc */
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Tree Store");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
      bobgui_widget_set_margin_start (vbox, 8);
      bobgui_widget_set_margin_end (vbox, 8);
      bobgui_widget_set_margin_top (vbox, 8);
      bobgui_widget_set_margin_bottom (vbox, 8);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      bobgui_box_append (BOBGUI_BOX (vbox),
                          bobgui_label_new ("Jonathan's Holiday Card Planning Sheet"));

      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_AUTOMATIC,
                                      BOBGUI_POLICY_AUTOMATIC);
      bobgui_box_append (BOBGUI_BOX (vbox), sw);

      /* create model */
      model = create_model ();

      /* create tree view */
      treeview = bobgui_tree_view_new_with_model (model);
      bobgui_widget_set_vexpand (treeview, TRUE);
      g_object_unref (model);
      bobgui_tree_selection_set_mode (bobgui_tree_view_get_selection (BOBGUI_TREE_VIEW (treeview)),
                                   BOBGUI_SELECTION_MULTIPLE);

      add_columns (BOBGUI_TREE_VIEW (treeview));

      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), treeview);

      /* expand all rows after the treeview widget has been realized */
      g_signal_connect (treeview, "realize",
                        G_CALLBACK (bobgui_tree_view_expand_all), NULL);
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 650, 400);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
