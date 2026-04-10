#include <bobgui/bobgui.h>

int main (int argc, char *argv[])
{
  BobguiWidget *window, *sw, *viewport, *grid;
  BobguiWidget *entry;
  int i, j;
  char *text;

  bobgui_init ();

  window = bobgui_window_new ();
  sw = bobgui_scrolled_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), sw);
  viewport = bobgui_viewport_new (bobgui_scrolled_window_get_hadjustment (BOBGUI_SCROLLED_WINDOW (sw)),
                               bobgui_scrolled_window_get_vadjustment (BOBGUI_SCROLLED_WINDOW (sw)));
  bobgui_viewport_set_scroll_to_focus (BOBGUI_VIEWPORT (viewport), TRUE);
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), viewport);
  grid = bobgui_grid_new ();
  bobgui_widget_set_margin_start (grid, 20);
  bobgui_widget_set_margin_end (grid, 20);
  bobgui_widget_set_margin_top (grid, 20);
  bobgui_widget_set_margin_bottom (grid, 20);
  bobgui_viewport_set_child (BOBGUI_VIEWPORT (viewport), grid);

  for (i = 0; i < 20; i++)
    for (j = 0; j < 20; j++)
      {
        entry = bobgui_entry_new ();
        text = g_strdup_printf ("(%d, %d)", i, j);
        bobgui_editable_set_text (BOBGUI_EDITABLE (entry), text);
        g_free (text);
        bobgui_editable_set_width_chars (BOBGUI_EDITABLE (entry), 6);
        bobgui_grid_attach (BOBGUI_GRID (grid), entry, i, j, 1, 1);
      }
 
  bobgui_window_present (BOBGUI_WINDOW (window));

  while (1)
    g_main_context_iteration (NULL, FALSE);

  return 0;
}
