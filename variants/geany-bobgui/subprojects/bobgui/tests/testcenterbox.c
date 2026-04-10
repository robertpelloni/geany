#include <bobgui/bobgui.h>

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
  BobguiWidget *box;
  BobguiWidget *child;
  gboolean done = FALSE;

  bobgui_init ();

  if (g_getenv ("RTL"))
    bobgui_widget_set_default_direction (BOBGUI_TEXT_DIR_RTL);

  window = bobgui_window_new ();
  box = bobgui_center_box_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), box);

  child = bobgui_label_new ("Start Start Start Start Start");
  bobgui_label_set_ellipsize (BOBGUI_LABEL (child), PANGO_ELLIPSIZE_END);
  bobgui_center_box_set_start_widget (BOBGUI_CENTER_BOX (box), child);

  child = bobgui_label_new ("Center");
  bobgui_label_set_ellipsize (BOBGUI_LABEL (child), PANGO_ELLIPSIZE_END);
  bobgui_center_box_set_center_widget (BOBGUI_CENTER_BOX (box), child);

  child = bobgui_label_new ("End");
  bobgui_label_set_ellipsize (BOBGUI_LABEL (child), PANGO_ELLIPSIZE_END);
  bobgui_center_box_set_end_widget (BOBGUI_CENTER_BOX (box), child);

  g_signal_connect (window, "destroy", G_CALLBACK (quit_cb), &done);

  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!done)
    g_main_context_iteration (NULL, TRUE);

  return 0;
}
