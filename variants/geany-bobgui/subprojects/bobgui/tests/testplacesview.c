#include <bobgui/bobgui.h>
#define BOBGUI_COMPILATION
#include "bobgui/bobguiplacesviewprivate.h"

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
  BobguiWidget *win;
  BobguiWidget *view;
  gboolean done = FALSE;

  bobgui_init ();

  win = bobgui_window_new ();
  bobgui_window_set_default_size (BOBGUI_WINDOW (win), 400, 600);

  view = bobgui_places_view_new ();

  bobgui_window_set_child (BOBGUI_WINDOW (win), view);
  bobgui_window_present (BOBGUI_WINDOW (win));

  g_signal_connect (win, "destroy", G_CALLBACK (quit_cb), &done);

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
