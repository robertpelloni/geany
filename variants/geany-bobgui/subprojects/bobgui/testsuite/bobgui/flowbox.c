#include <bobgui/bobgui.h>

static gboolean
main_loop_quit_cb (gpointer data)
{
  gboolean *done = data;

  *done = TRUE;

  g_main_context_wakeup (NULL);

  return FALSE;
}

static void
show_and_wait (BobguiWidget *widget)
{
  gboolean done = FALSE;

  g_timeout_add (500, main_loop_quit_cb, &done);
  bobgui_widget_set_visible (widget, TRUE);
  while (!done)
    g_main_context_iteration (NULL, FALSE);
}

/* this was triggering a crash in bobgui_flow_box_measure(),
 * see #2702
 */
static void
test_measure_crash (void)
{
  BobguiWidget *window, *box, *child;

  window = bobgui_window_new ();
  box = bobgui_flow_box_new ();
  bobgui_widget_set_valign (BOBGUI_WIDGET (box), BOBGUI_ALIGN_START);
  child = g_object_new (BOBGUI_TYPE_FLOW_BOX_CHILD,
                        "css-name", "nopadding",
                        NULL);
  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (box), child, -1);
  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (box), BOBGUI_ORIENTATION_VERTICAL);
  bobgui_flow_box_set_row_spacing (BOBGUI_FLOW_BOX (box), 0);

  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  show_and_wait (window);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv);

  g_test_add_func ("/flowbox/measure-crash", test_measure_crash);

  return g_test_run ();
}
