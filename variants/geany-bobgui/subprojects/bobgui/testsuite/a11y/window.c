#include <bobgui/bobgui.h>

static void
window_role (void)
{
  BobguiWidget *window = bobgui_window_new ();

  bobgui_test_accessible_assert_role (window, BOBGUI_ACCESSIBLE_ROLE_WINDOW);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
window_state (void)
{
  BobguiWidget *window = bobgui_window_new ();

  bobgui_window_present (BOBGUI_WINDOW (window));

  bobgui_test_accessible_assert_state (window, BOBGUI_ACCESSIBLE_STATE_HIDDEN, FALSE);

  bobgui_widget_set_visible (window, FALSE);

  bobgui_test_accessible_assert_state (window, BOBGUI_ACCESSIBLE_STATE_HIDDEN, TRUE);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
window_properties (void)
{
  BobguiWidget *window = bobgui_window_new ();

  bobgui_window_set_modal (BOBGUI_WINDOW (window), TRUE);

  bobgui_test_accessible_assert_property (window, BOBGUI_ACCESSIBLE_PROPERTY_MODAL, TRUE);
  bobgui_window_set_modal (BOBGUI_WINDOW (window), FALSE);

  bobgui_test_accessible_assert_property (window, BOBGUI_ACCESSIBLE_PROPERTY_MODAL, FALSE);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/window/role", window_role);
  g_test_add_func ("/a11y/window/state", window_state);
  g_test_add_func ("/a11y/window/properties", window_properties);

  return g_test_run ();
}
