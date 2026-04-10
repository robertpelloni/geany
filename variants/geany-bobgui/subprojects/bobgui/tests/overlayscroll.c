/* simple.c
 * Copyright (C) 1997  Red Hat, Inc
 * Author: Elliot Lee
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
#include <string.h>

G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static char *
get_content (void)
{
  GString *s;
  int i;

  s = g_string_new ("");
  for (i = 1; i <= 150; i++)
    g_string_append_printf (s, "Line %d\n", i);

  return g_string_free (s, FALSE);
}

static void
mode_changed (BobguiComboBox *combo, BobguiScrolledWindow *sw)
{
  int active = bobgui_combo_box_get_active (combo);

  bobgui_scrolled_window_set_overlay_scrolling (sw, active == 1);
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
  BobguiWidget *window;
  char *content;
  BobguiWidget *box;
  BobguiWidget *sw;
  BobguiWidget *tv;
  BobguiWidget *sb2;
  BobguiWidget *combo;
  BobguiAdjustment *adj;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 640, 480);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 20);
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_AUTOMATIC);

  bobgui_widget_set_hexpand (sw, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), sw);

  content = get_content ();

  tv = bobgui_text_view_new ();
  bobgui_text_view_set_wrap_mode (BOBGUI_TEXT_VIEW (tv), BOBGUI_WRAP_WORD);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), tv);
  bobgui_text_buffer_set_text (bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (tv)),
                            content, -1);
  g_free (content);

  adj = bobgui_scrollable_get_vadjustment (BOBGUI_SCROLLABLE (tv));

  combo = bobgui_combo_box_text_new ();
  bobgui_widget_set_valign (combo, BOBGUI_ALIGN_START);
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Traditional");
  bobgui_combo_box_text_append_text (BOBGUI_COMBO_BOX_TEXT (combo), "Overlay");
  g_signal_connect (combo, "changed", G_CALLBACK (mode_changed), sw);
  bobgui_combo_box_set_active (BOBGUI_COMBO_BOX (combo), 1);

  bobgui_box_append (BOBGUI_BOX (box), combo);

  sb2 = bobgui_scrollbar_new (BOBGUI_ORIENTATION_VERTICAL, adj);
  bobgui_box_append (BOBGUI_BOX (box), sb2);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
