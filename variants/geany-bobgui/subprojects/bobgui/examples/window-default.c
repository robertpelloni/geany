#include <bobgui/bobgui.h>

static void
activate (BobguiApplication* app,
          gpointer        user_data)
{
  BobguiWidget *window;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Window");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 200, 200);
  bobgui_window_present (BOBGUI_WINDOW (window));
}

int
main (int    argc,
      char **argv)
{
  BobguiApplication *app;
  int status;

  app = bobgui_application_new ("org.bobgui.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);
  status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
