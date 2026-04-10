#include <bobgui/bobgui.h>

static void
fill_text_view (BobguiWidget *tv, const char *text)
{
  int i;
  GString *s;

  s = g_string_new ("");
  for (i = 0; i < 200; i++)
    g_string_append_printf (s, "%s %d\n", text, i);

  bobgui_text_buffer_set_text (bobgui_text_view_get_buffer (BOBGUI_TEXT_VIEW (tv)),
                            g_string_free (s, FALSE), -1); 
}

int
main (int argc, char *argv[])
{
  BobguiWidget *win, *box, *tv, *sw, *sb;
  BobguiAdjustment *adj;

  bobgui_init ();

  win = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (win), 640, 480);

  box = bobgui_box_new (BOBGUI_ORIENTATION_HORIZONTAL, 5);
  bobgui_window_set_child (BOBGUI_WINDOW (win), box);

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_EXTERNAL);
  bobgui_widget_set_hexpand (sw, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), sw);
  tv = bobgui_text_view_new ();
  fill_text_view (tv, "Left");
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), tv);

  adj = bobgui_scrolled_window_get_vadjustment (BOBGUI_SCROLLED_WINDOW (sw));

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_vadjustment (BOBGUI_SCROLLED_WINDOW (sw), adj);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_EXTERNAL);
  bobgui_widget_set_hexpand (sw, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), sw);
  tv = bobgui_text_view_new ();
  fill_text_view (tv, "Middle");
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), tv);

  sw = bobgui_scrolled_window_new ();
  bobgui_scrolled_window_set_vadjustment (BOBGUI_SCROLLED_WINDOW (sw), adj);
  bobgui_scrolled_window_set_policy (BOBGUI_SCROLLED_WINDOW (sw),
                                  BOBGUI_POLICY_NEVER,
                                  BOBGUI_POLICY_EXTERNAL);
  bobgui_widget_set_hexpand (sw, TRUE);
  bobgui_box_append (BOBGUI_BOX (box), sw);
  tv = bobgui_text_view_new ();
  fill_text_view (tv, "Right");
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), tv);

  sb = bobgui_scrollbar_new (BOBGUI_ORIENTATION_VERTICAL, adj);

  bobgui_box_append (BOBGUI_BOX (box), sb);

  bobgui_window_present (BOBGUI_WINDOW (win));

  while (TRUE)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
