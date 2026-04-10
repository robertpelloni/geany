#include <bobgui/bobgui.h>

static void
quit_cb (BobguiWidget *widget,
         gpointer   data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);
}

static void
report_suspended_state (BobguiWindow *window)
{
  g_print ("Window is %s\n",
           bobgui_window_is_suspended (window) ? "suspended" : "active");
}

static void
suspended_changed_cb (BobguiWindow *window)
{
  report_suspended_state (window);
}

int main (int argc, char *argv[])
{
  BobguiWidget *window;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 200, 200);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);
  g_signal_connect (window, "notify::suspended",
                    G_CALLBACK (suspended_changed_cb), &done);
  bobgui_window_present (BOBGUI_WINDOW (window));
  report_suspended_state (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return EXIT_SUCCESS;
}
