#include <bobgui/bobgui.h>

static void
stack_role (void)
{
  BobguiWidget *stack = bobgui_stack_new ();
  BobguiWidget *child1 = bobgui_label_new ("a");
  BobguiWidget *child2 = bobgui_label_new ("b");
  BobguiWidget *switcher = bobgui_stack_switcher_new ();
  BobguiWidget *child;
  BobguiStackPage *page;

  g_object_ref_sink (stack);
  g_object_ref_sink (switcher);

  bobgui_stack_add_named (BOBGUI_STACK (stack), child1, "a");
  bobgui_stack_add_named (BOBGUI_STACK (stack), child2, "b");

  bobgui_stack_switcher_set_stack (BOBGUI_STACK_SWITCHER (switcher), BOBGUI_STACK (stack));

  bobgui_test_accessible_assert_role (BOBGUI_ACCESSIBLE (stack), BOBGUI_ACCESSIBLE_ROLE_GROUP);
  bobgui_test_accessible_assert_role (BOBGUI_ACCESSIBLE (switcher), BOBGUI_ACCESSIBLE_ROLE_TAB_LIST);

  child = bobgui_widget_get_first_child (switcher);
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), child1);
  bobgui_test_accessible_assert_role (BOBGUI_ACCESSIBLE (child), BOBGUI_ACCESSIBLE_ROLE_TAB);
  bobgui_test_accessible_assert_role (BOBGUI_ACCESSIBLE (page), BOBGUI_ACCESSIBLE_ROLE_TAB_PANEL);

  child = bobgui_widget_get_last_child (switcher);
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), child2);
  bobgui_test_accessible_assert_role (BOBGUI_ACCESSIBLE (child), BOBGUI_ACCESSIBLE_ROLE_TAB);
  bobgui_test_accessible_assert_role (BOBGUI_ACCESSIBLE (page), BOBGUI_ACCESSIBLE_ROLE_TAB_PANEL);

  g_object_unref (stack);
  g_object_unref (switcher);
}

static void
stack_state (void)
{
  BobguiWidget *stack = bobgui_stack_new ();
  BobguiWidget *child1 = bobgui_label_new ("a");
  BobguiWidget *child2 = bobgui_label_new ("b");
  BobguiWidget *switcher = bobgui_stack_switcher_new ();
  BobguiWidget *child;

  g_object_ref_sink (stack);
  g_object_ref_sink (switcher);

  bobgui_stack_add_named (BOBGUI_STACK (stack), child1, "a");
  bobgui_stack_add_named (BOBGUI_STACK (stack), child2, "b");

  bobgui_stack_switcher_set_stack (BOBGUI_STACK_SWITCHER (switcher), BOBGUI_STACK (stack));

  child = bobgui_widget_get_first_child (switcher);
  bobgui_test_accessible_assert_state (BOBGUI_ACCESSIBLE (child), BOBGUI_ACCESSIBLE_STATE_SELECTED, TRUE);

  child = bobgui_widget_get_last_child (switcher);
  bobgui_test_accessible_assert_state (BOBGUI_ACCESSIBLE (child), BOBGUI_ACCESSIBLE_STATE_SELECTED, FALSE);

  g_object_unref (stack);
  g_object_unref (switcher);
}

static void
stack_relations (void)
{
  BobguiWidget *stack = bobgui_stack_new ();
  BobguiWidget *child1 = bobgui_label_new ("a");
  BobguiWidget *child2 = bobgui_label_new ("b");
  BobguiWidget *switcher = bobgui_stack_switcher_new ();
  BobguiWidget *child;
  BobguiStackPage *page;

  g_object_ref_sink (stack);
  g_object_ref_sink (switcher);

  bobgui_stack_add_named (BOBGUI_STACK (stack), child1, "a");
  bobgui_stack_add_named (BOBGUI_STACK (stack), child2, "b");

  bobgui_stack_switcher_set_stack (BOBGUI_STACK_SWITCHER (switcher), BOBGUI_STACK (stack));

  child = bobgui_widget_get_first_child (switcher);
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), child1);
  bobgui_test_accessible_assert_relation (BOBGUI_ACCESSIBLE (child), BOBGUI_ACCESSIBLE_RELATION_CONTROLS, page, NULL);

  child = bobgui_widget_get_last_child (switcher);
  page = bobgui_stack_get_page (BOBGUI_STACK (stack), child2);
  bobgui_test_accessible_assert_relation (BOBGUI_ACCESSIBLE (child), BOBGUI_ACCESSIBLE_RELATION_CONTROLS, page, NULL);

  g_object_unref (stack);
  g_object_unref (switcher);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/stack/role", stack_role);
  g_test_add_func ("/a11y/stack/state", stack_state);
  g_test_add_func ("/a11y/stack/relations", stack_relations);

  return g_test_run ();
}

