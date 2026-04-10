/* Tree View/List Store
 *
 * The BobguiListStore is used to store data in list form, to be used
 * later on by a BobguiTreeView to display it. This demo builds a
 * simple BobguiListStore and displays it.
 */

#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiWidget *window = NULL;
static BobguiTreeModel *model = NULL;
static guint timeout = 0;

typedef struct
{
  const gboolean  fixed;
  const guint     number;
  const char     *severity;
  const char     *description;
} Bug;

enum
{
  COLUMN_FIXED,
  COLUMN_NUMBER,
  COLUMN_SEVERITY,
  COLUMN_DESCRIPTION,
  COLUMN_PULSE,
  COLUMN_ICON,
  COLUMN_ACTIVE,
  COLUMN_SENSITIVE,
  NUM_COLUMNS
};

static Bug bugs[] =
{
  { FALSE, 60482, "Normal",     "scrollable notebooks and hidden tabs" },
  { FALSE, 60620, "Critical",   "gdk_surface_clear_area (gdksurface-win32.c) is not thread-safe" },
  { FALSE, 50214, "Major",      "Xft support does not clean up correctly" },
  { TRUE,  52877, "Major",      "BobguiFileSelection needs a refresh method. " },
  { FALSE, 56070, "Normal",     "Can't click button after setting in sensitive" },
  { TRUE,  56355, "Normal",     "BobguiLabel - Not all changes propagate correctly" },
  { FALSE, 50055, "Normal",     "Rework width/height computations for TreeView" },
  { FALSE, 58278, "Normal",     "bobgui_dialog_set_response_sensitive () doesn't work" },
  { FALSE, 55767, "Normal",     "Getters for all setters" },
  { FALSE, 56925, "Normal",     "Bobguicalender size" },
  { FALSE, 56221, "Normal",     "Selectable label needs right-click copy menu" },
  { TRUE,  50939, "Normal",     "Add shift clicking to BobguiTextView" },
  { FALSE, 6112,  "Enhancement","netscape-like collapsible toolbars" },
  { FALSE, 1,     "Normal",     "First bug :=)" },
};

static gboolean
spinner_timeout (gpointer data)
{
  BobguiTreeIter iter;
  guint pulse;

  if (model == NULL)
    return G_SOURCE_REMOVE;

  bobgui_tree_model_get_iter_first (model, &iter);
  bobgui_tree_model_get (model, &iter,
                      COLUMN_PULSE, &pulse,
                      -1);
  if (pulse == G_MAXUINT)
    pulse = 0;
  else
    pulse++;

  bobgui_list_store_set (BOBGUI_LIST_STORE (model),
                      &iter,
                      COLUMN_PULSE, pulse,
                      COLUMN_ACTIVE, TRUE,
                      -1);

  return G_SOURCE_CONTINUE;
}

static BobguiTreeModel *
create_model (void)
{
  int i = 0;
  BobguiListStore *store;
  BobguiTreeIter iter;

  /* create list store */
  store = bobgui_list_store_new (NUM_COLUMNS,
                              G_TYPE_BOOLEAN,
                              G_TYPE_UINT,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_UINT,
                              G_TYPE_STRING,
                              G_TYPE_BOOLEAN,
                              G_TYPE_BOOLEAN);

  /* add data to the list store */
  for (i = 0; i < G_N_ELEMENTS (bugs); i++)
    {
      const char *icon_name;
      gboolean sensitive;

      if (i == 1 || i == 3)
        icon_name = "battery-level-10-charging-symbolic";
      else
        icon_name = NULL;
      if (i == 3)
        sensitive = FALSE;
      else
        sensitive = TRUE;
      bobgui_list_store_append (store, &iter);
      bobgui_list_store_set (store, &iter,
                          COLUMN_FIXED, bugs[i].fixed,
                          COLUMN_NUMBER, bugs[i].number,
                          COLUMN_SEVERITY, bugs[i].severity,
                          COLUMN_DESCRIPTION, bugs[i].description,
                          COLUMN_PULSE, 0,
                          COLUMN_ICON, icon_name,
                          COLUMN_ACTIVE, FALSE,
                          COLUMN_SENSITIVE, sensitive,
                          -1);
    }

  return BOBGUI_TREE_MODEL (store);
}

static void
fixed_toggled (BobguiCellRendererToggle *cell,
               char                  *path_str,
               gpointer               data)
{
  BobguiTreeModel *tree_model = (BobguiTreeModel *)data;
  BobguiTreeIter  iter;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_str);
  gboolean fixed;

  /* get toggled iter */
  bobgui_tree_model_get_iter (tree_model, &iter, path);
  bobgui_tree_model_get (tree_model, &iter, COLUMN_FIXED, &fixed, -1);

  /* do something with the value */
  fixed ^= 1;

  /* set new value */
  bobgui_list_store_set (BOBGUI_LIST_STORE (tree_model), &iter, COLUMN_FIXED, fixed, -1);

  /* clean up */
  bobgui_tree_path_free (path);
}

