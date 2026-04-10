/* testexpand.c
 * Copyright (C) 2010 Havoc Pennington
 *
 * Author:
 *      Havoc Pennington <hp@pobox.com>
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

static void
on_toggle_hexpand (BobguiToggleButton *toggle,
                   void            *data)
{
  g_object_set (toggle,
                "hexpand", bobgui_toggle_button_get_active (toggle),
                NULL);
}

static void
on_toggle_vexpand (BobguiToggleButton *toggle,
                   void            *data)
{
  g_object_set (toggle,
                "vexpand", bobgui_toggle_button_get_active (toggle),
                NULL);
}

static void
create_box_window (void)
{
  BobguiWidget *window;
  BobguiWidget *box1, *box2, *box3;
  BobguiWidget *toggle;
  BobguiWidget *colorbox;

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Boxes");

  box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 0);
  box3 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);

  bobgui_box_append (BOBGUI_BOX (box1),
                      bobgui_label_new ("VBox 1 Top"));
  bobgui_box_append (BOBGUI_BOX (box1),
                      box2);
  bobgui_box_append (BOBGUI_BOX(box1),
                     bobgui_label_new ("VBox 1 Bottom"));

  bobgui_box_append (BOBGUI_BOX (box2),
                      bobgui_label_new ("HBox 2 Left"));
  bobgui_box_append (BOBGUI_BOX (box2),
                      box3);
  bobgui_box_append (BOBGUI_BOX(box2),
                     bobgui_label_new ("HBox 2 Right"));

  bobgui_box_append (BOBGUI_BOX (box3),
                      bobgui_label_new ("VBox 3 Top"));

  colorbox = bobgui_frame_new (NULL);

  toggle = bobgui_toggle_button_new_with_label ("H Expand");
  bobgui_widget_set_halign (toggle, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (toggle, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_margin_start (toggle, 5);
  bobgui_widget_set_margin_end (toggle, 5);
  bobgui_widget_set_margin_top (toggle, 5);
  bobgui_widget_set_margin_bottom (toggle, 5);
  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (on_toggle_hexpand), NULL);
  bobgui_frame_set_child (BOBGUI_FRAME (colorbox), toggle);

  bobgui_box_append (BOBGUI_BOX (box3), colorbox);

  colorbox = bobgui_frame_new (NULL);

  toggle = bobgui_toggle_button_new_with_label ("V Expand");
  bobgui_widget_set_halign (toggle, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (toggle, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_margin_start (toggle, 5);
  bobgui_widget_set_margin_end (toggle, 5);
  bobgui_widget_set_margin_top (toggle, 5);
  bobgui_widget_set_margin_bottom (toggle, 5);
  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (on_toggle_vexpand), NULL);
  bobgui_frame_set_child (BOBGUI_FRAME (colorbox), toggle);
  bobgui_box_append (BOBGUI_BOX (box3), colorbox);
  bobgui_box_append (BOBGUI_BOX (box3),
                     bobgui_label_new ("VBox 3 Bottom"));

  bobgui_window_set_child (BOBGUI_WINDOW (window), box1);
  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
create_grid_window (void)
{
  BobguiWidget *window;
  BobguiWidget *grid;
  BobguiWidget *toggle;
  BobguiWidget *colorbox;

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Grid");

  grid = bobgui_grid_new ();

  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Top"), 1, 0, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Bottom"), 1, 3, 1, 1);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Left"), 0, 1, 1, 2);
  bobgui_grid_attach (BOBGUI_GRID (grid), bobgui_label_new ("Right"), 2, 1, 1, 2);

  colorbox = bobgui_frame_new (NULL);

  toggle = bobgui_toggle_button_new_with_label ("H Expand");
  bobgui_widget_set_halign (toggle, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (toggle, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_margin_start (toggle, 5);
  bobgui_widget_set_margin_end (toggle, 5);
  bobgui_widget_set_margin_top (toggle, 5);
  bobgui_widget_set_margin_bottom (toggle, 5);
  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (on_toggle_hexpand), NULL);
  bobgui_frame_set_child (BOBGUI_FRAME (colorbox), toggle);

  bobgui_grid_attach (BOBGUI_GRID (grid), colorbox, 1, 1, 1, 1);

  colorbox = bobgui_frame_new (NULL);

  toggle = bobgui_toggle_button_new_with_label ("V Expand");
  bobgui_widget_set_halign (toggle, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (toggle, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_margin_start (toggle, 5);
  bobgui_widget_set_margin_end (toggle, 5);
  bobgui_widget_set_margin_top (toggle, 5);
  bobgui_widget_set_margin_bottom (toggle, 5);
  g_signal_connect (G_OBJECT (toggle), "toggled",
                    G_CALLBACK (on_toggle_vexpand), NULL);
  bobgui_frame_set_child (BOBGUI_FRAME (colorbox), toggle);

  bobgui_grid_attach (BOBGUI_GRID (grid), colorbox, 1, 2, 1, 1); 

  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);
  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  bobgui_init ();

  if (g_getenv ("RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  create_box_window ();
  create_grid_window ();

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
