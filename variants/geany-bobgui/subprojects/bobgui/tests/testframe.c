/* testframe.c
 * Copyright (C) 2007  Xan López <xan@gnome.org>
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
#include <math.h>

/* Function to normalize rounding errors in FP arithmetic to
   our desired limits */

#define EPSILON 1e-10

static double
double_normalize (double n)
{
  if (fabs (1.0 - n) < EPSILON)
    n = 1.0;
  else if (n < EPSILON)
    n = 0.0;

  return n;
}

static void
spin_xalign_cb (BobguiSpinButton *spin, BobguiFrame *frame)
{
  double xalign;

  xalign = double_normalize (bobgui_spin_button_get_value (spin));
  bobgui_frame_set_label_align (frame, xalign);
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int main (int argc, char **argv)
{
  BobguiWidget *window, *widget;
  BobguiBox *vbox;
  BobguiFrame *frame;
  BobguiGrid *grid;
  float xalign;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 300, 300);

  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  vbox = BOBGUI_BOX (bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5));
  bobgui_widget_set_margin_start (BOBGUI_WIDGET (vbox), 12);
  bobgui_widget_set_margin_end (BOBGUI_WIDGET (vbox), 12);
  bobgui_widget_set_margin_top (BOBGUI_WIDGET (vbox), 12);
  bobgui_widget_set_margin_bottom (BOBGUI_WIDGET (vbox), 12);
  bobgui_window_set_child (BOBGUI_WINDOW (window), BOBGUI_WIDGET (vbox));

  frame = BOBGUI_FRAME (bobgui_frame_new ("Test BobguiFrame"));
  bobgui_widget_set_vexpand (BOBGUI_WIDGET (frame), TRUE);
  bobgui_box_append (BOBGUI_BOX (vbox), BOBGUI_WIDGET (frame));

  widget = bobgui_button_new_with_label ("Hello!");
  bobgui_frame_set_child (BOBGUI_FRAME (frame), widget);

  grid = BOBGUI_GRID (bobgui_grid_new ());
  bobgui_grid_set_row_spacing (grid, 12);
  bobgui_grid_set_column_spacing (grid, 6);
  bobgui_box_append (BOBGUI_BOX (vbox), BOBGUI_WIDGET (grid));

  xalign = bobgui_frame_get_label_align (frame);

  /* Spin to control :label-xalign */
  widget = bobgui_label_new ("label xalign:");
  bobgui_grid_attach (grid, widget, 0, 0, 1, 1);

  widget = bobgui_spin_button_new_with_range (0.0, 1.0, 0.1);
  bobgui_spin_button_set_value (BOBGUI_SPIN_BUTTON (widget), xalign);
  g_signal_connect (widget, "value-changed", G_CALLBACK (spin_xalign_cb), frame);
  bobgui_grid_attach (grid, widget, 1, 0, 1, 1);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
