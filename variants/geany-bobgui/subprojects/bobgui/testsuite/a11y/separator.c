#include <bobgui/bobgui.h>

static void
separator_role (void)
{
  BobguiWidget *widget = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_role (widget, BOBGUI_ACCESSIBLE_ROLE_SEPARATOR);

  g_object_unref (widget);
}

static void
separator_properties (void)
{
  BobguiWidget *widget = bobgui_separator_new (BOBGUI_ORIENTATION_HORIZONTAL);
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, BOBGUI_ORIENTATION_HORIZONTAL);

  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (widget), BOBGUI_ORIENTATION_VERTICAL);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, BOBGUI_ORIENTATION_VERTICAL);

  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/separator/role", separator_role);
  g_test_add_func ("/a11y/separator/properties", separator_properties);

  return g_test_run ();
}
