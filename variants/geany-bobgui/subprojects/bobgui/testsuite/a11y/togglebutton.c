#include <bobgui/bobgui.h>

static void
toggle_button_role (void)
{
  BobguiWidget *button = bobgui_toggle_button_new ();
  g_object_ref_sink (button);

  bobgui_test_accessible_assert_role (button, BOBGUI_ACCESSIBLE_ROLE_TOGGLE_BUTTON);

  /* Simple command buttons have a "pressed" state set to "undefined" */
  bobgui_test_accessible_assert_state (button, BOBGUI_ACCESSIBLE_STATE_PRESSED, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED);

  g_object_unref (button);
}

static void
toggle_button_label (void)
{
  BobguiWidget *button = bobgui_toggle_button_new_with_label ("Hello");
  g_object_ref_sink (button);

  bobgui_test_accessible_assert_relation (BOBGUI_ACCESSIBLE (button),
                                       BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, bobgui_widget_get_first_child (button), NULL);

  g_object_unref (button);
}

/* Check that we set up a labelled_by relationship between a button
 * and its label.
 */
static void
toggle_button_relation (void)
{
  BobguiWidget *button = bobgui_toggle_button_new_with_mnemonic ("_Hello");

  g_object_ref_sink (button);

  bobgui_test_accessible_assert_relation (BOBGUI_ACCESSIBLE (button),
                                       BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, bobgui_widget_get_first_child (button), NULL);

  g_object_unref (button);
}

static void
toggle_button_generic (void)
{
  BobguiBuilder *builder;
  BobguiWidget *button;
  GError *error = NULL;

  button = g_object_new (BOBGUI_TYPE_TOGGLE_BUTTON,
                         "accessible-role", BOBGUI_ACCESSIBLE_ROLE_GENERIC,
                         NULL);

  g_object_ref_sink (button);

  bobgui_test_accessible_assert_role (button, BOBGUI_ACCESSIBLE_ROLE_GENERIC);

  g_object_unref (button);

  builder = bobgui_builder_new ();

  bobgui_builder_add_from_string (builder,
    "<interface>"
    "  <object class='BobguiToggleButton' id='togglebutton'>"
    "    <property name='accessible-role'>generic</property>"
    "  </object>"
    "</interface>",
    -1, &error);
  g_assert_no_error (error);

  button = BOBGUI_WIDGET (bobgui_builder_get_object (builder, "togglebutton"));

  bobgui_test_accessible_assert_role (button, BOBGUI_ACCESSIBLE_ROLE_GENERIC);

  g_object_unref (builder);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/togglebutton/role", toggle_button_role);
  g_test_add_func ("/a11y/togglebutton/label", toggle_button_label);
  g_test_add_func ("/a11y/togglebutton/relation", toggle_button_relation);
  g_test_add_func ("/a11y/togglebutton/generic", toggle_button_generic);

  return g_test_run ();
}
