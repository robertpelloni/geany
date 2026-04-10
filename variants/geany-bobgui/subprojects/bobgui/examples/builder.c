#include <bobgui/bobgui.h>
#include <glib/gstdio.h>

static void
print_hello (BobguiWidget *widget,
             gpointer   data)
{
  g_print ("Hello World\n");
}

static void
quit_cb (BobguiWidget *widget, gpointer data)
{
  BobguiWindow *window = data;

  bobgui_window_close (window);
}

static void
activate (BobguiApplication *app,
          gpointer        user_data)
{
  /* Construct a BobguiBuilder instance and load our UI description */
  BobguiBuilder *builder = bobgui_builder_new ();
  bobgui_builder_add_from_file (builder, "builder.ui", NULL);

  /* Connect signal handlers to the constructed widgets. */
  GObject *window = bobgui_builder_get_object (builder, "window");
  bobgui_window_set_application (BOBGUI_WINDOW (window), app);

  GObject *button = bobgui_builder_get_object (builder, "button1");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  button = bobgui_builder_get_object (builder, "button2");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  button = bobgui_builder_get_object (builder, "quit");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (quit_cb), window);

  bobgui_window_present (BOBGUI_WINDOW (window));
  g_object_unref (builder);
}

int
main (int   argc,
      char *argv[])
{
#ifdef BOBGUI_SRCDIR
  g_chdir (BOBGUI_SRCDIR);
#endif

  BobguiApplication *app = bobgui_application_new ("org.bobgui.example", G_APPLICATION_DEFAULT_FLAGS);
  g_signal_connect (app, "activate", G_CALLBACK (activate), NULL);

  int status = g_application_run (G_APPLICATION (app), argc, argv);
  g_object_unref (app);

  return status;
}
