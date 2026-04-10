/* Icon View/Icon View Basics
 *
 * The BobguiIconView widget is used to display and manipulate icons.
 * It uses a BobguiTreeModel for data storage, so the list store
 * example might be helpful.
 */

#include <glib/gi18n.h>
#include <bobgui/bobgui.h>
#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiWidget *window = NULL;

#define FOLDER_NAME "/iconview/gnome-fs-directory.png"
#define FILE_NAME "/iconview/gnome-fs-regular.png"

enum
{
  COL_PATH,
  COL_DISPLAY_NAME,
  COL_PIXBUF,
  COL_IS_DIRECTORY,
  NUM_COLS
};


static GdkPixbuf *file_pixbuf, *folder_pixbuf;
char *parent;
BobguiWidget *up_button;

/* Loads the images for the demo and returns whether the operation succeeded */
static void
load_pixbufs (void)
{
  if (file_pixbuf)
    return; /* already loaded earlier */

  file_pixbuf = gdk_pixbuf_new_from_resource (FILE_NAME, NULL);
  /* resources must load successfully */
  g_assert (file_pixbuf);

  folder_pixbuf = gdk_pixbuf_new_from_resource (FOLDER_NAME, NULL);
  g_assert (folder_pixbuf);
}

static void
fill_store (BobguiListStore *store)
{
  GDir *dir;
  const char *name;
  BobguiTreeIter iter;

  /* First clear the store */
  bobgui_list_store_clear (store);

  /* Now go through the directory and extract all the file
   * information */
  dir = g_dir_open (parent, 0, NULL);
  if (!dir)
    return;

  name = g_dir_read_name (dir);
  while (name != NULL)
    {
      char *path, *display_name;
      gboolean is_dir;

      /* We ignore hidden files that start with a '.' */
      if (name[0] != '.')
        {
          path = g_build_filename (parent, name, NULL);

          is_dir = g_file_test (path, G_FILE_TEST_IS_DIR);

          display_name = g_filename_to_utf8 (name, -1, NULL, NULL, NULL);

          bobgui_list_store_append (store, &iter);
          bobgui_list_store_set (store, &iter,
                              COL_PATH, path,
                              COL_DISPLAY_NAME, display_name,
                              COL_IS_DIRECTORY, is_dir,
                              COL_PIXBUF, is_dir ? folder_pixbuf : file_pixbuf,
                              -1);
          g_free (path);
          g_free (display_name);
        }

      name = g_dir_read_name (dir);
    }
  g_dir_close (dir);
}

static int
sort_func (BobguiTreeModel *model,
           BobguiTreeIter  *a,
           BobguiTreeIter  *b,
           gpointer      user_data)
{
  gboolean is_dir_a, is_dir_b;
  char *name_a, *name_b;
  int ret;

  /* We need this function because we want to sort
   * folders before files.
   */


  bobgui_tree_model_get (model, a,
                      COL_IS_DIRECTORY, &is_dir_a,
                      COL_DISPLAY_NAME, &name_a,
                      -1);

  bobgui_tree_model_get (model, b,
                      COL_IS_DIRECTORY, &is_dir_b,
                      COL_DISPLAY_NAME, &name_b,
                      -1);

  if (!is_dir_a && is_dir_b)
    ret = 1;
  else if (is_dir_a && !is_dir_b)
    ret = -1;
  else
    {
      ret = g_utf8_collate (name_a, name_b);
    }

  g_free (name_a);
  g_free (name_b);

  return ret;
}

static BobguiListStore *
create_store (void)
{
  BobguiListStore *store;

  store = bobgui_list_store_new (NUM_COLS,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              GDK_TYPE_PIXBUF,
                              G_TYPE_BOOLEAN);

  /* Set sort column and function */
  bobgui_tree_sortable_set_default_sort_func (BOBGUI_TREE_SORTABLE (store),
                                           sort_func,
                                           NULL, NULL);
  bobgui_tree_sortable_set_sort_column_id (BOBGUI_TREE_SORTABLE (store),
                                        BOBGUI_TREE_SORTABLE_DEFAULT_SORT_COLUMN_ID,
                                        BOBGUI_SORT_ASCENDING);

  return store;
}

static void
item_activated (BobguiIconView *icon_view,
                BobguiTreePath *tree_path,
                gpointer     user_data)
{
  BobguiListStore *store;
  char *path;
  BobguiTreeIter iter;
  gboolean is_dir;

  store = BOBGUI_LIST_STORE (user_data);

  bobgui_tree_model_get_iter (BOBGUI_TREE_MODEL (store),
                           &iter, tree_path);
  bobgui_tree_model_get (BOBGUI_TREE_MODEL (store), &iter,
                      COL_PATH, &path,
                      COL_IS_DIRECTORY, &is_dir,
                      -1);

  if (!is_dir)
    {
      g_free (path);
      return;
    }

  /* Replace parent with path and re-fill the model*/
  g_free (parent);
  parent = path;

  fill_store (store);

  /* Sensitize the up button */
  bobgui_widget_set_sensitive (BOBGUI_WIDGET (up_button), TRUE);
}

