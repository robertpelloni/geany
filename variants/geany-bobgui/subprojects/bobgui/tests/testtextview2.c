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
#include "config.h"
#include <bobgui/bobgui.h>


const char text[] = 
"This library is free software; you can redistribute it and/or\n"
"modify it under the terms of the GNU Library General Public\n"
"License as published by the Free Software Foundation; either\n"
"version 2 of the License, or (at your option) any later version.\n"
"\n"
"This library is distributed in the hope that it will be useful,\n"
"but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU\n"
"Library General Public License for more details.\n"
"\n"
"You should have received a copy of the GNU Library General Public\n"
"License along with this library. If not, see <http://www.gnu.org/licenses/>.\n";

static BobguiWidget *tv;
static BobguiTextBuffer *buffer;
static int len;

static BobguiTextMark **marks;
static guint marks_timeout;

static gboolean toggle_mark (gpointer data)
{
  int pos;
  BobguiTextMark *mark;

  pos = g_random_int_range (0, len);
  mark = marks[pos];

  bobgui_text_mark_set_visible (mark, !bobgui_text_mark_get_visible (mark));

  return G_SOURCE_CONTINUE;
}

static void
toggle_marks (BobguiToggleButton *button)
{
  int i;
  gboolean enable;

  enable = bobgui_toggle_button_get_active (button);

  if (!marks)
    {
      marks = g_new (BobguiTextMark*, len);

      for (i = 0; i < len; i++)
        {
          marks[i] = bobgui_text_mark_new (NULL, TRUE);
          bobgui_text_mark_set_visible (marks[i], i % 2);
        }
     }

  if (enable)
    {
      for (i = 0; i < len; i++)
        {
          BobguiTextIter iter;

          bobgui_text_buffer_get_iter_at_offset (buffer, &iter, i);
          bobgui_text_buffer_add_mark (buffer, marks[i], &iter);
        }

      marks_timeout = g_timeout_add (16, toggle_mark, NULL);
    }
  else
    {
      for (i = 0; i < len; i++)
        bobgui_text_buffer_delete_mark (buffer, marks[i]);

      if (marks_timeout)
        g_source_remove (marks_timeout);
      marks_timeout = 0;
    }
}

static gboolean
move_insert (gpointer data)
{
  BobguiTextMark *mark;
  BobguiTextIter iter, start, end;

  mark = bobgui_text_buffer_get_insert (buffer);
  bobgui_text_buffer_get_iter_at_mark (buffer, &iter, mark);
  bobgui_text_buffer_get_bounds (buffer, &start, &end);

  if (bobgui_text_iter_equal (&iter, &end))
    bobgui_text_iter_assign (&iter, &start);
  else
    bobgui_text_iter_forward_cursor_position (&iter);

  bobgui_text_buffer_place_cursor (buffer, &iter);

  return G_SOURCE_CONTINUE;
}

static guint cursor_timeout;

static void
toggle_cursor (BobguiToggleButton *button)
{
  gboolean enable;

  enable = bobgui_toggle_button_get_active (button);
  if (enable)
    cursor_timeout = g_timeout_add (16, move_insert, NULL);
  else
    {
      if (cursor_timeout)
        g_source_remove (cursor_timeout);
      cursor_timeout = 0;
    }
}

static BobguiTextMark *the_mark;
static BobguiWidget *mark_check;
static BobguiWidget *mark_visible;
static BobguiWidget *position_spin;

static void
update_mark_exists (void)
{
  int pos;
  BobguiTextIter iter;

  pos = bobgui_spin_button_get_value_as_int (BOBGUI_SPIN_BUTTON (position_spin));
  bobgui_text_buffer_get_iter_at_offset (buffer, &iter, pos);

  if (bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (mark_check)))
    bobgui_text_buffer_add_mark (buffer, the_mark, &iter);
  else 
    bobgui_text_buffer_delete_mark (buffer, the_mark);
}

static void
update_mark_visible (void)
{
  bobgui_text_mark_set_visible (the_mark, bobgui_check_button_get_active (BOBGUI_CHECK_BUTTON (mark_visible)));
}

static void
update_mark_position (void)
{
  int pos;
  BobguiTextIter iter;

  pos = bobgui_spin_button_get_value_as_int (BOBGUI_SPIN_BUTTON (position_spin));
  bobgui_text_buffer_get_iter_at_offset (buffer, &iter, pos);

  bobgui_text_buffer_move_mark (buffer, the_mark, &iter);
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
  BobguiWidget *window, *sw, *box, *box2, *button;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 600, 400);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  box = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 10);

  sw = bobgui_scrolled_window_new ();
  bobgui_widget_set_hexpand (sw, TRUE);
  bobgui_widget_set_vexpand (sw, TRUE);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);
  bobgui_box_append (BOBGUI_BOX (box), sw);

  tv = bobgui_text_view_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), tv);

  buffer = bobgui_text_buffer_new (NULL);
  bobgui_text_view_set_buffer (BOBGUI_TEXT_VIEW (tv), buffer);

  bobgui_text_buffer_set_text (buffer, text, -1);

  len = strlen (text);

  box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  g_object_set (box, "margin-start", 10, "margin-end", 10, NULL);
  bobgui_box_append (BOBGUI_BOX (box), box2);

  the_mark = bobgui_text_mark_new ("my mark", TRUE);
  box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_box_append (BOBGUI_BOX (box), box2);
  mark_check = bobgui_check_button_new_with_label ("Mark");
  g_signal_connect (mark_check, "notify::active", G_CALLBACK (update_mark_exists), NULL);
  bobgui_box_append (BOBGUI_BOX (box2), mark_check);
  mark_visible = bobgui_check_button_new_with_label ("Visible");
  g_signal_connect (mark_visible, "notify::active", G_CALLBACK (update_mark_visible), NULL);
  bobgui_box_append (BOBGUI_BOX (box2), mark_visible);
  bobgui_box_append (BOBGUI_BOX (box2), bobgui_label_new ("Position:"));
  position_spin = bobgui_spin_button_new_with_range (0, len, 1);  
  g_signal_connect (position_spin, "value-changed", G_CALLBACK (update_mark_position), NULL);
  bobgui_box_append (BOBGUI_BOX (box2), position_spin);

  box2 = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  g_object_set (box, "margin-start", 10, "margin-end", 10, NULL);
  bobgui_box_append (BOBGUI_BOX (box), box2);

  button = bobgui_toggle_button_new_with_label ("Random marks");
  g_signal_connect (button, "notify::active", G_CALLBACK (toggle_marks), NULL);
  bobgui_box_append (BOBGUI_BOX (box2), button);

  button = bobgui_toggle_button_new_with_label ("Wandering cursor");
  g_signal_connect (button, "notify::active", G_CALLBACK (toggle_cursor), NULL);
  bobgui_box_append (BOBGUI_BOX (box2), button);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
