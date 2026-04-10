#include <bobgui/bobgui.h>

static void
flowbox_role (void)
{
  BobguiWidget *widget = bobgui_flow_box_new ();

  g_object_ref_sink (widget);

  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (widget), bobgui_label_new ("a"), 0);

  bobgui_test_accessible_assert_role (BOBGUI_ACCESSIBLE (widget), BOBGUI_ACCESSIBLE_ROLE_GRID);
  bobgui_test_accessible_assert_role (BOBGUI_ACCESSIBLE (bobgui_widget_get_first_child (widget)), BOBGUI_ACCESSIBLE_ROLE_GRID_CELL);

  g_object_unref (widget);
}

static void
flowbox_state (void)
{
  BobguiWidget *window = bobgui_window_new ();
  BobguiWidget *widget = bobgui_flow_box_new ();

  bobgui_window_set_child (BOBGUI_WINDOW (window), widget);

  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (widget), bobgui_label_new ("a"), 0);

  bobgui_test_accessible_assert_state (BOBGUI_ACCESSIBLE (bobgui_widget_get_first_child (widget)), BOBGUI_ACCESSIBLE_STATE_SELECTED, FALSE);

  bobgui_flow_box_select_child (BOBGUI_FLOW_BOX (widget),
                             BOBGUI_FLOW_BOX_CHILD (bobgui_widget_get_first_child (widget)));

  bobgui_test_accessible_assert_state (BOBGUI_ACCESSIBLE (bobgui_widget_get_first_child (widget)), BOBGUI_ACCESSIBLE_STATE_SELECTED, TRUE);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
flowbox_properties (void)
{
  BobguiWidget *widget = bobgui_flow_box_new ();

  g_object_ref_sink (widget);

  bobgui_flow_box_insert (BOBGUI_FLOW_BOX (widget), bobgui_label_new ("a"), 0);

  bobgui_test_accessible_assert_property (BOBGUI_ACCESSIBLE (widget), BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE, FALSE);

  bobgui_flow_box_set_selection_mode (BOBGUI_FLOW_BOX (widget), BOBGUI_SELECTION_MULTIPLE);

  bobgui_test_accessible_assert_property (BOBGUI_ACCESSIBLE (widget), BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE, TRUE);

  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/flowbox/role", flowbox_role);
  g_test_add_func ("/a11y/flowbox/state", flowbox_state);
  g_test_add_func ("/a11y/flowbox/properties", flowbox_properties);

  return g_test_run ();
}
