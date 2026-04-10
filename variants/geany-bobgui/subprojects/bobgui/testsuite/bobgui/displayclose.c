#include <bobgui/bobgui.h>

int
main (int argc, char **argv)
{
  const char *display_name;
  GdkDisplay *display;
  BobguiWidget *win, *but;
  gboolean has_display;

  g_log_set_always_fatal (G_LOG_LEVEL_WARNING | G_LOG_LEVEL_CRITICAL);

  gdk_set_allowed_backends ("x11");
  display_name = g_getenv ("DISPLAY");
  g_unsetenv ("DISPLAY");
  has_display = bobgui_init_check ();
  g_assert_false (has_display);

  display = gdk_display_open (display_name);

  if (!display)
    return 0;

  gdk_display_manager_set_default_display (gdk_display_manager_get (), display);

  win = bobgui_window_new ();

  but = bobgui_button_new_with_label ("Try to Exit");
  g_signal_connect_swapped (but, "clicked",
                            G_CALLBACK (bobgui_window_destroy), win);
  bobgui_window_set_child (BOBGUI_WINDOW (win), but);

  bobgui_window_present (BOBGUI_WINDOW (win));

  bobgui_test_widget_wait_for_draw (win);

  gdk_display_close (display);

  return 0;
}
