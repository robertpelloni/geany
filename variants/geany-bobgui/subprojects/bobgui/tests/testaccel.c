/* bobguicellrendereraccel.h
 * Copyright (C) 2000  Red Hat, Inc.,  Jonathan Blandford <jrb@redhat.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#include <bobgui/bobgui.h>
#include <gdk/gdkkeysyms.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
accel_edited_callback (BobguiCellRendererText *cell,
                       const char          *path_string,
                       guint                keyval,
                       GdkModifierType      mask,
                       guint                hardware_keycode,
                       gpointer             data)
{
  BobguiTreeModel *model = (BobguiTreeModel *)data;
  BobguiTreePath *path = bobgui_tree_path_new_from_string (path_string);
  BobguiTreeIter iter;

  bobgui_tree_model_get_iter (model, &iter, path);

  g_print ("%u %d %u\n", keyval, mask, hardware_keycode);
  
  bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter,
                      0, (int)mask,
                      1, keyval,
                      2, hardware_keycode,
                      -1);
  bobgui_tree_path_free (path);
}

static void
accel_cleared_callback (BobguiCellRendererText *cell,
                        const char          *path_string,
                        gpointer             data)
{
  BobguiTreeModel *model = (BobguiTreeModel *)data;
  BobguiTreePath *path;
  BobguiTreeIter iter;

  path = bobgui_tree_path_new_from_string (path_string);
  bobgui_tree_model_get_iter (model, &iter, path);
  bobgui_list_store_set (BOBGUI_LIST_STORE (model), &iter, 0, 0, 1, 0, 2, 0, -1);
  bobgui_tree_path_free (path);
}
static BobguiWidget *
key_test (void)
{
  BobguiWidget *window, *sw, *tv;
  BobguiListStore *store;
  BobguiTreeViewColumn *column;
  BobguiCellRenderer *rend;
  int i;
  BobguiWidget *box, *entry;

  /* create window */
  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 400);

  sw = bobgui_scrolled_window_new ();
  bobgui_widget_set_vexpand (sw, TRUE);
  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);
  bobgui_box_append (BOBGUI_BOX (box), sw);

  store = bobgui_list_store_new (3, G_TYPE_INT, G_TYPE_UINT, G_TYPE_UINT);
  tv = bobgui_tree_view_new_with_model (BOBGUI_TREE_MODEL (store));
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), tv);
  column = bobgui_tree_view_column_new ();
  rend = bobgui_cell_renderer_accel_new ();
  g_object_set (G_OBJECT (rend),
                "accel-mode", BOBGUI_CELL_RENDERER_ACCEL_MODE_BOBGUI,
                "editable", TRUE,
                NULL);
  g_signal_connect (G_OBJECT (rend),
                    "accel-edited",
                    G_CALLBACK (accel_edited_callback),
                    store);
  g_signal_connect (G_OBJECT (rend),
                    "accel-cleared",
                    G_CALLBACK (accel_cleared_callback),
                    store);

  bobgui_tree_view_column_pack_start (column, rend,
                                   TRUE);
  bobgui_tree_view_column_set_attributes (column, rend,
                                      "accel-mods", 0,
                                      "accel-key", 1,
                                      "keycode", 2,
                                      NULL);
  bobgui_tree_view_append_column (BOBGUI_TREE_VIEW (tv), column);

  for (i = 0; i < 10; i++)
    {
      BobguiTreeIter iter;

      bobgui_list_store_append (store, &iter);
    }

  entry = bobgui_entry_new ();
  bobgui_box_append (BOBGUI_BOX (box), entry);

  return window;
}

int
main (int argc, char **argv)
{
  BobguiWidget *dialog;
  
  bobgui_init ();

  dialog = key_test ();

  bobgui_window_present (BOBGUI_WINDOW (dialog));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
