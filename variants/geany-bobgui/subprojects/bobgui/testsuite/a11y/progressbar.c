#include <bobgui/bobgui.h>

static void
progress_bar_role (void)
{
  BobguiWidget *widget = bobgui_progress_bar_new ();
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_role (widget, BOBGUI_ACCESSIBLE_ROLE_PROGRESS_BAR);

  g_object_unref (widget);
}

static void
progress_bar_state (void)
{
  BobguiWidget *widget = bobgui_progress_bar_new ();
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_BUSY, FALSE);

  bobgui_progress_bar_pulse (BOBGUI_PROGRESS_BAR (widget));

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_BUSY, TRUE);

  g_object_unref (widget);
}

static void
progress_bar_properties (void)
{
  BobguiWidget *widget = bobgui_progress_bar_new ();
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 1.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 0.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT, NULL);

  bobgui_progress_bar_set_fraction (BOBGUI_PROGRESS_BAR (widget), 0.5);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 1.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 0.5);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT, NULL);
  g_assert_false (bobgui_test_accessible_has_property (BOBGUI_ACCESSIBLE (widget), BOBGUI_ACCESSIBLE_PROPERTY_VALUE_TEXT));

  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/progressbar/role", progress_bar_role);
  g_test_add_func ("/a11y/progressbar/state", progress_bar_state);
  g_test_add_func ("/a11y/progressbar/properties", progress_bar_properties);

  return g_test_run ();
}
