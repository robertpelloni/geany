/* testiconview-keynav.c
 * Copyright (C) 2010  Red Hat, Inc.
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
 *
 * Author: Matthias Clasen
 */

/*
 * This example demonstrates how to use the keynav-failed signal to
 * extend arrow keynav over adjacent icon views. This can be used when
 * grouping items.
 */

#include <bobgui/bobgui.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static BobguiTreeModel *
get_model (void)
{
  static BobguiListStore *store;
  BobguiTreeIter iter;

  if (store)
    return (BobguiTreeModel *) g_object_ref (store);

  store = bobgui_list_store_new (1, G_TYPE_STRING);

  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 0, "One", -1);
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 0, "Two", -1);
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 0, "Three", -1);
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 0, "Four", -1);
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 0, "Five", -1);
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 0, "Six", -1);
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 0, "Seven", -1);
  bobgui_list_store_append (store, &iter);
  bobgui_list_store_set (store, &iter, 0, "Eight", -1);

  return (BobguiTreeModel *) store;
}

static gboolean
visible_func (BobguiTreeModel *model,
              BobguiTreeIter  *iter,
              gpointer      data)
{
  gboolean first = GPOINTER_TO_INT (data);
  gboolean visible;
  BobguiTreePath *path;

  path = bobgui_tree_model_get_path (model, iter);

  if (bobgui_tree_path_get_indices (path)[0] < 4)
    visible = first;
  else
    visible = !first;

  bobgui_tree_path_free (path);

  return visible;
}

static BobguiTreeModel *
get_filter_model (gboolean first)
{
  BobguiTreeModelFilter *model;

  model = (BobguiTreeModelFilter *)bobgui_tree_model_filter_new (get_model (), NULL);

  bobgui_tree_model_filter_set_visible_func (model, visible_func, GINT_TO_POINTER (first), NULL);

  return (BobguiTreeModel *) model;
}

static BobguiWidget *
get_view (gboolean first)
{
  BobguiWidget *view;

  view = bobgui_icon_view_new_with_model (get_filter_model (first));
  bobgui_icon_view_set_text_column (BOBGUI_ICON_VIEW (view), 0);
  bobgui_widget_set_size_request (view, 0, -1);

  return view;
}

typedef struct
{
  BobguiWidget *header1;
  BobguiWidget *view1;
  BobguiWidget *header2;
  BobguiWidget *view2;
} Views;

static gboolean
keynav_failed (BobguiWidget        *view,
               BobguiDirectionType  direction,
               Views            *views)
{
  BobguiTreePath *path;
  BobguiTreeModel *model;
  BobguiTreeIter iter;
  int col;
  BobguiTreePath *sel;

  if (view == views->view1 && direction == BOBGUI_DIR_DOWN)
    {
      if (bobgui_icon_view_get_cursor (BOBGUI_ICON_VIEW (views->view1), &path, NULL))
        {
          col = bobgui_icon_view_get_item_column (BOBGUI_ICON_VIEW (views->view1), path);
          bobgui_tree_path_free (path);

          sel = NULL;
          model = bobgui_icon_view_get_model (BOBGUI_ICON_VIEW (views->view2));
          bobgui_tree_model_get_iter_first (model, &iter);
          do {
            path = bobgui_tree_model_get_path (model, &iter);
            if (bobgui_icon_view_get_item_column (BOBGUI_ICON_VIEW (views->view2), path) == col)
              {
                sel = path;
                break;
              }
          } while (bobgui_tree_model_iter_next (model, &iter));

          bobgui_icon_view_set_cursor (BOBGUI_ICON_VIEW (views->view2), sel, NULL, FALSE);
          bobgui_tree_path_free (sel);
        }
      bobgui_widget_grab_focus (views->view2);
      return TRUE;
    }

  if (view == views->view2 && direction == BOBGUI_DIR_UP)
    {
      if (bobgui_icon_view_get_cursor (BOBGUI_ICON_VIEW (views->view2), &path, NULL))
        {
          col = bobgui_icon_view_get_item_column (BOBGUI_ICON_VIEW (views->view2), path);
          bobgui_tree_path_free (path);

          sel = NULL;
          model = bobgui_icon_view_get_model (BOBGUI_ICON_VIEW (views->view1));
          bobgui_tree_model_get_iter_first (model, &iter);
          do {
            path = bobgui_tree_model_get_path (model, &iter);
            if (bobgui_icon_view_get_item_column (BOBGUI_ICON_VIEW (views->view1), path) == col)
              {
                if (sel)
                  bobgui_tree_path_free (sel);
                sel = path;
              }
            else
              bobgui_tree_path_free (path);
          } while (bobgui_tree_model_iter_next (model, &iter));

          bobgui_icon_view_set_cursor (BOBGUI_ICON_VIEW (views->view1), sel, NULL, FALSE);
          bobgui_tree_path_free (sel);
        }
      bobgui_widget_grab_focus (views->view1);
      return TRUE;
    }

  return FALSE;
}

