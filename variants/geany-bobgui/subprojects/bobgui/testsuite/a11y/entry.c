#include <bobgui/bobgui.h>

static void
entry_role (void)
{
  BobguiWidget *widget = bobgui_entry_new ();
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_role (widget, BOBGUI_ACCESSIBLE_ROLE_TEXT_BOX);

  g_object_unref (widget);
}

static void
entry_properties (void)
{
  BobguiWidget *widget = bobgui_entry_new ();
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER, NULL);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY, FALSE);

  bobgui_entry_set_placeholder_text (BOBGUI_ENTRY (widget), "Hello");
  bobgui_editable_set_editable (BOBGUI_EDITABLE (widget), FALSE);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER, "Hello");
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY, TRUE);

  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/entry/role", entry_role);
  g_test_add_func ("/a11y/entry/properties", entry_properties);

  return g_test_run ();
}
