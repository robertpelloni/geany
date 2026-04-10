#include <bobgui/bobgui.h>

static void
capture_widget_destroy (void)
{
  BobguiWidget *searchbar = bobgui_search_bar_new ();
  BobguiWidget *button = bobgui_button_new ();

  g_object_ref_sink (searchbar);
  g_object_ref_sink (button);

  bobgui_search_bar_set_key_capture_widget (BOBGUI_SEARCH_BAR (searchbar), button);

  g_assert_true (bobgui_search_bar_get_key_capture_widget (BOBGUI_SEARCH_BAR (searchbar)) == button);

  g_object_unref (button);

  g_assert_null (bobgui_search_bar_get_key_capture_widget (BOBGUI_SEARCH_BAR (searchbar)));

  g_object_unref (searchbar);
}

static void
capture_widget_unset (void)
{
  BobguiWidget *searchbar = bobgui_search_bar_new ();
  BobguiWidget *button = bobgui_button_new ();

  g_object_ref_sink (searchbar);
  g_object_ref_sink (button);

  bobgui_search_bar_set_key_capture_widget (BOBGUI_SEARCH_BAR (searchbar), button);

  g_assert_true (bobgui_search_bar_get_key_capture_widget (BOBGUI_SEARCH_BAR (searchbar)) == button);

  bobgui_search_bar_set_key_capture_widget (BOBGUI_SEARCH_BAR (searchbar), NULL);

  g_assert_null (bobgui_search_bar_get_key_capture_widget (BOBGUI_SEARCH_BAR (searchbar)));

  g_object_unref (searchbar);
  g_object_unref (button);
}

int
main (int   argc,
      char *argv[])
{
  bobgui_test_init (&argc, &argv);

  g_test_add_func ("/searchbar/capture-widget-destroy", capture_widget_destroy);
  g_test_add_func ("/searchbar/capture-widget-unset", capture_widget_unset);

  return g_test_run();
}
