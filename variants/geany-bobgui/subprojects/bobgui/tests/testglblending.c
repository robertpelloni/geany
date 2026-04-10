#include <stdlib.h>
#include <bobgui/bobgui.h>

#include "bobguigears.h"

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
  BobguiWidget *window, *fixed, *gears, *spinner;
  gboolean done = FALSE;

  bobgui_init ();

  window = bobgui_window_new ();
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Test GL/bobgui inter-blending");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 250, 250);
  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  fixed = bobgui_fixed_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), fixed);

  spinner = bobgui_spinner_new ();
  bobgui_spinner_start (BOBGUI_SPINNER (spinner));
  bobgui_widget_set_size_request (spinner, 50, 50);
  bobgui_fixed_put (BOBGUI_FIXED (fixed), spinner, 90, 80);

  spinner = bobgui_spinner_new ();
  bobgui_spinner_start (BOBGUI_SPINNER (spinner));
  bobgui_widget_set_size_request (spinner, 50, 50);
  bobgui_fixed_put (BOBGUI_FIXED (fixed), spinner, 100, 80);

  spinner = bobgui_spinner_new ();
  bobgui_spinner_start (BOBGUI_SPINNER (spinner));
  bobgui_widget_set_size_request (spinner, 50, 50);
  bobgui_fixed_put (BOBGUI_FIXED (fixed), spinner, 110, 80);


  gears = bobgui_gears_new ();
  bobgui_widget_set_size_request (gears, 70, 50);
  bobgui_fixed_put (BOBGUI_FIXED (fixed), gears, 60, 100);

  spinner = bobgui_spinner_new ();
  bobgui_spinner_start (BOBGUI_SPINNER (spinner));
  bobgui_widget_set_size_request (spinner, 50, 50);
  bobgui_fixed_put (BOBGUI_FIXED (fixed), spinner, 90, 110);

  spinner = bobgui_spinner_new ();
  bobgui_spinner_start (BOBGUI_SPINNER (spinner));
  bobgui_widget_set_size_request (spinner, 50, 50);
  bobgui_fixed_put (BOBGUI_FIXED (fixed), spinner, 100, 110);

  spinner = bobgui_spinner_new ();
  bobgui_spinner_start (BOBGUI_SPINNER (spinner));
  bobgui_widget_set_size_request (spinner, 50, 50);
  bobgui_fixed_put (BOBGUI_FIXED (fixed), spinner, 110, 110);


  gears = bobgui_gears_new ();
  bobgui_widget_set_size_request (gears, 70, 50);
  bobgui_fixed_put (BOBGUI_FIXED (fixed), gears, 60, 130);

  spinner = bobgui_spinner_new ();
  bobgui_spinner_start (BOBGUI_SPINNER (spinner));
  bobgui_widget_set_size_request (spinner, 50, 50);
  bobgui_fixed_put (BOBGUI_FIXED (fixed), spinner, 90, 150);

  spinner = bobgui_spinner_new ();
  bobgui_spinner_start (BOBGUI_SPINNER (spinner));
  bobgui_widget_set_size_request (spinner, 50, 50);
  bobgui_fixed_put (BOBGUI_FIXED (fixed), spinner, 100, 150);

  spinner = bobgui_spinner_new ();
  bobgui_spinner_start (BOBGUI_SPINNER (spinner));
  bobgui_widget_set_size_request (spinner, 50, 50);
  bobgui_fixed_put (BOBGUI_FIXED (fixed), spinner, 110, 150);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return EXIT_SUCCESS;
}
