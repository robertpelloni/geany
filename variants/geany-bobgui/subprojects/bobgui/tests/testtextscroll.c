/* simple.c
 * Copyright (C) 2017  Red Hat, Inc
 * Author: Benjamin Otte
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

static BobguiWidget *_margin;
static BobguiWidget *_align;
static BobguiWidget *_xalign;
static BobguiWidget *_yalign;

static void
highlight_at_mark (BobguiTextBuffer *buffer,
                   BobguiTextMark   *mark,
                   gboolean       on)
{
  BobguiTextIter iter, iter2;

  bobgui_text_buffer_get_iter_at_mark (buffer, &iter, mark);
  iter2 = iter;
  bobgui_text_iter_forward_line (&iter2);

  if (on)
    bobgui_text_buffer_apply_tag_by_name (buffer, "hihi", &iter, &iter2);
  else
    bobgui_text_buffer_remove_tag_by_name (buffer, "hihi", &iter, &iter2);
}

static void
go_forward_or_back (BobguiButton   *button,
                    BobguiTextView *tv,
                    gboolean     forward)
{
  BobguiTextBuffer *buffer;
  BobguiTextMark *mark;
  BobguiTextIter iter;
  gboolean found;

  buffer = bobgui_text_view_get_buffer (tv);
  mark = bobgui_text_buffer_get_mark (buffer, "mimi");
  highlight_at_mark (buffer, mark, FALSE);

  bobgui_text_buffer_get_iter_at_mark (buffer, &iter, mark);
  if (forward)
    found = bobgui_text_iter_forward_search (&iter, "\n-----", 0, &iter, NULL, NULL);
  else
    found = bobgui_text_iter_backward_search (&iter, "\n-----", 0, &iter, NULL, NULL);
  if (found)
    {
      double margin;
      gboolean use_align;
      double xalign, yalign;

      bobgui_text_iter_forward_char (&iter);
      bobgui_text_buffer_move_mark (buffer, mark, &iter);
      highlight_at_mark (buffer, mark, TRUE);

      margin = bobgui_spin_button_get_value (BOBGUI_SPIN_BUTTON (_margin));
      use_align = bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (_align));
      xalign = bobgui_spin_button_get_value (BOBGUI_SPIN_BUTTON (_xalign));
      yalign = bobgui_spin_button_get_value (BOBGUI_SPIN_BUTTON (_yalign));

      bobgui_text_view_scroll_to_mark (tv, mark, margin, use_align, xalign, yalign);
    }
  else
    {
      if (forward)
        bobgui_text_buffer_get_end_iter (buffer, &iter);
      else
        bobgui_text_buffer_get_start_iter (buffer, &iter);

      bobgui_text_buffer_move_mark (buffer, mark, &iter);

      bobgui_widget_error_bell (BOBGUI_WIDGET (button));
    }
}

static void
go_forward (BobguiButton   *button,
            BobguiTextView *tv)
{
  go_forward_or_back (button, tv, TRUE);
}

static void
go_back (BobguiButton   *button,
         BobguiTextView *tv)
{
  go_forward_or_back (button, tv, FALSE);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *window, *box;
  BobguiWidget *sw, *tv;
  BobguiWidget *button, *box2;
  BobguiTextBuffer *buffer;
  BobguiTextIter iter;
  BobguiTextTag *tag;
  GdkRGBA bg;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 400, 600);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);

  tv = bobgui_text_view_new ();
  bobgui_text_view_set_left_margin (BOBGUI_TEXT_VIEW (tv), 10);
  bobgui_text_view_set_right_margin (BOBGUI_TEXT_VIEW (tv), 10);
  bobgui_text_view_set_top_margin (BOBGUI_TEXT_VIEW (tv), 10);
  bobgui_text_view_set_bottom_margin (BOBGUI_TEXT_VIEW (tv), 10);

  buffer = bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (tv));

  if (argc > 1)
    {
      char *contents;
      gsize size;

      if (g_file_get_contents (argv[1], &contents, &size, NULL))
        bobgui_text_buffer_set_text (buffer, contents, size);
    }

  bobgui_text_buffer_get_start_iter (buffer, &iter);
  bobgui_text_buffer_create_mark (buffer, "mimi", &iter, TRUE);

  tag = bobgui_text_tag_new ("hihi");
  bg.red = 0;
  bg.green = 0;
  bg.blue = 1;
  bg.alpha = 0.3;
  g_object_set (tag, "background-rgba", &bg, NULL);
  bobgui_text_tag_table_add (bobgui_text_buffer_get_tag_table (buffer), tag);

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), tv);

  bobgui_widget_set_hexpand (sw, TRUE);
  bobgui_widget_set_vexpand (sw, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), sw);

  box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_box_append (BOBGUI_BOX (box), box2);

  button = bobgui_button_new_with_label ("Forward");
  g_signal_connect (button, "clicked", G_CALLBACK (go_forward), tv);
  bobgui_box_append (BOBGUI_BOX (box2), button);

  button = bobgui_button_new_with_label ("Back");
  g_signal_connect (button, "clicked", G_CALLBACK (go_back), tv);
  bobgui_box_append (BOBGUI_BOX (box2), button);

  box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_box_append (BOBGUI_BOX (box), box2);
  bobgui_box_append (BOBGUI_BOX (box2), bobgui_label_new ("Margin:"));
  _margin = bobgui_spin_button_new_with_range (0, 0.5, 0.1);
  bobgui_box_append (BOBGUI_BOX (box2), _margin);

  box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_box_append (BOBGUI_BOX (box), box2);
  bobgui_box_append (BOBGUI_BOX (box2), bobgui_label_new ("Align:"));
  _align = bobgui_check_button_new ();
  bobgui_box_append (BOBGUI_BOX (box2), _align);
  _xalign = bobgui_spin_button_new_with_range (0, 1, 0.1);
  bobgui_box_append (BOBGUI_BOX (box2), _xalign);
  _yalign = bobgui_spin_button_new_with_range (0, 1, 0.1);
  bobgui_box_append (BOBGUI_BOX (box2), _yalign);

  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (1)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
