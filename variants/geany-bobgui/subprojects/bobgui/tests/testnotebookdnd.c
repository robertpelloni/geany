/* -*- Mode: C; c-file-style: "gnu"; tab-width: 8 -*- */
/*
 * BOBGUI - The GIMP Toolkit
 * Copyright (C) 2006  Carlos Garnacho Parro <carlosg@gnome.org>
 *
 * All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */
#include <bobgui/bobgui.h>

static gconstpointer GROUP_A = "GROUP_A";
static gconstpointer GROUP_B = "GROUP_B";

const char *tabs1 [] = {
  "aaaaaaaaaa",
  "bbbbbbbbbb",
  "cccccccccc",
  "dddddddddd",
  NULL
};

const char *tabs2 [] = {
  "1",
  "2",
  "3",
  "4",
  "55555",
  NULL
};

const char *tabs3 [] = {
  "foo",
  "bar",
  NULL
};

const char *tabs4 [] = {
  "beer",
  "water",
  "lemonade",
  "coffee",
  "tea",
  NULL
};

static BobguiNotebook*
window_creation_function (BobguiNotebook *source_notebook,
                          BobguiWidget   *child,
                          int          x,
                          int          y,
                          gpointer     data)
{
  BobguiWidget *window, *notebook;

  window = bobgui_window_new ();
  notebook = bobgui_notebook_new ();
  g_signal_connect (notebook, "create-window",
                    G_CALLBACK (window_creation_function), NULL);

  bobgui_notebook_set_group_name (BOBGUI_NOTEBOOK (notebook),
                               bobgui_notebook_get_group_name (source_notebook));

  bobgui_window_set_child (BOBGUI_WINDOW (window), notebook);

  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 300);
  bobgui_window_present (BOBGUI_WINDOW (window));

  return BOBGUI_NOTEBOOK (notebook);
}

static void
on_page_reordered (BobguiNotebook *notebook, BobguiWidget *child, guint page_num, gpointer data)
{
  g_print ("page %d reordered\n", page_num);
}

static gboolean
remove_in_idle (gpointer data)
{
  BobguiNotebookPage *page = data;
  BobguiWidget *child = bobgui_notebook_page_get_child (page);
  BobguiWidget *parent = bobgui_widget_get_ancestor (child, BOBGUI_TYPE_NOTEBOOK);
  BobguiWidget *tab_label;

  tab_label = bobgui_notebook_get_tab_label (BOBGUI_NOTEBOOK (parent), child);
  g_print ("Removing tab: %s\n", bobgui_label_get_text (BOBGUI_LABEL (tab_label)));
  bobgui_box_remove (BOBGUI_BOX (parent), child);

  return G_SOURCE_REMOVE;
}

static gboolean
on_button_drag_drop (BobguiDropTarget *dest,
                     const GValue  *value,
                     double         x,
                     double         y,
                     gpointer       user_data)
{
  BobguiNotebookPage *page;

  page = g_value_get_object (value);
  g_idle_add (remove_in_idle, page);

  return TRUE;
}

static void
action_clicked_cb (BobguiWidget *button,
                   BobguiWidget *notebook)
{
  BobguiWidget *page, *title;

  page = bobgui_entry_new ();
  bobgui_editable_set_text (BOBGUI_EDITABLE (page), "Addition");

  title = bobgui_label_new ("Addition");

  bobgui_notebook_append_page (BOBGUI_NOTEBOOK (notebook), page, title);
  bobgui_notebook_set_tab_reorderable (BOBGUI_NOTEBOOK (notebook), page, TRUE);
  bobgui_notebook_set_tab_detachable (BOBGUI_NOTEBOOK (notebook), page, TRUE);
}

static BobguiWidget*
create_notebook (const char     **labels,
                 const char      *group,
                 BobguiPositionType   pos)
{
  BobguiWidget *notebook, *title, *page, *action_widget;

  notebook = bobgui_notebook_new ();
  bobgui_widget_set_vexpand (notebook, TRUE);
  bobgui_widget_set_hexpand (notebook, TRUE);

  action_widget = bobgui_button_new_from_icon_name ("list-add-symbolic");
  g_signal_connect (action_widget, "clicked", G_CALLBACK (action_clicked_cb), notebook);
  bobgui_notebook_set_action_widget (BOBGUI_NOTEBOOK (notebook), action_widget, BOBGUI_PACK_END);

  g_signal_connect (notebook, "create-window",
                    G_CALLBACK (window_creation_function), NULL);

  bobgui_notebook_set_tab_pos (BOBGUI_NOTEBOOK (notebook), pos);
  bobgui_notebook_set_scrollable (BOBGUI_NOTEBOOK (notebook), TRUE);
  bobgui_notebook_set_group_name (BOBGUI_NOTEBOOK (notebook), group);

  while (*labels)
    {
      page = bobgui_entry_new ();
      bobgui_editable_set_text (BOBGUI_EDITABLE (page), *labels);

      title = bobgui_label_new (*labels);

      bobgui_notebook_append_page (BOBGUI_NOTEBOOK (notebook), page, title);
      bobgui_notebook_set_tab_reorderable (BOBGUI_NOTEBOOK (notebook), page, TRUE);
      bobgui_notebook_set_tab_detachable (BOBGUI_NOTEBOOK (notebook), page, TRUE);

      labels++;
    }

  g_signal_connect (BOBGUI_NOTEBOOK (notebook), "page-reordered",
                    G_CALLBACK (on_page_reordered), NULL);
  return notebook;
}