static void
focus_changed (BobguiWidget  *view,
               GParamSpec *pspec,
                gpointer   data)
{
  if (bobgui_widget_has_focus (view))
    {
      BobguiTreePath *path;

      if (!bobgui_icon_view_get_cursor (BOBGUI_ICON_VIEW (view), &path, NULL))
        {
          path = bobgui_tree_path_new_from_indices (0, -1);
          bobgui_icon_view_set_cursor (BOBGUI_ICON_VIEW (view), path, NULL, FALSE);
        }

      bobgui_icon_view_select_path (BOBGUI_ICON_VIEW (view), path);
      bobgui_tree_path_free (path);
    }
  else
    {
      bobgui_icon_view_unselect_all (BOBGUI_ICON_VIEW (view));
    }
}

#define CSS \
  "BobguiWindow {\n" \
  "  background-color: @base_color;\n" \
  "}\n"

static void
set_styles (void)
{
  BobguiCssProvider *provider;

  provider = bobgui_css_provider_new ();

  bobgui_css_provider_load_from_data (provider, CSS, -1);

  bobgui_style_context_add_provider_for_display (gdk_display_get_default (),
                                              BOBGUI_STYLE_PROVIDER (provider),
                                              BOBGUI_STYLE_PROVIDER_PRIORITY_APPLICATION);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *vbox;
  Views views;

  bobgui_init ();

  set_styles ();

  window = bobgui_window_new ();
  vbox = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  bobgui_window_set_child (BOBGUI_WINDOW (window), vbox);

  views.header1 = g_object_new (BOBGUI_TYPE_LABEL,
                                "label", "<b>Group 1</b>",
                                "use-markup", TRUE,
                                "xalign", 0.0,
                                NULL);
  views.view1 = get_view (TRUE);
  views.header2 = g_object_new (BOBGUI_TYPE_LABEL,
                                "label", "<b>Group 2</b>",
                                "use-markup", TRUE,
                                "xalign", 0.0,
                                NULL);
  views.view2 = get_view (FALSE);

  g_signal_connect (views.view1, "keynav-failed", G_CALLBACK (keynav_failed), &views);
  g_signal_connect (views.view2, "keynav-failed", G_CALLBACK (keynav_failed), &views);
  g_signal_connect (views.view1, "notify::has-focus", G_CALLBACK (focus_changed), &views);
  g_signal_connect (views.view2, "notify::has-focus", G_CALLBACK (focus_changed), &views);

  bobgui_box_append (BOBGUI_BOX (vbox), views.header1);
  bobgui_box_append (BOBGUI_BOX (vbox), views.view1);
  bobgui_box_append (BOBGUI_BOX (vbox), views.header2);
  bobgui_box_append (BOBGUI_BOX (vbox), views.view2);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}

