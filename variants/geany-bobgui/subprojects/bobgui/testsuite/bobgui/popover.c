#include <bobgui/bobgui.h>
#include <locale.h>
#include "gdk/gdkeventsprivate.h"

static gboolean
pop_up (gpointer data)
{
  bobgui_popover_popup (BOBGUI_POPOVER (data));

  return G_SOURCE_REMOVE;
}

static gboolean
tickle (gpointer data)
{
  BobguiWidget *label;

  label = bobgui_widget_get_first_child (BOBGUI_WIDGET (data));
  bobgui_widget_set_valign (label, BOBGUI_ALIGN_START);

  return G_SOURCE_REMOVE;
}

static gboolean
stop (gpointer data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);

  return G_SOURCE_REMOVE;
}

static void
test_show_popover (void)
{
  BobguiWidget *window;
  BobguiWidget *button;
  BobguiWidget *popover;
  gboolean done;

  window = bobgui_window_new ();
  button = bobgui_menu_button_new ();
  popover = bobgui_popover_new ();
  bobgui_popover_set_child (BOBGUI_POPOVER (popover), bobgui_label_new ("Nu?"));
  bobgui_menu_button_set_popover (BOBGUI_MENU_BUTTON (button), popover);
  bobgui_window_set_child (BOBGUI_WINDOW (window), button);

  bobgui_window_present (BOBGUI_WINDOW (window));

  g_timeout_add (1000, pop_up, popover);
  g_timeout_add (2000, tickle, popover);
  done = FALSE;
  g_timeout_add (3000, stop, &done);

  while (!done)
    g_main_context_iteration (NULL, TRUE);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv);

  g_test_add_func ("/popover/show", test_show_popover);

  return g_test_run ();
}