static void
add_columns (BobguiTreeView *treeview)
{
  BobguiCellRenderer *renderer;
  BobguiTreeViewColumn *column;

  /* column for fixed toggles */
  renderer = bobgui_cell_renderer_toggle_new ();
  g_signal_connect (renderer, "toggled",
                    G_CALLBACK (fixed_toggled), model);

  column = bobgui_tree_view_column_new_with_attributes ("Fixed?",
                                                     renderer,
                                                     "active", COLUMN_FIXED,
                                                     NULL);

  /* set this column to a fixed sizing (of 50 pixels) */
  bobgui_tree_view_column_set_sizing (BOBGUI_TREE_VIEW_COLUMN (column),
                                   BOBGUI_TREE_VIEW_COLUMN_FIXED);
  bobgui_tree_view_column_set_fixed_width (BOBGUI_TREE_VIEW_COLUMN (column), 50);
  bobgui_tree_view_append_column (treeview, column);

  /* column for bug numbers */
  renderer = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("Bug number",
                                                     renderer,
                                                     "text",
                                                     COLUMN_NUMBER,
                                                     NULL);
  bobgui_tree_view_column_set_sort_column_id (column, COLUMN_NUMBER);
  bobgui_tree_view_append_column (treeview, column);

  /* column for severities */
  renderer = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("Severity",
                                                     renderer,
                                                     "text",
                                                     COLUMN_SEVERITY,
                                                     NULL);
  bobgui_tree_view_column_set_sort_column_id (column, COLUMN_SEVERITY);
  bobgui_tree_view_append_column (treeview, column);

  /* column for description */
  renderer = bobgui_cell_renderer_text_new ();
  column = bobgui_tree_view_column_new_with_attributes ("Description",
                                                     renderer,
                                                     "text",
                                                     COLUMN_DESCRIPTION,
                                                     NULL);
  bobgui_tree_view_column_set_sort_column_id (column, COLUMN_DESCRIPTION);
  bobgui_tree_view_append_column (treeview, column);

  /* column for spinner */
  renderer = bobgui_cell_renderer_spinner_new ();
  column = bobgui_tree_view_column_new_with_attributes ("Spinning",
                                                     renderer,
                                                     "pulse",
                                                     COLUMN_PULSE,
                                                     "active",
                                                     COLUMN_ACTIVE,
                                                     NULL);
  bobgui_tree_view_column_set_sort_column_id (column, COLUMN_PULSE);
  bobgui_tree_view_append_column (treeview, column);

  /* column for symbolic icon */
  renderer = bobgui_cell_renderer_pixbuf_new ();
  column = bobgui_tree_view_column_new_with_attributes ("Symbolic icon",
                                                     renderer,
                                                     "icon-name",
                                                     COLUMN_ICON,
                                                     "sensitive",
                                                     COLUMN_SENSITIVE,
                                                     NULL);
  bobgui_tree_view_column_set_sort_column_id (column, COLUMN_ICON);
  bobgui_tree_view_append_column (treeview, column);
}

static gboolean
window_closed (BobguiWidget *widget,
               GdkEvent  *event,
               gpointer   user_data)
{
  model = NULL;
  window = NULL;
  if (timeout != 0)
    {
      g_source_remove (timeout);
      timeout = 0;
    }
  return FALSE;
}

BobguiWidget *
do_list_store (BobguiWidget *do_widget)
{
  if (!window)
    {
      BobguiWidget *vbox;
      BobguiWidget *label;
      BobguiWidget *sw;
      BobguiWidget *treeview;

      /* create window, etc */
      window = bobgui_window_new ();
      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "List Store");
      g_object_add_weak_pointer (G_OBJECT (window), (gpointer *)&window);

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 8);
      bobgui_widget_set_margin_start (vbox, 8);
      bobgui_widget_set_margin_end (vbox, 8);
      bobgui_widget_set_margin_top (vbox, 8);
      bobgui_widget_set_margin_bottom (vbox, 8);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      label = bobgui_label_new ("This is the bug list (note: not based on real data, it would be nice to have a nice ODBC interface to bugzilla or so, though).");
      bobgui_box_append (BOBGUI_BOX (vbox), label);

      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_NEVER,
                                      BOBGUI_POLICY_AUTOMATIC);
      bobgui_box_append (BOBGUI_BOX (vbox), sw);

      /* create tree model */
      model = create_model ();

      /* create tree view */
      treeview = bobgui_tree_view_new_with_model (model);
      bobgui_widget_set_vexpand (treeview, TRUE);
      bobgui_tree_view_set_search_column (BOBGUI_TREE_VIEW (treeview),
                                       COLUMN_DESCRIPTION);

      g_object_unref (model);

      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), treeview);

      /* add columns to the tree view */
      add_columns (BOBGUI_TREE_VIEW (treeview));

      /* finish & show */
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 280, 250);
      g_signal_connect (window, "destroy", G_CALLBACK (window_closed), NULL);
    }

  if (!bobgui_widget_get_visible (window))
    {
      bobgui_widget_set_visible (window, TRUE);
      if (timeout == 0) {
        /* FIXME this should use the animation-duration instead */
        timeout = g_timeout_add (80, spinner_timeout, NULL);
      }
    }
  else
    {
      bobgui_window_destroy (BOBGUI_WINDOW (window));
      window = NULL;
      if (timeout != 0)
        {
          g_source_remove (timeout);
          timeout = 0;
        }
    }

  return window;
}
