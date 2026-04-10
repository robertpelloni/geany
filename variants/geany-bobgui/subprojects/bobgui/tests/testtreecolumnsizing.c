/* testtreecolumnsizing.c: Test case for tree view column resizing.
 *
 * Copyright (C) 2008  Kristian Rietveld  <kris@bobgui.org>
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include <bobgui/bobgui.h>
#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

#define NO_EXPAND "No expandable columns"
#define SINGLE_EXPAND "One expandable column"
#define MULTI_EXPAND "Multiple expandable columns"
#define LAST_EXPAND "Last column is expandable"
#define BORDER_EXPAND "First and last columns are expandable"
#define ALL_EXPAND "All columns are expandable"

#define N_ROWS 10


static BobguiTreeModel *
create_model (void)
{
  int i;
  BobguiListStore *store;

  store = bobgui_list_store_new (5,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING,
                              G_TYPE_STRING);

  for (i = 0; i < N_ROWS; i++)
    {
      char *str;

      str = g_strdup_printf ("Row %d", i);
      bobgui_list_store_insert_with_values (store, NULL, i,
                                         0, str,
                                         1, "Blah blah blah blah blah",
                                         2, "Less blah",
                                         3, "Medium length",
                                         4, "Eek",
                                         -1);
      g_free (str);
    }

  return BOBGUI_TREE_MODEL (store);
}

static void
toggle_long_content_row (BobguiToggleButton *button,
                         gpointer         user_data)
{
  BobguiTreeModel *model;

  model = bobgui_tree_view_get_model (BOBGUI_TREE_VIEW (user_data));
  if (bobgui_tree_model_iter_n_children (model, NULL) == N_ROWS)
    {
      bobgui_list_store_insert_with_values (BOBGUI_LIST_STORE (model), NULL, N_ROWS,
                                         0, "Very very very very longggggg",
                                         1, "Blah blah blah blah blah",
                                         2, "Less blah",
                                         3, "Medium length",
                                         4, "Eek we make the scrollbar appear",
                                         -1);
    }
  else
    {
      BobguiTreeIter iter;

      bobgui_tree_model_iter_nth_child (model, &iter, NULL, N_ROWS);
      bobgui_list_store_remove (BOBGUI_LIST_STORE (model), &iter);
    }
}

static void
combo_box_changed (BobguiComboBox *combo_box,
                   gpointer     user_data)
{
  char *str;
  GList *list;
  GList *columns;

  str = bobgui_combo_box_text_get_active_text (BOBGUI_COMBO_BOX_TEXT (combo_box));
  if (!str)
    return;

  columns = bobgui_tree_view_get_columns (BOBGUI_TREE_VIEW (user_data));

  if (!strcmp (str, NO_EXPAND))
    {
      for (list = columns; list; list = list->next)
        bobgui_tree_view_column_set_expand (list->data, FALSE);
    }
  else if (!strcmp (str, SINGLE_EXPAND))
    {
      for (list = columns; list; list = list->next)
        {
          if (list->prev && !list->prev->prev)
            /* This is the second column */
            bobgui_tree_view_column_set_expand (list->data, TRUE);
          else
            bobgui_tree_view_column_set_expand (list->data, FALSE);
        }
    }
  else if (!strcmp (str, MULTI_EXPAND))
    {
      for (list = columns; list; list = list->next)
        {
          if (list->prev && !list->prev->prev)
            /* This is the second column */
            bobgui_tree_view_column_set_expand (list->data, TRUE);
          else if (list->prev && !list->prev->prev->prev)
            /* This is the third column */
            bobgui_tree_view_column_set_expand (list->data, TRUE);
          else
            bobgui_tree_view_column_set_expand (list->data, FALSE);
        }
    }
  else if (!strcmp (str, LAST_EXPAND))
    {
      for (list = columns; list->next; list = list->next)
        bobgui_tree_view_column_set_expand (list->data, FALSE);
      /* This is the last column */
      bobgui_tree_view_column_set_expand (list->data, TRUE);
    }
  else if (!strcmp (str, BORDER_EXPAND))
    {
      bobgui_tree_view_column_set_expand (columns->data, TRUE);
      for (list = columns->next; list->next; list = list->next)
        bobgui_tree_view_column_set_expand (list->data, FALSE);
      /* This is the last column */
      bobgui_tree_view_column_set_expand (list->data, TRUE);
    }
  else if (!strcmp (str, ALL_EXPAND))
    {
      for (list = columns; list; list = list->next)
        bobgui_tree_view_column_set_expand (list->data, TRUE);
    }

  g_free (str);
  g_list_free (columns);
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
  int i;
  BobguiWidget *window;
  BobguiWidget *vbox;
  BobguiWidget *combo_box;
  BobguiWidget *sw;
  BobguiWidget *tree_view;
  BobguiWidget *button;
  gboolean done = FALSE;

  bobgui_init ();

  /* Window and box */
  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 640, 480);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  /* Option menu contents */
  combo_box = bobgui_combo_box_text_new ();

  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo_box), NO_EXPAND);
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo_box), SINGLE_EXPAND);
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo_box), MULTI_EXPAND);
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo_box), LAST_EXPAND);
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo_box), BORDER_EXPAND);
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo_box), ALL_EXPAND);

  bobgui_box_append (BOBGUI_BOX (vbox), combo_box);

  /* Scrolled window and tree view */
  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_AUTOMATIC,
                                  BOBGUI_POLICY_AUTOMATIC);
  bobgui_widget_set_vexpand (sw, TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), sw);

  tree_view = bobgui_tree_view_new_with_model (create_model ());
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), tree_view);

  for (i = 0; i < 5; i++)
    {
      BobguiTreeViewColumn *column;

      bobgui_tree_view_insert_column_with_attributes (BOBGUI_TREE_VIEW (tree_view),
                                                   i, "Header",
                                                   bobgui_cell_renderer_text_new (),
                                                   "text", i,
                                                   NULL);

      column = bobgui_tree_view_get_column (BOBGUI_TREE_VIEW (tree_view), i);
      bobgui_tree_view_column_set_resizable (column, TRUE);
    }

  /* Toggle button for long content row */
  button = bobgui_toggle_button_new_with_label ("Toggle long content row");
  g_signal_connect (button, "toggled",
                    G_CALLBACK (toggle_long_content_row), tree_view);
  bobgui_box_append (BOBGUI_BOX (vbox), button);

  /* Set up option menu callback and default item */
  g_signal_connect (combo_box, "changed",
                    G_CALLBACK (combo_box_changed), tree_view);
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo_box), 0);

  /* Done */
  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
