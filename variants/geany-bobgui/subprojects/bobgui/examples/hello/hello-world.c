#include <bobgui/bobgui.h>

static void
print_hello (BobguiWidget *widget,
             gpointer   data)
{
  g_print ("Hello World\n");
}

static void
activate (BobguiApplication *app,
          gpointer        user_data)
{
  BobguiWidget *window;
  BobguiWidget *button;

  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Window");
  bobgui_window_set_default_size (BOBGUI_WINDOW (window), 200, 200);

  button = bobgui_button_new_with_label ("Hello World");
  bobgui_widget_set_halign (button, BOBGUI_ALIGN_CENTER);
  bobgui_widget_set_valign (button, BOBGUI_ALIGN_CENTER);

  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (bobgui_window_destroy), window);

  bobgui_window_set_child (BOBGUI_WINDOW (window), button);

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
