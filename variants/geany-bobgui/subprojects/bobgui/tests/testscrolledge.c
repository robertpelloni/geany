/* testscrolledge.c
 *
 * Copyright (C) 2014 Matthias Clasen <mclasen@redhat.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <bobgui/bobgui.h>

static guint add_rows_id = 0;

static void
populate_list (BobguiListBox *list)
{
  int i;
  char *text;
  BobguiWidget *row, *label;
  int n;
  BobguiWidget *child;

  for (child = bobgui_widget_get_first_child (BOBGUI_WIDGET (list)), n = 0;
       child != NULL;
       child = bobgui_widget_get_next_sibling (child), n++) ;

  for (i = 1; i <= 50; i++)
    {
      row = bobgui_list_box_row_new ();
      text = g_strdup_printf ("List row %d", i + n);
      label = bobgui_label_new (text);
      g_free (text);

      bobgui_widget_set_margin_start (label, 10);
      bobgui_widget_set_margin_end (label, 10);
      bobgui_widget_set_margin_top (label, 10);
      bobgui_widget_set_margin_bottom (label, 10);
      bobgui_widget_set_halign (label, BOBGUI_ALIGN_START);
      bobgui_list_box_row_set_child (BOBGUI_LIST_BOX_ROW (row), label);
      bobgui_list_box_insert (BOBGUI_LIST_BOX (list), row, -1);
    }
}

static BobguiWidget *popup;
static BobguiWidget *spinner;

static gboolean
add_rows (gpointer data)
{
  BobguiListBox *list = data;

  bobgui_widget_set_visible (popup, FALSE);
  bobgui_spinner_stop (BOBGUI_SPINNER (spinner));

  populate_list (list);
  add_rows_id = 0;

  return G_SOURCE_REMOVE;
}

static void
edge_overshot (BobguiScrolledWindow *sw,
               BobguiPositionType    pos,
               BobguiListBox        *list)
{
  if (pos == BOBGUI_POS_BOTTOM)
    {
      bobgui_spinner_start (BOBGUI_SPINNER (spinner));
      bobgui_widget_set_visible (popup, TRUE);

      if (add_rows_id == 0)
        add_rows_id = g_timeout_add (2000, add_rows, list);
    }
}

static void
edge_reached (BobguiScrolledWindow *sw,
	      BobguiPositionType    pos,
	      BobguiListBox        *list)
{
  g_print ("Reached the edge at pos %d!\n", pos);
}

int
main (int argc, char *argv[])
{
  BobguiWidget *win;
  BobguiWidget *sw;
  BobguiWidget *list;
  BobguiWidget *overlay;
  BobguiWidget *label;

  bobgui_init ();

  win = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (win), 600, 400);

  overlay = bobgui_overlay_new ();
  popup = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 10);
  bobgui_widget_set_halign (popup, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (popup, BOBGUI_ALIGN_END);
  bobgui_widget_set_margin_start (popup, 40);
  bobgui_widget_set_margin_end (popup, 40);
  bobgui_widget_set_margin_top (popup, 40);
  bobgui_widget_set_margin_bottom (popup, 40);
  label = bobgui_label_new ("Getting more rows...");
  spinner = bobgui_spinner_new ();
  bobgui_box_append (BOBGUI_BOX (popup), label);
  bobgui_box_append (BOBGUI_BOX (popup), spinner);

  bobgui_overlay_add_overlay (BOBGUI_OVERLAY (overlay), popup);
  bobgui_widget_set_visible (popup, FALSE);

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw), BOBGUI_POLICY_NEVER, BOBGUI_POLICY_AUTOMATIC);
  list = bobgui_list_box_new ();
  bobgui_list_box_set_selection_mode (BOBGUI_LIST_BOX (list), BOBGUI_SELECTION_NONE);

  bobgui_window_set_child (BOBGUI_WINDOW (win), overlay);
  bobgui_overlay_set_child (BOBGUI_OVERLAY (overlay), sw);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), list);
  populate_list (BOBGUI_LIST_BOX (list));

  g_signal_connect (sw, "edge-overshot", G_CALLBACK (edge_overshot), list);
  g_signal_connect (sw, "edge-reached", G_CALLBACK (edge_reached), list);

  bobgui_window_present (BOBGUI_WINDOW (win));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