static BobguiWidget*
create_notebook_non_dragable_content (const char      **labels,
                                      const char       *group,
                                      BobguiPositionType   pos)
{
  BobguiWidget *notebook, *title, *page, *action_widget;

  notebook = bobgui_notebook_new ();
  bobgui_widget_set_vexpand (notebook, TRUE);
  bobgui_widget_set_hexpand (notebook, TRUE);

  action_widget = bobgui_button_new_from_icon_name ("list-add-symbolic");
  g_signal_connect (action_widget, "clicked", G_CALLBACK (action_clicked_cb), notebook);
  bobgui_notebook_set_action_widget (BOBGUI_NOTEBOOK (notebook), action_widget, BOBGUI_PACK_END);

  g_signal_connect (notebook, "create-window",
                    G_CALLBACK (window_creation_function), NULL);

  bobgui_notebook_set_tab_pos (BOBGUI_NOTEBOOK (notebook), pos);
  bobgui_notebook_set_scrollable (BOBGUI_NOTEBOOK (notebook), TRUE);
  bobgui_notebook_set_group_name (BOBGUI_NOTEBOOK (notebook), group);

  while (*labels)
    {
      BobguiWidget *button;
      button = bobgui_button_new_with_label ("example content");
      /* Use BobguiListBox since it bubbles up motion notify event, which can
       * experience more issues than BobguiBox. */
      page = bobgui_list_box_new ();
      bobgui_box_append (BOBGUI_BOX (page), button);

      button = bobgui_button_new_with_label ("row 2");
      bobgui_box_append (BOBGUI_BOX (page), button);

      button = bobgui_button_new_with_label ("third row");
      bobgui_box_append (BOBGUI_BOX (page), button);

      title = bobgui_label_new (*labels);

      bobgui_notebook_append_page (BOBGUI_NOTEBOOK (notebook), page, title);
      bobgui_notebook_set_tab_reorderable (BOBGUI_NOTEBOOK (notebook), page, TRUE);
      bobgui_notebook_set_tab_detachable (BOBGUI_NOTEBOOK (notebook), page, TRUE);

      labels++;
    }

  g_signal_connect (BOBGUI_NOTEBOOK (notebook), "page-reordered",
                    G_CALLBACK (on_page_reordered), NULL);
  return notebook;
}

static BobguiWidget*
create_notebook_with_notebooks (const char      **labels,
                                const char       *group,
                                BobguiPositionType   pos)
{
  BobguiWidget *notebook, *title, *page;

  notebook = bobgui_notebook_new ();
  g_signal_connect (notebook, "create-window",
                    G_CALLBACK (window_creation_function), NULL);

  bobgui_notebook_set_tab_pos (BOBGUI_NOTEBOOK (notebook), pos);
  bobgui_notebook_set_scrollable (BOBGUI_NOTEBOOK (notebook), TRUE);
  bobgui_notebook_set_group_name (BOBGUI_NOTEBOOK (notebook), group);

  while (*labels)
    {
      page = create_notebook (labels, group, pos);
      bobgui_notebook_popup_enable (BOBGUI_NOTEBOOK (page));

      title = bobgui_label_new (*labels);

      bobgui_notebook_append_page (BOBGUI_NOTEBOOK (notebook), page, title);
      bobgui_notebook_set_tab_reorderable (BOBGUI_NOTEBOOK (notebook), page, TRUE);
      bobgui_notebook_set_tab_detachable (BOBGUI_NOTEBOOK (notebook), page, TRUE);

      labels++;
    }

  g_signal_connect (BOBGUI_NOTEBOOK (notebook), "page-reordered",
                    G_CALLBACK (on_page_reordered), NULL);
  return notebook;
}

static BobguiWidget*
create_trash_button (void)
{
  BobguiWidget *button;
  BobguiDropTarget *dest;

  button = bobgui_button_new_with_mnemonic ("_Delete");

  dest = bobgui_drop_target_new (BOBGUI_TYPE_NOTEBOOK_PAGE, GDK_ACTION_MOVE);
  g_signal_connect (dest, "drop", G_CALLBACK (on_button_drag_drop), NULL);
  bobgui_widget_add_controller (button, BOBGUI_EVENT_CONTROLLER (dest));

  return button;
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
main (int argc, char *argv[])
{
  BobguiWidget *window, *grid;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  grid = bobgui_grid_new ();

  bobgui_grid_attach (BOBGUI_GRID (grid),
                   create_notebook_non_dragable_content (tabs1, GROUP_A, BOBGUI_POS_TOP),
                   0, 0, 1, 1);

  bobgui_grid_attach (BOBGUI_GRID (grid),
                   create_notebook (tabs2, GROUP_B, BOBGUI_POS_BOTTOM),
                   0, 1, 1, 1);

  bobgui_grid_attach (BOBGUI_GRID (grid),
                   create_notebook (tabs3, GROUP_B, BOBGUI_POS_LEFT),
                   1, 0, 1, 1);

  bobgui_grid_attach (BOBGUI_GRID (grid),
                   create_notebook_with_notebooks (tabs4, GROUP_A, BOBGUI_POS_RIGHT),
                   1, 1, 1, 1);

  bobgui_grid_attach (BOBGUI_GRID (grid),
                   create_trash_button (),
                   1, 2, 1, 1);

  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 400);

  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
