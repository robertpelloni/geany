#include <bobgui/bobgui.h>

static void
scale_role (void)
{
  BobguiWidget *widget = bobgui_scale_new (BOBGUI_ORIENTATION_HORIZONTAL, NULL);
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_role (widget, BOBGUI_ACCESSIBLE_ROLE_SLIDER);

  g_object_unref (widget);
}

static void
scale_state (void)
{
  BobguiWidget *widget = bobgui_scale_new (BOBGUI_ORIENTATION_HORIZONTAL, NULL);
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_DISABLED, FALSE);

  bobgui_widget_set_sensitive (widget, FALSE);

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_DISABLED, TRUE);

  g_object_unref (widget);
}

static void
scale_properties (void)
{
  BobguiAdjustment *adj = bobgui_adjustment_new (0.0, 0.0, 100.0, 1.0, 10.0, 10.0);
  BobguiWidget *widget = bobgui_scale_new (BOBGUI_ORIENTATION_HORIZONTAL, adj);
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, BOBGUI_ORIENTATION_HORIZONTAL);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 90.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 0.);

  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (widget), BOBGUI_ORIENTATION_VERTICAL);
  bobgui_adjustment_set_value (adj, 50.);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, BOBGUI_ORIENTATION_VERTICAL);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 90.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 50.);

  bobgui_range_set_fill_level (BOBGUI_RANGE (widget), 25.);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, BOBGUI_ORIENTATION_VERTICAL);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 25.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 25.);

  bobgui_range_set_restrict_to_fill_level (BOBGUI_RANGE (widget), FALSE);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, BOBGUI_ORIENTATION_VERTICAL);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 90.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 25.);

  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/scale/role", scale_role);
  g_test_add_func ("/a11y/scale/state", scale_state);
  g_test_add_func ("/a11y/scale/properties", scale_properties);

  return g_test_run ();
}
