#include <bobgui/bobgui.h>

static void
test_border_basic (void)
{
  BobguiBorder *border;
  BobguiBorder *border2;

  border = bobgui_border_new ();
  *border = (BobguiBorder) { 5, 6, 666, 777 };
  border2 = bobgui_border_copy (border);

  g_assert_true (memcmp (border, border2, sizeof (BobguiBorder)) == 0);

  bobgui_border_free (border);
  bobgui_border_free (border2);
}

int
main (int argc, char *argv[])
{
  bobgui_test_init (&argc, &argv);

  g_test_add_func ("/border/basic", test_border_basic);

  return g_test_run ();
}