static void
up_clicked (BobguiButton *item,
            gpointer   user_data)
{
  BobguiListStore *store;
  char *dir_name;

  store = BOBGUI_LIST_STORE (user_data);

  dir_name = g_path_get_dirname (parent);
  g_free (parent);

  parent = dir_name;

  fill_store (store);

  /* Maybe de-sensitize the up button */
  bobgui_widget_set_sensitive (BOBGUI_WIDGET (up_button),
                            strcmp (parent, "/") != 0);
}

static void
home_clicked (BobguiButton *item,
              gpointer   user_data)
{
  BobguiListStore *store;

  store = BOBGUI_LIST_STORE (user_data);

  g_free (parent);
  parent = g_strdup (g_get_home_dir ());

  fill_store (store);

  /* Sensitize the up button */
  bobgui_widget_set_sensitive (BOBGUI_WIDGET (up_button),
                            TRUE);
}

static void close_window(void)
{
  bobgui_window_destroy (BOBGUI_WINDOW (window));
  window = NULL;

  g_object_unref (file_pixbuf);
  file_pixbuf = NULL;

  g_object_unref (folder_pixbuf);
  folder_pixbuf = NULL;
}

BobguiWidget *
do_iconview (BobguiWidget *do_widget)
{
  if (!window)
    {
      BobguiWidget *sw;
      BobguiWidget *icon_view;
      BobguiListStore *store;
      BobguiWidget *vbox;
      BobguiWidget *tool_bar;
      BobguiWidget *home_button;

      window = bobgui_window_new ();
      bobgui_window_set_default_size (BOBGUI_WINDOW (window), 650, 400);

      bobgui_window_set_display (BOBGUI_WINDOW (window),
                              bobgui_widget_get_display (do_widget));
      bobgui_window_set_title (BOBGUI_WINDOW (window), "Icon View Basics");

      g_signal_connect (window, "destroy",
                        G_CALLBACK (close_window), NULL);

      load_pixbufs ();

      vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
      bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

      tool_bar = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
      bobgui_box_append (BOBGUI_BOX (vbox), tool_bar);

      up_button = bobgui_button_new_with_mnemonic ("_Up");
      bobgui_widget_set_sensitive (BOBGUI_WIDGET (up_button), FALSE);
      bobgui_box_append (BOBGUI_BOX (tool_bar), up_button);

      home_button = bobgui_button_new_with_mnemonic ("_Home");
      bobgui_box_append (BOBGUI_BOX (tool_bar), home_button);


      sw = bobgui_scrolled_window_new ();
      bobgui_scrolled_window_set_has_frame (BOBGUI_SCROLLED_WINDOW (sw), TRUE);
      bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                      BOBGUI_POLICY_AUTOMATIC,
                                      BOBGUI_POLICY_AUTOMATIC);
      bobgui_widget_set_vexpand (sw, TRUE);

      bobgui_box_append (BOBGUI_BOX (vbox), sw);

      /* Create the store and fill it with the contents of '/' */
      parent = g_strdup ("/");
      store = create_store ();
      fill_store (store);

      icon_view = bobgui_icon_view_new_with_model (BOBGUI_TREE_MODEL (store));
      bobgui_icon_view_set_selection_mode (BOBGUI_ICON_VIEW (icon_view),
                                        BOBGUI_SELECTION_MULTIPLE);
      g_object_unref (store);

      /* Connect to the "clicked" signal of the "Up" tool button */
      g_signal_connect (up_button, "clicked",
                        G_CALLBACK (up_clicked), store);

      /* Connect to the "clicked" signal of the "Home" tool button */
      g_signal_connect (home_button, "clicked",
                        G_CALLBACK (home_clicked), store);

      /* We now set which model columns that correspond to the text
       * and pixbuf of each item
       */
      bobgui_icon_view_set_text_column (BOBGUI_ICON_VIEW (icon_view), COL_DISPLAY_NAME);
      bobgui_icon_view_set_pixbuf_column (BOBGUI_ICON_VIEW (icon_view), COL_PIXBUF);

      /* Connect to the "item-activated" signal */
      g_signal_connect (icon_view, "item-activated",
                        G_CALLBACK (item_activated), store);
      bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), icon_view);

      bobgui_widget_grab_focus (icon_view);
    }

  if (!bobgui_widget_get_visible (window))
    bobgui_widget_set_visible (window, TRUE);
  else
    bobgui_window_destroy (BOBGUI_WINDOW (window));

  return window;
}
