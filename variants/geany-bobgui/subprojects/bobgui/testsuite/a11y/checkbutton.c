#include <bobgui/bobgui.h>

static void
check_button_role (void)
{
  BobguiWidget *button = bobgui_check_button_new ();
  g_object_ref_sink (button);

  bobgui_test_accessible_assert_role (button, BOBGUI_ACCESSIBLE_ROLE_CHECKBOX);

  g_object_unref (button);
}

static void
check_button_checked (void)
{
  BobguiWidget *button = bobgui_check_button_new ();
  g_object_ref_sink (button);

  bobgui_test_accessible_assert_state (button, BOBGUI_ACCESSIBLE_STATE_CHECKED, BOBGUI_ACCESSIBLE_TRISTATE_FALSE);

  bobgui_check_button_set_active (BOBGUI_CHECK_BUTTON (button), TRUE);

  bobgui_test_accessible_assert_state (button, BOBGUI_ACCESSIBLE_STATE_CHECKED, BOBGUI_ACCESSIBLE_TRISTATE_TRUE);

  bobgui_check_button_set_inconsistent (BOBGUI_CHECK_BUTTON (button), TRUE);

  bobgui_test_accessible_assert_state (button, BOBGUI_ACCESSIBLE_STATE_CHECKED, BOBGUI_ACCESSIBLE_TRISTATE_MIXED);

  g_object_unref (button);
}

static void
check_button_label (void)
{
  BobguiWidget *button = bobgui_check_button_new_with_label ("Hello");
  g_object_ref_sink (button);

  bobgui_test_accessible_assert_property (button, BOBGUI_ACCESSIBLE_PROPERTY_LABEL, "Hello");

  g_object_unref (button);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/checkbutton/role", check_button_role);
  g_test_add_func ("/a11y/checkbutton/checked", check_button_checked);
  g_test_add_func ("/a11y/checkbutton/label", check_button_label);

  return g_test_run ();
}
