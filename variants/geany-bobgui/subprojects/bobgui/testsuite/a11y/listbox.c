#include <bobgui/bobgui.h>

static void
listbox_role (void)
{
  BobguiWidget *widget = bobgui_list_box_new ();

  g_object_ref_sink (widget);

  bobgui_list_box_append (BOBGUI_LIST_BOX (widget), bobgui_label_new ("a"));

  bobgui_test_accessible_assert_role (BOBGUI_ACCESSIBLE (widget), BOBGUI_ACCESSIBLE_ROLE_LIST);
  bobgui_test_accessible_assert_role (BOBGUI_ACCESSIBLE (bobgui_widget_get_first_child (widget)), BOBGUI_ACCESSIBLE_ROLE_LIST_ITEM);

  g_object_unref (widget);
}

static void
listbox_state (void)
{
  BobguiWidget *window = bobgui_window_new ();
  BobguiWidget *widget = bobgui_list_box_new ();

  bobgui_window_set_child (BOBGUI_WINDOW (window), widget);

  bobgui_list_box_append (BOBGUI_LIST_BOX (widget), bobgui_label_new ("a"));

  bobgui_test_accessible_assert_state (BOBGUI_ACCESSIBLE (bobgui_widget_get_first_child (widget)), BOBGUI_ACCESSIBLE_STATE_SELECTED, FALSE);

  bobgui_list_box_select_row (BOBGUI_LIST_BOX (widget),
                           BOBGUI_LIST_BOX_ROW (bobgui_widget_get_first_child (widget)));

  bobgui_test_accessible_assert_state (BOBGUI_ACCESSIBLE (bobgui_widget_get_first_child (widget)), BOBGUI_ACCESSIBLE_STATE_SELECTED, TRUE);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
listbox_properties (void)
{
  BobguiWidget *widget = bobgui_list_box_new ();

  g_object_ref_sink (widget);

  bobgui_list_box_append (BOBGUI_LIST_BOX (widget), bobgui_label_new ("a"));

  bobgui_test_accessible_assert_property (BOBGUI_ACCESSIBLE (widget), BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE, FALSE);

  bobgui_list_box_set_selection_mode (BOBGUI_LIST_BOX (widget), BOBGUI_SELECTION_MULTIPLE);

  bobgui_test_accessible_assert_property (BOBGUI_ACCESSIBLE (widget), BOBGUI_ACCESSIBLE_PROPERTY_MULTI_SELECTABLE, TRUE);

  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/listbox/role", listbox_role);
  g_test_add_func ("/a11y/listbox/state", listbox_state);
  g_test_add_func ("/a11y/listbox/properties", listbox_properties);

  return g_test_run ();
}
