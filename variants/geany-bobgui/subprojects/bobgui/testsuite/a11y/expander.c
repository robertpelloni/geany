#include <bobgui/bobgui.h>

static void
expander_role (void)
{
  BobguiWidget *widget = bobgui_expander_new ("Hello");
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_role (widget, BOBGUI_ACCESSIBLE_ROLE_BUTTON);

  g_object_unref (widget);
}

static void
expander_state (void)
{
  BobguiWidget *widget = bobgui_expander_new ("Hello");
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_EXPANDED, FALSE);

  bobgui_expander_set_expanded (BOBGUI_EXPANDER (widget), TRUE);

  bobgui_test_accessible_assert_state (widget, BOBGUI_ACCESSIBLE_STATE_EXPANDED, TRUE);

  g_object_unref (widget);
}

static void
expander_relations (void)
{
  BobguiWidget *widget = bobgui_expander_new ("Hello");
  BobguiWidget *child = bobgui_label_new ("Child");

  g_object_ref_sink (widget);

  bobgui_expander_set_child (BOBGUI_EXPANDER (widget), child);

  bobgui_expander_set_expanded (BOBGUI_EXPANDER (widget), TRUE);

  bobgui_test_accessible_assert_relation (widget, BOBGUI_ACCESSIBLE_RELATION_CONTROLS, child, NULL);

  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/expander/role", expander_role);
  g_test_add_func ("/a11y/expander/state", expander_state);
  g_test_add_func ("/a11y/expander/relations", expander_relations);

  return g_test_run ();
}
