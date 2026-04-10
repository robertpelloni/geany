#include <bobgui/bobgui.h>
#include <locale.h>

static void
test_init (void)
{
  gboolean ret;

  g_assert_false (bobgui_is_initialized ());
  ret = bobgui_init_check ();
  g_assert_true (ret);
  g_assert_true (bobgui_is_initialized ());
}

static void
test_version (void)
{
  g_assert_cmpuint (bobgui_get_major_version (), ==, BOBGUI_MAJOR_VERSION);
  g_assert_cmpuint (bobgui_get_minor_version (), ==, BOBGUI_MINOR_VERSION);
  g_assert_cmpuint (bobgui_get_micro_version (), ==, BOBGUI_MICRO_VERSION);
  g_assert_cmpuint (bobgui_get_binary_age (), ==, BOBGUI_BINARY_AGE);
  g_assert_cmpuint (bobgui_get_interface_age (), ==, BOBGUI_INTERFACE_AGE);

 g_assert_null (bobgui_check_version (BOBGUI_MAJOR_VERSION, BOBGUI_MINOR_VERSION, BOBGUI_MICRO_VERSION));
 g_assert_nonnull (bobgui_check_version (5, 0, 0));
 g_assert_nonnull (bobgui_check_version (1, 0, 0));
 g_assert_nonnull (bobgui_check_version (3, 1000, 10));
}

int
main (int argc, char *argv[])
{
  /* Don't use bobgui_test_init here because it implicitly initializes BOBGUI. */
  (g_test_init) (&argc, &argv, NULL);
  bobgui_disable_setlocale();
  setlocale (LC_ALL, "C");

  g_test_add_func ("/main/init", test_init);
  g_test_add_func ("/main/version", test_version);

  return g_test_run ();
}
