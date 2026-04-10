/* testscale.c - scale mark demo
 * Copyright (C) 2009 Red Hat, Inc.
 * Author: Matthias Clasen
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


GSList *scales;
BobguiWidget *flipbox;
BobguiWidget *extra_scale;

static void
invert (BobguiButton *button)
{
  GSList *l;

  for (l = scales; l; l = l->next)
    {
      BobguiRange *range = l->data;
      bobgui_range_set_inverted (range, !bobgui_range_get_inverted (range));
    }
}

static void
flip (BobguiButton *button)
{
  GSList *l;

  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (flipbox), 1 - bobgui_orientable_get_orientation (BOBGUI_ORIENTABLE (flipbox)));

  for (l = scales; l; l = l->next)
    {
      BobguiOrientable *o = l->data;
      bobgui_orientable_set_orientation (o, 1 - bobgui_orientable_get_orientation (o));
    }
}

static void
trough (BobguiToggleButton *button)
{
  GSList *l;
  gboolean value;

  value = bobgui_toggle_button_get_active (button);

  for (l = scales; l; l = l->next)
    {
      BobguiRange *range = l->data;
      bobgui_range_set_range (range, 0., value ? 100.0 : 0.);
    }
}

double marks[3] = { 0.0, 50.0, 100.0 };
double extra_marks[2] = { 20.0, 40.0 };

static void
extra (BobguiToggleButton *button)
{
  gboolean value;

  value = bobgui_toggle_button_get_active (button);

  if (value)
    {
      bobgui_scale_add_mark (BOBGUI_SCALE (extra_scale), extra_marks[0], BOBGUI_POS_TOP, NULL);
      bobgui_scale_add_mark (BOBGUI_SCALE (extra_scale), extra_marks[1], BOBGUI_POS_TOP, NULL);
    }
  else
    {
      bobgui_scale_clear_marks (BOBGUI_SCALE (extra_scale));
      bobgui_scale_add_mark (BOBGUI_SCALE (extra_scale), marks[0], BOBGUI_POS_BOTTOM, NULL);
      bobgui_scale_add_mark (BOBGUI_SCALE (extra_scale), marks[1], BOBGUI_POS_BOTTOM, NULL);
      bobgui_scale_add_mark (BOBGUI_SCALE (extra_scale), marks[2], BOBGUI_POS_BOTTOM, NULL);
    }
}

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

int main (int argc, char *argv[])
{
  BobguiWidget *window;
  BobguiWidget *box;
  BobguiWidget *box1;
  BobguiWidget *box2;
  BobguiWidget *button;
  BobguiWidget *frame;
  BobguiWidget *scale;
  const char *labels[3] = {
    "<small>Left</small>",
    "<small>Middle</small>",
    "<small>Right</small>"
  };

  double bath_marks[4] = { 0.0, 33.3, 66.6, 100.0 };
  const char *bath_labels[4] = {
    "<span color='blue' size='small'>Cold</span>",
    "<span size='small'>Baby bath</span>",
    "<span size='small'>Hot tub</span>",
    "<span color='Red' size='small'>Hot</span>"
  };

  double pos_marks[4] = { 0.0, 33.3, 66.6, 100.0 };
  const char *pos_labels[4] = { "Left", "Right", "Top", "Bottom" };
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Ranges with marks");
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  box1 = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
  flipbox = box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 5);
  bobgui_widget_set_hexpand (flipbox, TRUE);
  bobgui_widget_set_vexpand (flipbox, TRUE);
  bobgui_box_append (BOBGUI_BOX (box1), box);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box1);

  frame = bobgui_frame_new ("No marks");
  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), FALSE);
  bobgui_frame_set_child (BOBGUI_FRAME (frame), scale);
  bobgui_box_append (BOBGUI_BOX (box), frame);

  frame = bobgui_frame_new ("With fill level");
  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), FALSE);
  bobgui_range_set_show_fill_level (BOBGUI_RANGE (scale), TRUE);
  bobgui_range_set_fill_level (BOBGUI_RANGE (scale), 50);
  bobgui_frame_set_child (BOBGUI_FRAME (frame), scale);
  bobgui_box_append (BOBGUI_BOX (box), frame);

  frame = bobgui_frame_new ("Simple marks");
  extra_scale = scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), FALSE);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), marks[0], BOBGUI_POS_BOTTOM, NULL);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), marks[1], BOBGUI_POS_BOTTOM, NULL);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), marks[2], BOBGUI_POS_BOTTOM, NULL);
  bobgui_frame_set_child (BOBGUI_FRAME (frame), scale);
  bobgui_box_append (BOBGUI_BOX (box), frame);

  frame = bobgui_frame_new ("Simple marks up");
  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), FALSE);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), marks[0], BOBGUI_POS_TOP, NULL);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), marks[1], BOBGUI_POS_TOP, NULL);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), marks[2], BOBGUI_POS_TOP, NULL);
  bobgui_frame_set_child (BOBGUI_FRAME (frame), scale);
  bobgui_box_append (BOBGUI_BOX (box), frame);

  frame = bobgui_frame_new ("Labeled marks");
  box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);

  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), FALSE);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), marks[0], BOBGUI_POS_BOTTOM, labels[0]);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), marks[1], BOBGUI_POS_BOTTOM, labels[1]);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), marks[2], BOBGUI_POS_BOTTOM, labels[2]);
  bobgui_frame_set_child (BOBGUI_FRAME (frame), scale);
  bobgui_box_append (BOBGUI_BOX (box), frame);

  frame = bobgui_frame_new ("Some labels");
  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), FALSE);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), marks[0], BOBGUI_POS_TOP, labels[0]);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), marks[1], BOBGUI_POS_TOP, NULL);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), marks[2], BOBGUI_POS_TOP, labels[2]);
  bobgui_frame_set_child (BOBGUI_FRAME (frame), scale);
  bobgui_box_append (BOBGUI_BOX (box), frame);

  frame = bobgui_frame_new ("Above and below");
  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), FALSE);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), bath_marks[0], BOBGUI_POS_TOP, bath_labels[0]);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), bath_marks[1], BOBGUI_POS_BOTTOM, bath_labels[1]);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), bath_marks[2], BOBGUI_POS_BOTTOM, bath_labels[2]);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), bath_marks[3], BOBGUI_POS_TOP, bath_labels[3]);
  bobgui_frame_set_child (BOBGUI_FRAME (frame), scale);
  bobgui_box_append (BOBGUI_BOX (box), frame);

  frame = bobgui_frame_new ("Positions");
  scale = bobgui_scale_new_with_range (BOBGUI_ORIENTATION_HORIZONTAL, 0, 100, 1);
  scales = g_slist_prepend (scales, scale);
  bobgui_scale_set_draw_value (BOBGUI_SCALE (scale), FALSE);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), pos_marks[0], BOBGUI_POS_LEFT, pos_labels[0]);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), pos_marks[1], BOBGUI_POS_RIGHT, pos_labels[1]);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), pos_marks[2], BOBGUI_POS_TOP, pos_labels[2]);
  bobgui_scale_add_mark (BOBGUI_SCALE (scale), pos_marks[3], BOBGUI_POS_BOTTOM, pos_labels[3]);
  bobgui_frame_set_child (BOBGUI_FRAME (frame), scale);
  bobgui_box_append (BOBGUI_BOX (box), frame);

  box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 6);
  bobgui_box_append (BOBGUI_BOX (box1), box2);
  button = bobgui_button_new_with_label ("Flip");
  g_signal_connect (button, "clicked", G_CALLBACK (flip), NULL);
  bobgui_box_append (BOBGUI_BOX (box2), button);

  button = bobgui_button_new_with_label ("Invert");
  g_signal_connect (button, "clicked", G_CALLBACK (invert), NULL);
  bobgui_box_append (BOBGUI_BOX (box2), button);

  button = bobgui_toggle_button_new_with_label ("Trough");
  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), TRUE);
  g_signal_connect (button, "toggled", G_CALLBACK (trough), NULL);
  bobgui_box_append (BOBGUI_BOX (box2), button);
  bobgui_window_present (BOBGUI_WINDOW (window));

  button = bobgui_toggle_button_new_with_label ("Extra");
  bobgui_toggle_button_set_active (BOBGUI_TOGGLE_BUTTON (button), FALSE);
  g_signal_connect (button, "toggled", G_CALLBACK (extra), NULL);
  bobgui_box_append (BOBGUI_BOX (box2), button);
  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}


