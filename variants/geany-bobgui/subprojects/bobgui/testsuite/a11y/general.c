#include <bobgui/bobgui.h>

/* Some of the accessible attributes are general enough
 * that BOBGUI maintains them for every widget. These tests
 * are checking them.
 */

static void
test_hidden (void)
{
  BobguiWidget *window;
  BobguiWidget *widget;

  window = bobgui_window_new ();
  widget = bobgui_button_new ();
  bobgui_window_set_child (BOBGUI_WINDOW (window), widget);

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_HIDDEN, TRUE);

  bobgui_window_present (BOBGUI_WINDOW (window));

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_HIDDEN, FALSE);

  bobgui_window_destroy (BOBGUI_WINDOW (window));
}

static void
test_disabled (void)
{
  BobguiWidget *widget;

  widget = bobgui_expander_new ("");
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_DISABLED, FALSE);

  bobgui_widget_set_sensitive (widget, FALSE);

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_DISABLED, TRUE);

  g_object_unref (widget);
}

static void
test_orientation (void)
{
  BobguiWidget *widget;

  widget = bobgui_scale_new (BOBGUI_ORIENTATION_HORIZONTAL, NULL);
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, BOBGUI_ORIENTATION_HORIZONTAL);

  bobgui_orientable_set_orientation (BOBGUI_ORIENTABLE (widget), BOBGUI_ORIENTATION_VERTICAL);

  bobgui_test_accessible_assert_property (widget, BOBGUI_ACCESSIBLE_PROPERTY_ORIENTATION, BOBGUI_ORIENTATION_VERTICAL);

  g_object_unref (widget);
}

static void
test_labelled_by (void)
{
  BobguiWidget *widget;
  BobguiWidget *label;

  widget = bobgui_switch_new ();
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_relation (widget, BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, NULL);

  label = bobgui_label_new ("Switch");
  g_object_ref_sink (label);
  bobgui_widget_add_mnemonic_label (widget, label);

  bobgui_test_accessible_assert_relation (widget, BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, label, NULL);

  g_object_unref (widget);
  g_object_unref (label);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/general/hidden", test_hidden);
  g_test_add_func ("/a11y/general/disabled", test_disabled);
  g_test_add_func ("/a11y/general/orientation", test_orientation);
  g_test_add_func ("/a11y/general/labelled-by", test_labelled_by);

  return g_test_run ();
}
