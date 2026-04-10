#include <bobgui/bobgui.h>

static void
button_role (void)
{
  BobguiWidget *button = bobgui_button_new ();
  g_object_ref_sink (button);

  bobgui_test_accessible_assert_role (button, BOBGUI_ACCESSIBLE_ROLE_BUTTON);

  /* Simple command buttons have a "pressed" state set to "undefined" */
  bobgui_test_accessible_assert_state (button, BOBGUI_ACCESSIBLE_STATE_PRESSED, BOBGUI_ACCESSIBLE_VALUE_UNDEFINED);

  g_object_unref (button);
}

static void
button_label (void)
{
  BobguiWidget *button = bobgui_button_new_with_label ("Hello");
  g_object_ref_sink (button);

  bobgui_test_accessible_assert_relation (BOBGUI_ACCESSIBLE (button),
                                       BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, bobgui_widget_get_first_child (button), NULL);

  g_object_unref (button);
}

/* Check that we set up a labelled_by relationship between a button
 * and its label.
 */
static void
button_relation (void)
{
  BobguiWidget *button = bobgui_button_new_with_mnemonic ("_Hello");

  g_object_ref_sink (button);

  bobgui_test_accessible_assert_relation (BOBGUI_ACCESSIBLE (button),
                                       BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, bobgui_widget_get_first_child (button), NULL);

  g_object_unref (button);
}

static void
button_state (void)
{
  BobguiWidget *button = bobgui_button_new_with_mnemonic ("_Hello");

  g_object_ref_sink (button);

  bobgui_test_accessible_assert_state (BOBGUI_ACCESSIBLE (button),
                                    BOBGUI_ACCESSIBLE_STATE_HIDDEN, button, FALSE,
                                    -1);

  bobgui_widget_set_visible (button, FALSE);

  bobgui_test_accessible_assert_state (BOBGUI_ACCESSIBLE (button),
                                    BOBGUI_ACCESSIBLE_STATE_HIDDEN, button, TRUE,
                                    -1);

  g_object_unref (button);
}

static void
linkbutton_role (void)
{
  BobguiWidget *button = bobgui_link_button_new ("Hello");
  g_object_ref_sink (button);

  bobgui_test_accessible_assert_role (button, BOBGUI_ACCESSIBLE_ROLE_LINK);

  g_object_unref (button);
}

static void
linkbutton_label (void)
{
  BobguiWidget *button = bobgui_link_button_new ("Hello");
  g_object_ref_sink (button);

  bobgui_test_accessible_assert_relation (BOBGUI_ACCESSIBLE (button),
                                       BOBGUI_ACCESSIBLE_RELATION_LABELLED_BY, bobgui_widget_get_first_child (button), NULL);

  g_object_unref (button);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/button/role", button_role);
  g_test_add_func ("/a11y/button/label", button_label);
  g_test_add_func ("/a11y/button/relation", button_relation);
  g_test_add_func ("/a11y/button/state", button_state);
  g_test_add_func ("/a11y/linkbutton/role", linkbutton_role);
  g_test_add_func ("/a11y/linkbutton/label", linkbutton_label);

  return g_test_run ();
}
