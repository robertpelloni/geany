#include <bobgui/bobgui.h>

static void
switch_role (void)
{
  BobguiWidget *widget = bobgui_switch_new ();
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_role (widget, BOBGUI_ACCESSIBLE_ROLE_SWITCH);

  g_object_unref (widget);
}

static void
switch_state (void)
{
  BobguiWidget *widget = bobgui_switch_new ();
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_CHECKED, FALSE);

  bobgui_switch_set_active (BOBGUI_SWITCH (widget), TRUE);

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_CHECKED, TRUE);

  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/switch/role", switch_role);
  g_test_add_func ("/a11y/switch/state", switch_state);

  return g_test_run ();
}
