#include <bobgui/bobgui.h>

static void
search_entry_role (void)
{
  BobguiWidget *widget = bobgui_search_entry_new ();
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_role (widget, BOBGUI_ACCESSIBLE_ROLE_SEARCH_BOX);

  g_object_unref (widget);
}

static void
search_entry_properties (void)
{
  BobguiWidget *widget = bobgui_search_entry_new ();
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER, NULL);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY, FALSE);

  g_object_set (widget, "placeholder-text", "Hello", NULL);
  bobgui_editable_set_editable (BOBGUI_EDITABLE (widget), FALSE);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_PLACEHOLDER, "Hello");
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_READ_ONLY, TRUE);

  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/searchentry/role", search_entry_role);
  g_test_add_func ("/a11y/searchentry/properties", search_entry_properties);

  return g_test_run ();
}
