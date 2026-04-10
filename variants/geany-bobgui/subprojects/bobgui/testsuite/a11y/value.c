#include <bobgui/bobgui.h>

static void
value_set_unset (void)
{
  BobguiAdjustment *adjustment = bobgui_adjustment_new (0, 0, 100, 1, 10, 10);
  BobguiWidget *scrollbar = bobgui_scrollbar_new (BOBGUI_ORIENTATION_HORIZONTAL, adjustment);

  g_object_ref_sink (scrollbar);

  bobgui_test_accessible_assert_property (scrollbar, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 0.);

  bobgui_adjustment_set_value (adjustment, 10);

  bobgui_test_accessible_assert_property (scrollbar, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 10.);

  bobgui_adjustment_set_value (adjustment, 0);

  bobgui_test_accessible_assert_property (scrollbar, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 0.);

  g_object_unref (scrollbar);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/value/set-unset", value_set_unset);

  return g_test_run ();
}
