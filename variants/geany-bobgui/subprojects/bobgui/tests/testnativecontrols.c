#include <bobgui/bobgui.h>

static void
spawn_side_window (BobguiWidget *button)
{
  BobguiWidget *window, *headerbar, *checkbox;

  window = bobgui_application_window_new (BOBGUI_APPLICATION (g_application_get_default ()));
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Side window");

  headerbar = bobgui_header_bar_new ();
  bobgui_header_bar_set_use_native_controls (BOBGUI_HEADER_BAR (headerbar), TRUE);
  bobgui_window_set_titlebar (BOBGUI_WINDOW (window), headerbar);

  checkbox = bobgui_check_button_new_with_label ("Click me to do things");
  bobgui_window_set_child (BOBGUI_WINDOW (window), checkbox);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

static void
activated (BobguiApplication *app)
{
  BobguiWidget *window, *headerbar, *button;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Top window");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 480, 480);

  headerbar = bobgui_header_bar_new ();
  bobgui_header_bar_set_use_native_controls (BOBGUI_HEADER_BAR (headerbar), TRUE);
  bobgui_window_set_titlebar (BOBGUI_WINDOW (window), headerbar);

  button = bobgui_button_new_with_label ("Spawn another window");
  bobgui_window_set_child (BOBGUI_WINDOW (window), button);
  g_signal_connect (button, "clicked", G_CALLBACK (spawn_side_window), NULL);

  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int argc, char **argv)
{
  BobguiApplication *app;
  int status;

  app = bobgui_application_new ("com.example.App", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activated), NULL);

  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);
  return status;
}

