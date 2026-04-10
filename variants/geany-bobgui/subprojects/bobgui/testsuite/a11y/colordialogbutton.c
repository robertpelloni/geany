#include <bobgui/bobgui.h>

static void
color_dialog_button_role (void)
{
  BobguiWidget *window, *widget;

  widget = bobgui_color_dialog_button_new (NULL);
  window = bobgui_window_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), widget);
  bobgui_window_present (BOBGUI_WINDOW (window));

  while (!bobgui_widget_get_realized (widget))
    g_main_context_iteration (NULL, FALSE);

  bobgui_test_accessible_assert_role (widget, BOBGUI_ACCESSIBLE_ROLE_GROUP);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/color-dialog-button/role", color_dialog_button_role);

  return g_test_run ();
}
