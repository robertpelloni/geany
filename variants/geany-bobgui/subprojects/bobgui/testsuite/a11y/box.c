#include <bobgui/bobgui.h>

static void
box_role (void)
{
  BobguiWidget *widget = bobgui_box_new (BOBGUI_ORIENTATION_VERTICAL, 0);
  g_object_ref_sink (widget);

  bobgui_test_accessible_assert_role (widget, BOBGUI_ACCESSIBLE_ROLE_GENERIC);

  g_object_unref (widget);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv, NULL);

  g_test_add_func ("/a11y/box/role", box_role);

  return g_test_run ();
}
