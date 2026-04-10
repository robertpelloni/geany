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
  BobguiWidget *grid;
  BobguiWidget *button;

  /* create a new window, and set its title */
  window = bobgui_application_window_new (app);
  bobgui_window_set_title (BOBGUI_WINDOW (window), "Window");

  /* Here we construct the container that is going pack our buttons */
  grid = bobgui_grid_new ();

  /* Pack the container in the window */
  bobgui_window_set_child (BOBGUI_WINDOW (window), grid);

  button = bobgui_button_new_with_label ("Button 1");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  /* Place the first button in the grid cell (0, 0), and make it fill
   * just 1 cell horizontally and vertically (ie no spanning)
   */
  bobgui_grid_attach (BOBGUI_GRID (grid), button, 0, 0, 1, 1);

  button = bobgui_button_new_with_label ("Button 2");
  g_signal_connect (button, "clicked", G_CALLBACK (print_hello), NULL);

  /* Place the second button in the grid cell (1, 0), and make it fill
   * just 1 cell horizontally and vertically (ie no spanning)
   */
  bobgui_grid_attach (BOBGUI_GRID (grid), button, 1, 0, 1, 1);

  button = bobgui_button_new_with_label ("Quit");
  g_signal_connect_swapped (button, "clicked", G_CALLBACK (bobgui_window_destroy), window);

  /* Place the Quit button in the grid cell (0, 1), and make it
   * span 2 columns.
   */
  bobgui_grid_attach (BOBGUI_GRID (grid), button, 0, 1, 2, 1);

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
