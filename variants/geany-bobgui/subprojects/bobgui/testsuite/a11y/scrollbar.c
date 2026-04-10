#include <bobgui/bobgui.h>

static void
scrollbar_role (void)
{
  BobguiWidget *widget = bobgui_scrollbar_new (BOBGUI_ORIENTATION_HORIZONTAL, NULL);
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_role (widget, BOBGUI_ACCESSIBLE_ROLE_SCROLLBAR);

  g_object_unref (widget);
}

static void
scrollbar_state (void)
{
  BobguiWidget *widget = bobgui_scrollbar_new (BOBGUI_ORIENTATION_HORIZONTAL, NULL);
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_DISABLED, FALSE);

  bobgui_widget_set_sensitive (widget, FALSE);

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_DISABLED, TRUE);

  g_object_unref (widget);
}

static void
scrollbar_properties (void)
{
  BobguiAdjustment *adj = bobgui_adjustment_new (0.0, 0.0, 100.0, 1.0, 10.0, 10.0);
  BobguiWidget *widget = bobgui_scrollbar_new (BOBGUI_ORIENTATION_HORIZONTAL, adj);
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, BOBGUI_ORIENTATION_HORIZONTAL);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 90.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 0.);

  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (widget), BOBGUI_ORIENTATION_VERTICAL);
  bobgui_adjustment_set_value (adj, 50.0);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, BOBGUI_ORIENTATION_VERTICAL);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MAX, 90.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_MIN, 0.);
  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_VALUE_NOW, 50.0);

  g_object_unref (widget);
}

static void
scrollbar_relations (void)
{
  BobguiWidget *sw = bobgui_scrolled_window_new ();
  BobguiWidget *hscrollbar;
  BobguiWidget *vscrollbar;
  BobguiWidget *child;

  g_object_ref_sink (sw);

  hscrollbar = bobgui_scrolled_window_get_hscrollbar (BOBGUI_SCROLLED_WINDOW (sw));
  vscrollbar = bobgui_scrolled_window_get_vscrollbar (BOBGUI_SCROLLED_WINDOW (sw));

  bobgui_test_accessible_assert_relation (hscrollbar, BOBGUI_ACCESSIBLE_RELATION_CONTROLS, NULL);
  bobgui_test_accessible_assert_relation (vscrollbar, BOBGUI_ACCESSIBLE_RELATION_CONTROLS, NULL);

  child = bobgui_text_view_new ();
  bobgui_scrolled_window_set_child (BOBGUI_SCROLLED_WINDOW (sw), child);

  bobgui_test_accessible_assert_relation (hscrollbar, BOBGUI_ACCESSIBLE_RELATION_CONTROLS, child, NULL);
  bobgui_test_accessible_assert_relation (vscrollbar, BOBGUI_ACCESSIBLE_RELATION_CONTROLS, child, NULL);

  g_object_unref (sw);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/scrollbar/role", scrollbar_role);
  g_test_add_func ("/a11y/scrollbar/state", scrollbar_state);
  g_test_add_func ("/a11y/scrollbar/properties", scrollbar_properties);
  g_test_add_func ("/a11y/scrollbar/relations", scrollbar_relations);

  return g_test_run ();
}
